#include "overlay_canvas.h"

#include "nlohmann/json.hpp"

namespace
{
	std::mutex g_Mutex;
	float g_Width = 0.f;
	float g_Height = 0.f;
	std::vector<MecchaCheatV::OverlayCanvas::LineCmd> g_Lines;
	std::vector<MecchaCheatV::OverlayCanvas::RectCmd> g_Rects;
	std::vector<MecchaCheatV::OverlayCanvas::TextCmd> g_Texts;
}

namespace MecchaCheatV::OverlayCanvas
{
	void BeginFrame( float width , float height )
	{
		std::lock_guard lock( g_Mutex );
		g_Width = width;
		g_Height = height;
		g_Lines.clear();
		g_Rects.clear();
		g_Texts.clear();
	}

	void AddLine( float x1 , float y1 , float x2 , float y2 , uint32_t color , float thickness )
	{
		std::lock_guard lock( g_Mutex );
		g_Lines.push_back( LineCmd{ x1 , y1 , x2 , y2 , thickness , color } );
	}

	void AddRect( float x1 , float y1 , float x2 , float y2 , uint32_t color , float thickness , float rounding )
	{
		std::lock_guard lock( g_Mutex );
		g_Rects.push_back( RectCmd{ x1 , y1 , x2 , y2 , thickness , rounding , color } );
	}

	void AddText( float x , float y , uint32_t color , const char* text , float size )
	{
		if ( !text || !*text )
			return;

		std::lock_guard lock( g_Mutex );
		g_Texts.push_back( TextCmd{ x , y , size , color , text } );
	}

	std::string BuildFrameJson()
	{
		std::lock_guard lock( g_Mutex );

		nlohmann::json root;
		root["w"] = g_Width;
		root["h"] = g_Height;
		root["active"] = g_Width > 0.f && g_Height > 0.f;

		nlohmann::json lines = nlohmann::json::array();
		for ( const auto& line : g_Lines )
		{
			lines.push_back( nlohmann::json{
				{ "x1" , line.x1 } , { "y1" , line.y1 } , { "x2" , line.x2 } , { "y2" , line.y2 } ,
				{ "c" , line.color } , { "t" , line.thickness }
			} );
		}

		nlohmann::json rects = nlohmann::json::array();
		for ( const auto& rect : g_Rects )
		{
			rects.push_back( nlohmann::json{
				{ "x1" , rect.x1 } , { "y1" , rect.y1 } , { "x2" , rect.x2 } , { "y2" , rect.y2 } ,
				{ "c" , rect.color } , { "t" , rect.thickness } , { "r" , rect.rounding }
			} );
		}

		nlohmann::json texts = nlohmann::json::array();
		for ( const auto& text : g_Texts )
		{
			texts.push_back( nlohmann::json{
				{ "x" , text.x } , { "y" , text.y } , { "s" , text.size } ,
				{ "c" , text.color } , { "v" , text.text }
			} );
		}

		root["lines"] = lines;
		root["rects"] = rects;
		root["texts"] = texts;
		return root.dump();
	}
}
