#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "inject.hpp"
#include "embedded_payload.hpp"

#include <tlhelp32.h>
#include <psapi.h>
#include <shlobj.h>
#include <shellapi.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <random>
#include <string>
#include <vector>

namespace
{
	constexpr wchar_t kProcessName[] = L"PenguinHotel-Win64-Shipping.exe";
	constexpr wchar_t kDllName[] = L"xv_meca.dll";
	constexpr wchar_t kPayloadFolderName[] = L"xv_meca";
	constexpr wchar_t kCheatRoot[] = L"C:\\VComDev\\xv_meca";
	constexpr char kEmbeddedDllName[] = "xv_meca.dll";
	constexpr wchar_t kDllNamePrefix[] = L"xv_meca_";
	constexpr wchar_t kDllNameSuffix[] = L".dll";
	constexpr wchar_t kRequiredVersion[] = L"14.50.35719.0";
	constexpr DWORD kSteamAppId = 4704690;
	constexpr wchar_t kGameLaunchArgs[] = L"-dx11";
	constexpr wchar_t kGameRelativePath[] =
		L"steamapps\\common\\MECCHA CHAMELEON\\Chameleon\\Binaries\\Win64\\PenguinHotel-Win64-Shipping.exe";
	constexpr DWORD kGameLaunchTimeoutMs = 120000;
	constexpr DWORD kGameLaunchPollMs = 500;
	constexpr DWORD kGameStartupStabilizeMs = 10000;

	std::wstring g_PayloadDirectory;
	std::wstring g_CheatDataDirectory;
	std::wstring g_DllPath;

	auto Utf8ToWide( const char* text ) -> std::wstring
	{
		if ( !text || !*text )
			return {};

		const int length = MultiByteToWideChar( CP_UTF8 , 0 , text , -1 , nullptr , 0 );
		if ( length <= 0 )
			return {};

		std::wstring result( static_cast<size_t>( length ) , L'\0' );
		MultiByteToWideChar( CP_UTF8 , 0 , text , -1 , result.data() , length );
		if ( !result.empty() && result.back() == L'\0' )
			result.pop_back();
		return result;
	}

	auto EnsureDirectory( const std::wstring& path ) -> bool
	{
		if ( CreateDirectoryW( path.c_str() , nullptr ) )
			return true;

		return GetLastError() == ERROR_ALREADY_EXISTS;
	}

	auto EnsureDirectoryTree( const std::wstring& path ) -> bool
	{
		if ( path.empty() )
			return false;

		if ( EnsureDirectory( path ) )
			return true;

		const size_t slash = path.find_last_of( L"\\/" );
		if ( slash == std::wstring::npos )
			return false;

		if ( !EnsureDirectoryTree( path.substr( 0 , slash ) ) )
			return false;

		return EnsureDirectory( path );
	}

	auto BuildPayloadDirectory() -> std::wstring
	{
		wchar_t localAppData[MAX_PATH]{};
		if ( SHGetFolderPathW( nullptr , CSIDL_LOCAL_APPDATA , nullptr , SHGFP_TYPE_CURRENT , localAppData ) != S_OK )
		{
			if ( GetTempPathW( MAX_PATH , localAppData ) == 0 )
				return {};
		}

		std::wstring directory = localAppData;
		if ( !directory.empty() && directory.back() != L'\\' )
			directory.push_back( L'\\' );

		directory += kPayloadFolderName;
		directory += L"\\payload\\";
		return directory;
	}

	auto WriteFileFromMemory( const std::wstring& path , const unsigned char* data , size_t size ) -> bool
	{
		const HANDLE fileHandle = CreateFileW(
			path.c_str() ,
			GENERIC_WRITE ,
			0 ,
			nullptr ,
			CREATE_ALWAYS ,
			FILE_ATTRIBUTE_NORMAL ,
			nullptr );

		if ( fileHandle == INVALID_HANDLE_VALUE )
			return false;

		DWORD totalWritten = 0;
		while ( totalWritten < size )
		{
			DWORD written = 0;
			const DWORD chunk = static_cast<DWORD>( std::min<size_t>( size - totalWritten , 1 << 20 ) );

			if ( !WriteFile( fileHandle , data + totalWritten , chunk , &written , nullptr ) || written == 0 )
			{
				CloseHandle( fileHandle );
				return false;
			}

			totalWritten += written;
		}

		CloseHandle( fileHandle );
		return true;
	}

