#pragma once

#include <windows.h>

#include <string>

enum class InjectStatus : int
{
	Idle = 0 ,
	Extracting ,
	Ready ,
	WaitingForGame ,
	LaunchingGame ,
	GameFound ,
	Injecting ,
	Injected ,
	AlreadyInjected ,
	Failed ,
};

struct InjectState
{
	InjectStatus Status = InjectStatus::Idle;
	std::wstring ProcessName;
	DWORD ProcessId = 0;
	std::wstring StatusText;
	bool DllLoaded = false;
};

auto InitializePayload( std::wstring& outError ) -> bool;
auto EnsureGameRunning( InjectState& state , std::wstring& outError ) -> bool;
auto RefreshProcessState( InjectState& state ) -> void;
auto PerformInject( InjectState& state , std::wstring& outError ) -> bool;
auto GetDllPath() -> const std::wstring&;
auto GetPayloadDirectory() -> const std::wstring&;
auto GetCheatDataDirectory() -> const std::wstring&;
