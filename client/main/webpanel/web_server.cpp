#include "web_server.h"
#include "web_ui.h"
#include "web_ui_view.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <shellapi.h>

#include "../features/features.h"
#include "../../Globals.h"
#include "../menu/renderer.h"

#include <atomic>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#include "overlay_canvas.h"

#include "nlohmann/json.hpp"

#pragma comment( lib , "ws2_32.lib" )

namespace
{
	std::thread g_ServerThread;
	std::atomic<bool> g_Running{ false };
	SOCKET g_ListenSocket = INVALID_SOCKET;
	std::mutex g_RequestMutex;

	auto SendAll( SOCKET socket , const char* data , int length ) -> bool
	{
		int sent = 0;
		while ( sent < length )
		{
			const int chunk = send( socket , data + sent , length - sent , 0 );
			if ( chunk <= 0 )
				return false;
			sent += chunk;
		}
		return true;
	}

	auto HttpStatusText( int code ) -> const char*
	{
		switch ( code )
		{
		case 200: return "OK";
		case 400: return "Bad Request";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		default: return "Error";
		}
	}

	auto SendHttpResponse( SOCKET client , int code , const char* contentType , const std::string& body ) -> void
	{
		std::ostringstream response;
		response
			<< "HTTP/1.1 " << code << ' ' << HttpStatusText( code ) << "\r\n"
			<< "Content-Type: " << contentType << "\r\n"
			<< "Content-Length: " << body.size() << "\r\n"
			<< "Connection: close\r\n"
			<< "Cache-Control: no-store\r\n"
			<< "\r\n"
			<< body;

		const std::string payload = response.str();
		SendAll( client , payload.c_str() , static_cast<int>( payload.size() ) );
	}

	auto ReadRequest( SOCKET client , std::string& method , std::string& path , std::string& body ) -> bool
	{
		std::string raw;
		raw.reserve( 8192 );

		char buffer[4096];
		for ( int attempts = 0; attempts < 32 && raw.find( "\r\n\r\n" ) == std::string::npos; ++attempts )
		{
			const int received = recv( client , buffer , sizeof( buffer ) , 0 );
			if ( received <= 0 )
				return false;

			raw.append( buffer , received );
			if ( raw.size() > 1024 * 1024 )
				return false;
		}

		const auto headerEnd = raw.find( "\r\n\r\n" );
		if ( headerEnd == std::string::npos )
			return false;

		const std::string headers = raw.substr( 0 , headerEnd );
		body = raw.substr( headerEnd + 4 );

		const auto lineEnd = headers.find( "\r\n" );
		const std::string requestLine = headers.substr( 0 , lineEnd );

		std::istringstream lineStream( requestLine );
		lineStream >> method >> path;

		size_t contentLength = 0;
		{
			const auto lower = headers;
			const auto pos = lower.find( "Content-Length:" );
			if ( pos != std::string::npos )
			{
				auto valueStart = pos + 15;
				while ( valueStart < lower.size() && ( lower[valueStart] == ' ' || lower[valueStart] == '\t' ) )
					++valueStart;

				contentLength = static_cast<size_t>( std::stoul( lower.substr( valueStart ) ) );
			}
		}

		while ( body.size() < contentLength )
		{
			const int received = recv( client , buffer , sizeof( buffer ) , 0 );
			if ( received <= 0 )
				break;
			body.append( buffer , received );
		}

		if ( body.size() > contentLength )
			body.resize( contentLength );

		return !method.empty() && !path.empty();
	}