	auto GetEmbeddedEntry( const char* name ) -> const EmbeddedPayload::FileEntry*
	{
		const auto* files = EmbeddedPayload::GetFiles();
		const auto count = EmbeddedPayload::GetFileCount();
		if ( !files || count == 0 )
			return nullptr;

		for ( size_t index = 0; index < count; ++index )
		{
			if ( files[index].Name && name && strcmp( files[index].Name , name ) == 0 )
				return &files[index];
		}

		return nullptr;
	}

	auto ComputeDllFingerprint( const unsigned char* data , size_t size ) -> uint32_t
	{
		uint32_t hash = 2166136261u;
		for ( size_t index = 0; index < size; ++index )
		{
			hash ^= data[index];
			hash *= 16777619u;
		}
		return hash;
	}

	auto BuildVersionedDllName( uint32_t fingerprint ) -> std::wstring
	{
		wchar_t buffer[64]{};
		swprintf_s( buffer , L"%s%08X%s" , kDllNamePrefix , fingerprint , kDllNameSuffix );
		return buffer;
	}

	auto IsVersionedDllName( const wchar_t* fileName ) -> bool
	{
		if ( !fileName || !*fileName )
			return false;

		if ( wcsncmp( fileName , kDllNamePrefix , wcslen( kDllNamePrefix ) ) != 0 )
			return false;

		const size_t suffixLength = wcslen( kDllNameSuffix );
		const size_t nameLength = wcslen( fileName );
		if ( nameLength < wcslen( kDllNamePrefix ) + suffixLength + 8 )
			return false;

		return _wcsicmp( fileName + nameLength - suffixLength , kDllNameSuffix ) == 0;
	}

	auto IsOurInjectedModulePath( const wchar_t* modulePath ) -> bool
	{
		if ( !modulePath || !*modulePath )
			return false;

		if ( !wcsstr( modulePath , kPayloadFolderName ) )
			return false;

		const wchar_t* fileName = wcsrchr( modulePath , L'\\' );
		fileName = fileName ? fileName + 1 : modulePath;
		return IsVersionedDllName( fileName ) || _wcsicmp( fileName , kDllName ) == 0;
	}

	auto IsMsvcVersionCorrect() -> bool
	{
		wchar_t systemDir[MAX_PATH]{};
		GetSystemDirectoryW( systemDir , MAX_PATH );
		const std::wstring msvcPath = std::wstring( systemDir ) + L"\\msvcp140.dll";

		DWORD versionHandle = 0;
		const DWORD versionSize = GetFileVersionInfoSizeW( msvcPath.c_str() , &versionHandle );
		if ( versionSize == 0 )
			return false;

		std::vector<char> versionData( versionSize );
		if ( !GetFileVersionInfoW( msvcPath.c_str() , 0 , versionSize , versionData.data() ) )
			return false;

		VS_FIXEDFILEINFO* fileInfo = nullptr;
		UINT fileInfoSize = 0;
		if ( !VerQueryValueW( versionData.data() , L"\\" , reinterpret_cast<LPVOID*>( &fileInfo ) , &fileInfoSize ) )
			return false;

		wchar_t currentVersion[32]{};
		swprintf_s(
			currentVersion ,
			L"%u.%u.%u.%u" ,
			HIWORD( fileInfo->dwFileVersionMS ) ,
			LOWORD( fileInfo->dwFileVersionMS ) ,
			HIWORD( fileInfo->dwFileVersionLS ) ,
			LOWORD( fileInfo->dwFileVersionLS ) );

		int cMajor , cMinor , cBuild , cRevision;
		int rMajor , rMinor , rBuild , rRevision;
		swscanf_s( currentVersion , L"%d.%d.%d.%d" , &cMajor , &cMinor , &cBuild , &cRevision );
		swscanf_s( kRequiredVersion , L"%d.%d.%d.%d" , &rMajor , &rMinor , &rBuild , &rRevision );

		if ( cMajor != rMajor ) return cMajor > rMajor;
		if ( cMinor != rMinor ) return cMinor > rMinor;
		if ( cBuild != rBuild ) return cBuild > rBuild;
		return cRevision >= rRevision;
	}

	auto EnableDebugPrivilege() -> bool
	{
		HANDLE token = nullptr;
		if ( !OpenProcessToken( GetCurrentProcess() , TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY , &token ) )
			return false;

		LUID luid{};
		if ( !LookupPrivilegeValueW( nullptr , L"SeDebugPrivilege" , &luid ) )
		{
			CloseHandle( token );
			return false;
		}

		TOKEN_PRIVILEGES privileges{};
		privileges.PrivilegeCount = 1;
		privileges.Privileges[0].Luid = luid;
		privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		AdjustTokenPrivileges( token , FALSE , &privileges , sizeof( privileges ) , nullptr , nullptr );

		const bool success = GetLastError() == ERROR_SUCCESS;
		CloseHandle( token );
		return success;
	}

	auto FindTargetProcess() -> DWORD
	{
		PROCESSENTRY32W entry{ sizeof( entry ) };
		const HANDLE snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS , 0 );
		if ( snapshot == INVALID_HANDLE_VALUE )
			return 0;

		DWORD processId = 0;
		if ( Process32FirstW( snapshot , &entry ) )
		{
			do
			{
				if ( _wcsicmp( entry.szExeFile , kProcessName ) == 0 )
				{
					processId = entry.th32ProcessID;
					break;
				}
			} while ( Process32NextW( snapshot , &entry ) );
		}