	auto HandleRequest( const std::string& method , const std::string& path , const std::string& body ) -> std::pair<int, std::string>
	{
		if ( method == "GET" && ( path == "/" || path == "/index.html" ) )
			return { 200 , std::string( MecchaCheatV::WebPanel::kIndexHtml ) };

		if ( method == "GET" && path == "/web" )
			return { 200 , std::string( MecchaCheatV::WebPanel::kWebViewHtml ) };

		if ( method == "GET" && path == "/api/web/frame" )
		{
			if ( !GET_FEATURE_HANDLER() || !GET_FEATURE_HANDLER()->IsWebOnlyEnabled() )
				return { 200 , R"({"w":0,"h":0,"active":false,"webOnly":false,"inMatch":false,"lines":[],"rects":[],"texts":[]})" };

			nlohmann::json frame = nlohmann::json::parse( MecchaCheatV::OverlayCanvas::BuildFrameJson() );
			frame["webOnly"] = true;
			frame["inMatch"] = false;

			auto world = SDK::UWorld::GetWorld();
			if ( world && Utils::IsInMatch( world ) )
				frame["inMatch"] = true;

			if ( !frame.value( "active" , false ) )
			{
				const float w = static_cast<float>( MecchaCheatV::Renderer::BufferWidth );
				const float h = static_cast<float>( MecchaCheatV::Renderer::BufferHeight );
				if ( w > 0.f && h > 0.f )
				{
					frame["w"] = w;
					frame["h"] = h;
					frame["active"] = true;
				}
			}

			return { 200 , frame.dump() };
		}

		if ( method == "GET" && path == "/api/state" )
		{
			if ( !GET_FEATURE_HANDLER() )
				return { 503 , R"({"error":"features unavailable"})" };

			return { 200 , GET_FEATURE_HANDLER()->BuildWebStateJson() };
		}

		if ( method == "POST" && path == "/api/feature" )
		{
			if ( !GET_FEATURE_HANDLER() )
				return { 503 , R"({"ok":false})" };

			try
			{
				const auto json = nlohmann::json::parse( body );
				const bool ok = GET_FEATURE_HANDLER()->ApplyWebFeatureUpdate( json );
				return { ok ? 200 : 400 , ok ? R"({"ok":true})" : R"({"ok":false})" };
			}
			catch ( ... )
			{
				return { 400 , R"({"ok":false})" };
			}
		}

		if ( method == "POST" && path == "/api/option" )
		{
			if ( !GET_FEATURE_HANDLER() )
				return { 503 , R"({"ok":false})" };

			try
			{
				const auto json = nlohmann::json::parse( body );
				const bool ok = GET_FEATURE_HANDLER()->ApplyWebOptionUpdate( json );
				return { ok ? 200 : 400 , ok ? R"({"ok":true})" : R"({"ok":false})" };
			}
			catch ( ... )
			{
				return { 400 , R"({"ok":false})" };
			}
		}

		if ( method == "GET" && path == "/api/teleport/players" )
		{
			if ( !GET_FEATURE_HANDLER() )
				return { 503 , R"({"players":[]})" };

			return { 200 , GET_FEATURE_HANDLER()->BuildTeleportPlayersJson() };
		}

		if ( method == "POST" && path == "/api/teleport/coords" )
		{
			if ( !GET_FEATURE_HANDLER() )
				return { 503 , R"({"ok":false})" };

			try
			{
				const auto json = nlohmann::json::parse( body );
				const bool ok = GET_FEATURE_HANDLER()->ApplyTeleportCoords(
					json.value( "x" , 0.f ) ,
					json.value( "y" , 0.f ) ,
					json.value( "z" , 0.f ) );
				return { ok ? 200 : 400 , ok ? R"({"ok":true})" : R"({"ok":false})" };
			}
			catch ( ... )
			{
				return { 400 , R"({"ok":false})" };
			}
		}

		if ( method == "POST" && path == "/api/teleport/player" )
		{
			if ( !GET_FEATURE_HANDLER() )
				return { 503 , R"({"ok":false})" };

			try
			{
				const auto json = nlohmann::json::parse( body );
				if ( !json.contains( "index" ) || !json["index"].is_number_integer() )
					return { 400 , R"({"ok":false})" };

				const bool ok = GET_FEATURE_HANDLER()->ApplyTeleportToPlayer( json["index"].get<int>() );
				return { ok ? 200 : 400 , ok ? R"({"ok":true})" : R"({"ok":false})" };
			}
			catch ( ... )
			{
				return { 400 , R"({"ok":false})" };
			}
		}

		if ( method == "POST" && path == "/api/setname/apply" )
		{
			if ( !GET_FEATURE_HANDLER() )
				return { 503 , R"({"ok":false})" };

			try
			{
				const auto json = nlohmann::json::parse( body );
				if ( !json.contains( "name" ) || !json["name"].is_string() )
					return { 400 , R"({"ok":false})" };

				const bool ok = GET_FEATURE_HANDLER()->ApplySetName( json["name"].get<std::string>() );
				return { ok ? 200 : 400 , ok ? R"({"ok":true})" : R"({"ok":false})" };
			}
			catch ( ... )
			{
				return { 400 , R"({"ok":false})" };
			}
		}

		if ( method == "POST" && path == "/api/unload" )
		{
			MecchaCheatV::Globals::CheatWork.store( false );
			return { 200 , R"({"ok":true})" };
		}

		return { 404 , R"({"error":"not found"})" };
	}