		CloseHandle( snapshot );
		return processId;
	}

	auto IsDllLoaded( DWORD processId ) -> bool
	{
		const HANDLE process = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ , FALSE , processId );
		if ( !process )
			return false;

		HMODULE modules[1024]{};
		DWORD needed = 0;
		bool loaded = false;

		if ( EnumProcessModules( process , modules , sizeof( modules ) , &needed ) )
		{
			const DWORD count = needed / sizeof( HMODULE );
			for ( DWORD index = 0; index < count; ++index )
			{
				wchar_t modulePath[MAX_PATH]{};
				if ( GetModuleFileNameExW( process , modules[index] , modulePath , MAX_PATH ) &&
					 IsOurInjectedModulePath( modulePath ) )
				{
					loaded = true;
					break;
				}
			}
		}

		CloseHandle( process );
		return loaded;
	}

	auto ResolveSidecarDllPath() -> std::wstring
	{
		wchar_t exePath[MAX_PATH]{};
		GetModuleFileNameW( nullptr , exePath , MAX_PATH );
		wchar_t* slash = wcsrchr( exePath , L'\\' );
		if ( slash )
			*( slash + 1 ) = L'\0';
		else
			exePath[0] = L'\0';

		const std::wstring sidecar = std::wstring( exePath ) + kDllName;
		if ( GetFileAttributesW( sidecar.c_str() ) != INVALID_FILE_ATTRIBUTES )
			return sidecar;

		return {};
	}

	auto PrepareInjectPayload( std::wstring& outError ) -> bool
	{
		const auto* dllEntry = GetEmbeddedEntry( kEmbeddedDllName );
		if ( dllEntry && dllEntry->Data && dllEntry->Size > 0 )
		{
			const auto fingerprint = ComputeDllFingerprint( dllEntry->Data , dllEntry->Size );
			g_DllPath = g_PayloadDirectory + BuildVersionedDllName( fingerprint );

			if ( !WriteFileFromMemory( g_DllPath , dllEntry->Data , dllEntry->Size ) )
			{
				outError = L"Failed to write embedded DLL";
				return false;
			}

			return true;
		}

		const std::wstring sidecar = ResolveSidecarDllPath();
		if ( sidecar.empty() )
		{
			outError = L"Embedded DLL missing. Rebuild with scripts\\build.ps1";
			return false;
		}

		g_DllPath = sidecar;
		return true;
	}

	auto InjectDll( DWORD processId , const std::wstring& dllPath ) -> bool
	{
		EnableDebugPrivilege();

		const HANDLE process = OpenProcess(
			PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ ,
			FALSE ,
			processId );

		if ( !process )
			return false;

		const size_t pathSize = ( dllPath.size() + 1 ) * sizeof( wchar_t );
		void* remoteMemory = VirtualAllocEx( process , nullptr , pathSize , MEM_COMMIT | MEM_RESERVE , PAGE_READWRITE );
		if ( !remoteMemory )
		{
			CloseHandle( process );
			return false;
		}

		if ( !WriteProcessMemory( process , remoteMemory , dllPath.c_str() , pathSize , nullptr ) )
		{
			VirtualFreeEx( process , remoteMemory , 0 , MEM_RELEASE );
			CloseHandle( process );
			return false;
		}

		const auto loadLibrary = reinterpret_cast<LPTHREAD_START_ROUTINE>(
			GetProcAddress( GetModuleHandleW( L"kernel32.dll" ) , "LoadLibraryW" ) );

		if ( !loadLibrary )
		{
			VirtualFreeEx( process , remoteMemory , 0 , MEM_RELEASE );
			CloseHandle( process );
			return false;
		}

		const HANDLE thread = CreateRemoteThread( process , nullptr , 0 , loadLibrary , remoteMemory , 0 , nullptr );
		if ( !thread )
		{
			VirtualFreeEx( process , remoteMemory , 0 , MEM_RELEASE );
			CloseHandle( process );
			return false;
		}

		WaitForSingleObject( thread , INFINITE );
		CloseHandle( thread );
		CloseHandle( process );
		return true;
	}

	auto ReadSteamInstallPath( std::wstring& outPath ) -> bool
	{
		HKEY steamKey = nullptr;
		if ( RegOpenKeyExW( HKEY_CURRENT_USER , L"Software\\Valve\\Steam" , 0 , KEY_READ , &steamKey ) != ERROR_SUCCESS )
			return false;

		wchar_t steamPath[MAX_PATH]{};
		DWORD pathSize = sizeof( steamPath );
		const LSTATUS status = RegQueryValueExW(
			steamKey ,
			L"SteamPath" ,
			nullptr ,
			nullptr ,
			reinterpret_cast<LPBYTE>( steamPath ) ,
			&pathSize );

		RegCloseKey( steamKey );

		if ( status != ERROR_SUCCESS || steamPath[0] == L'\0' )
			return false;

		outPath = steamPath;
		return true;
	}

	auto ReadSteamExePath( std::wstring& outPath ) -> bool
	{
		HKEY steamKey = nullptr;
		if ( RegOpenKeyExW( HKEY_CURRENT_USER , L"Software\\Valve\\Steam" , 0 , KEY_READ , &steamKey ) != ERROR_SUCCESS )
			return false;

		wchar_t steamExe[MAX_PATH]{};
		DWORD pathSize = sizeof( steamExe );
		const LSTATUS status = RegQueryValueExW(
			steamKey ,
			L"SteamExe" ,
			nullptr ,
			nullptr ,
			reinterpret_cast<LPBYTE>( steamExe ) ,
			&pathSize );

		RegCloseKey( steamKey );

		if ( status != ERROR_SUCCESS || steamExe[0] == L'\0' )
			return false;

		outPath = steamExe;
		return true;
	}

	auto TryLaunchGameDirect( std::wstring& outError ) -> bool
	{
		std::wstring steamRoot;
		if ( !ReadSteamInstallPath( steamRoot ) )
			return false;

		const std::wstring exePath = steamRoot + L"\\" + kGameRelativePath;
		if ( GetFileAttributesW( exePath.c_str() ) == INVALID_FILE_ATTRIBUTES )
			return false;

		std::wstring commandLine = L"\"" + exePath + L"\" " + kGameLaunchArgs;
		STARTUPINFOW startupInfo{};
		PROCESS_INFORMATION processInfo{};
		startupInfo.cb = sizeof( startupInfo );

		if ( !CreateProcessW(
				exePath.c_str() ,
				commandLine.data() ,
				nullptr ,
				nullptr ,
				FALSE ,
				0 ,
				nullptr ,
				nullptr ,
				&startupInfo ,
				&processInfo ) )
		{
			outError = L"Failed to launch game executable";
			return false;
		}

		CloseHandle( processInfo.hThread );
		CloseHandle( processInfo.hProcess );
		return true;
	}

	auto LaunchGame( std::wstring& outError ) -> bool
	{
		std::wstring steamExe;
		if ( ReadSteamExePath( steamExe ) )
		{
			const std::wstring commandLine =
				L"-applaunch " + std::to_wstring( kSteamAppId ) + L" " + kGameLaunchArgs;

			const HINSTANCE result = ShellExecuteW(
				nullptr ,
				L"open" ,
				steamExe.c_str() ,
				commandLine.c_str() ,
				nullptr ,
				SW_SHOWNORMAL );

			if ( reinterpret_cast<intptr_t>( result ) > 32 )
				return true;
		}

		const std::wstring steamUrl =
			L"steam://run/" + std::to_wstring( kSteamAppId ) + L"//" + kGameLaunchArgs;

		const HINSTANCE urlResult = ShellExecuteW(
			nullptr ,
			L"open" ,
			steamUrl.c_str() ,
			nullptr ,
			nullptr ,
			SW_SHOWNORMAL );

		if ( reinterpret_cast<intptr_t>( urlResult ) > 32 )
			return true;

		if ( TryLaunchGameDirect( outError ) )
			return true;

		outError = L"Failed to launch Meccha Chameleon. Is Steam installed?";
		return false;
	}

	auto WaitForTargetProcess( DWORD timeoutMs , InjectState& state ) -> bool
	{
		const DWORD startTick = GetTickCount();

		while ( GetTickCount() - startTick < timeoutMs )
		{
			const DWORD processId = FindTargetProcess();
			if ( processId != 0 )
			{
				state.ProcessId = processId;
				state.ProcessName = kProcessName;
				state.DllLoaded = IsDllLoaded( processId );
				return true;
			}

			Sleep( kGameLaunchPollMs );
		}

		return false;
	}

	auto EnsureGameRunningImpl( InjectState& state , std::wstring& outError ) -> bool
	{
		const DWORD processId = FindTargetProcess();
		if ( processId != 0 )
		{
			state.ProcessId = processId;
			state.ProcessName = kProcessName;
			state.DllLoaded = IsDllLoaded( processId );
			return true;
		}

		state.Status = InjectStatus::LaunchingGame;
		state.StatusText = L"Launching game via Steam (-dx11)...";

		if ( !LaunchGame( outError ) )
		{
			state.Status = InjectStatus::Failed;
			state.StatusText = outError;
			return false;
		}

		state.Status = InjectStatus::WaitingForGame;
		state.StatusText = L"Waiting for game to start...";

		if ( !WaitForTargetProcess( kGameLaunchTimeoutMs , state ) )
		{
			outError = L"Game did not start within 2 minutes";
			state.Status = InjectStatus::Failed;
			state.StatusText = outError;
			return false;
		}

		state.StatusText = L"Game started, waiting for load...";
		Sleep( kGameStartupStabilizeMs );

		state.DllLoaded = IsDllLoaded( state.ProcessId );
		return true;
	}
}

auto GetDllPath() -> const std::wstring&
{
	return g_DllPath;
}

auto GetPayloadDirectory() -> const std::wstring&
{
	return g_PayloadDirectory;
}

auto GetCheatDataDirectory() -> const std::wstring&
{
	return g_CheatDataDirectory;
}

auto InitializePayload( std::wstring& outError ) -> bool
{
	if ( !IsMsvcVersionCorrect() )
	{
		outError = L"VC++ Redist 14.50+ required";
		return false;
	}

	{
		std::error_code ec;
		const std::wstring vcomRoot = L"C:\\VComDev";
		if ( std::filesystem::exists( vcomRoot ) )
			std::filesystem::remove_all( vcomRoot , ec );
	}

	const auto fileCount = EmbeddedPayload::GetFileCount();
	const auto* files = EmbeddedPayload::GetFiles();

	g_PayloadDirectory = BuildPayloadDirectory();
	g_CheatDataDirectory = kCheatRoot;

	if ( g_PayloadDirectory.empty() || !EnsureDirectoryTree( g_PayloadDirectory ) )
	{
		outError = L"Failed to create payload directory";
		return false;
	}

	if ( !EnsureDirectoryTree( g_CheatDataDirectory ) )
	{
		outError = L"Failed to create cheat data directory";
		return false;
	}

	if ( fileCount > 0 && files )
	{
		for ( size_t index = 0; index < fileCount; ++index )
		{
			const auto& entry = files[index];
			if ( !entry.Name || !entry.Data || entry.Size == 0 )
				continue;

			if ( strcmp( entry.Name , kEmbeddedDllName ) == 0 )
				continue;

			const std::wstring destination = g_CheatDataDirectory + L"\\" + Utf8ToWide( entry.Name );
			if ( !WriteFileFromMemory( destination , entry.Data , entry.Size ) )
			{
				outError = L"Failed to extract ";
				outError += Utf8ToWide( entry.Name );
				return false;
			}
		}
	}

	EnableDebugPrivilege();
	return true;
}