	auto HandleClient( SOCKET client ) -> void
	{
		std::string method;
		std::string path;
		std::string body;

		if ( !ReadRequest( client , method , path , body ) )
		{
			closesocket( client );
			return;
		}

		int status = 404;
		std::string responseBody = R"({"error":"not found"})";

		{
			std::lock_guard lock( g_RequestMutex );
			std::tie( status , responseBody ) = HandleRequest( method , path , body );
		}

		const char* contentType = "application/json";
		if ( path == "/" || path == "/index.html" || path == "/web" )
			contentType = "text/html; charset=utf-8";

		SendHttpResponse( client , status , contentType , responseBody );
		closesocket( client );
	}

	auto ServerLoop() -> void
	{
		while ( g_Running.load() )
		{
			fd_set readSet;
			FD_ZERO( &readSet );
			FD_SET( g_ListenSocket , &readSet );

			timeval timeout{};
			timeout.tv_sec = 1;
			timeout.tv_usec = 0;

			const int ready = select( 0 , &readSet , nullptr , nullptr , &timeout );
			if ( ready <= 0 )
				continue;

			const SOCKET client = accept( g_ListenSocket , nullptr , nullptr );
			if ( client == INVALID_SOCKET )
				continue;

			HandleClient( client );
		}
	}
}

namespace MecchaCheatV::WebPanel
{
	bool Start()
	{
		if ( g_Running.load() )
			return true;

		WSADATA wsaData{};
		if ( WSAStartup( MAKEWORD( 2 , 2 ) , &wsaData ) != 0 )
			return false;

		g_ListenSocket = socket( AF_INET , SOCK_STREAM , IPPROTO_TCP );
		if ( g_ListenSocket == INVALID_SOCKET )
		{
			WSACleanup();
			return false;
		}

		BOOL reuse = TRUE;
		setsockopt( g_ListenSocket , SOL_SOCKET , SO_REUSEADDR , reinterpret_cast<const char*>( &reuse ) , sizeof( reuse ) );

		sockaddr_in address{};
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
		address.sin_port = htons( static_cast<u_short>( MecchaCheatV::Globals::WebPanelPort ) );

		if ( bind( g_ListenSocket , reinterpret_cast<sockaddr*>( &address ) , sizeof( address ) ) == SOCKET_ERROR )
		{
			closesocket( g_ListenSocket );
			g_ListenSocket = INVALID_SOCKET;
			WSACleanup();
			return false;
		}

		if ( listen( g_ListenSocket , SOMAXCONN ) == SOCKET_ERROR )
		{
			closesocket( g_ListenSocket );
			g_ListenSocket = INVALID_SOCKET;
			WSACleanup();
			return false;
		}

		g_Running.store( true );
		g_ServerThread = std::thread( ServerLoop );
		return true;
	}

	void Stop()
	{
		if ( !g_Running.exchange( false ) )
			return;

		if ( g_ListenSocket != INVALID_SOCKET )
		{
			closesocket( g_ListenSocket );
			g_ListenSocket = INVALID_SOCKET;
		}

		if ( g_ServerThread.joinable() )
			g_ServerThread.join();

		WSACleanup();
	}

	std::string GetPanelUrl()
	{
		return "http://127.0.0.1:" + std::to_string( MecchaCheatV::Globals::WebPanelPort );
	}

	void OpenInBrowser()
	{
		const std::string url = GetPanelUrl();
		ShellExecuteA( nullptr , "open" , url.c_str() , nullptr , nullptr , SW_SHOWNORMAL );
	}

	bool IsRunning()
	{
		return g_Running.load();
	}
}