auto RefreshProcessState( InjectState& state ) -> void
{
	state.ProcessName = kProcessName;
	state.ProcessId = FindTargetProcess();

	if ( state.ProcessId == 0 )
	{
		state.DllLoaded = false;
		if ( state.Status != InjectStatus::Injecting && state.Status != InjectStatus::LaunchingGame )
		{
			state.Status = InjectStatus::WaitingForGame;
			state.StatusText = L"Waiting for game...";
		}
		return;
	}

	state.DllLoaded = IsDllLoaded( state.ProcessId );
	if ( state.DllLoaded )
	{
		state.Status = InjectStatus::AlreadyInjected;
		state.StatusText = L"Already injected. Menu: INS / HOME / RSHIFT";
	}
	else if ( state.Status != InjectStatus::Injecting )
	{
		state.Status = InjectStatus::GameFound;
		state.StatusText = L"Game found. Ready to inject";
	}
}

auto EnsureGameRunning( InjectState& state , std::wstring& outError ) -> bool
{
	return EnsureGameRunningImpl( state , outError );
}

auto PerformInject( InjectState& state , std::wstring& outError ) -> bool
{
	if ( g_PayloadDirectory.empty() && g_DllPath.empty() )
	{
		outError = L"Payload not initialized";
		state.Status = InjectStatus::Failed;
		state.StatusText = outError;
		return false;
	}

	RefreshProcessState( state );
	if ( state.ProcessId == 0 )
	{
		if ( !EnsureGameRunning( state , outError ) )
			return false;
	}

	if ( state.DllLoaded )
	{
		state.Status = InjectStatus::AlreadyInjected;
		state.StatusText = L"Already injected";
		return true;
	}

	if ( !PrepareInjectPayload( outError ) )
	{
		state.Status = InjectStatus::Failed;
		state.StatusText = outError;
		return false;
	}

	state.Status = InjectStatus::Injecting;
	state.StatusText = L"Injecting...";

	if ( !InjectDll( state.ProcessId , g_DllPath ) )
	{
		outError = L"Injection failed (try running as admin)";
		state.Status = InjectStatus::Failed;
		state.StatusText = outError;
		return false;
	}

	RefreshProcessState( state );
	if ( state.DllLoaded )
	{
		state.Status = InjectStatus::Injected;
		state.StatusText = L"Injected! Menu: INS / HOME / RSHIFT";
		return true;
	}

	outError = L"Injection finished but DLL not detected";
	state.Status = InjectStatus::Failed;
	state.StatusText = outError;
	return false;
}
