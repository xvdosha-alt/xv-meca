#include "Includes.h"
#include <TlHelp32.h>
#include "memory.h"
#include "main/webpanel/web_server.h"

using namespace MecchaCheatV;

static std::unique_ptr<Logger> loggerInstance;
static std::unique_ptr<Renderer> rendererInstance;
static std::unique_ptr<Hooking> hookingInstance;
static std::unique_ptr<FeatureHandler> featureInstance;

bool IsLikelyFunction(void* address)
{
    if (!address)
        return false;

    MEMORY_BASIC_INFORMATION mbi{};
    if (!VirtualQuery(address, &mbi, sizeof(mbi)))
        return false;

    if (mbi.State != MEM_COMMIT)
        return false;

    DWORD protect = mbi.Protect & 0xFF;

    if (protect != PAGE_EXECUTE &&
        protect != PAGE_EXECUTE_READ &&
        protect != PAGE_EXECUTE_READWRITE &&
        protect != PAGE_EXECUTE_WRITECOPY)
    {
        return false;
    }

    auto* bytes = reinterpret_cast<uint8_t*>(address);

    bool allZero = true;
    bool allCC = true;
    bool allFF = true;

    for (int i = 0; i < 16; i++)
    {
        if (bytes[i] != 0x00) allZero = false;
        if (bytes[i] != 0xCC) allCC = false;
        if (bytes[i] != 0xFF) allFF = false;
    }

    if (allZero || allCC || allFF)
        return false;

    if (bytes[0] == 0x40 ||                
        bytes[0] == 0x48 ||                
        bytes[0] == 0x55 ||                
        bytes[0] == 0x53 ||                
        bytes[0] == 0x57 ||                
        bytes[0] == 0x41 ||                
        bytes[0] == 0xE9 ||                
        bytes[0] == 0xEB)                  
    {
        return true;
    }

    return false;
}

extern "C" __declspec(dllexport) DWORD WINAPI MecchaCheatVThread()
{
    Utils::CreateCheatDirectory();

    bool hooksApplied = false;

    try {
        loggerInstance = std::make_unique<Logger>(Logger::Level::Call);
    }
    catch (...) {
        return 0;
    }

    try {
        rendererInstance = std::make_unique<Renderer>();
        hookingInstance = std::make_unique<Hooking>();
        featureInstance = std::make_unique<FeatureHandler>();

        hookingInstance->OriginalPresent = rendererInstance->GetPresent();
        hookingInstance->OriginalResizeBuffers = rendererInstance->GetResizeBuffers();

        if ( !hookingInstance->OriginalPresent || !hookingInstance->OriginalResizeBuffers )
        {
            LOG_ERROR( "Failed to resolve DXGI hooks (Present/ResizeBuffers)");
            goto failedaddr;
        }

        if (IsDebugging)
            LOG_WARN("The build is built with the IsDebugging flag enabled.");

        uintptr_t addr = SDK::InSDKUtils::GetImageBase() + SDK::Offsets::ProcessEvent;

        Globals::hookedProcessEvent = reinterpret_cast<ProcessEvent_t>(addr);

        auto* bytes = reinterpret_cast<uint8_t*>(addr);

        std::ostringstream oss;
        oss << std::hex << std::uppercase << std::setfill('0');

        for (int i = 0; i < 16; i++)
            oss << std::setw(2) << (int)bytes[i] << ' ';

		bool likelyFunction = IsLikelyFunction((void*)addr);
        if (!likelyFunction)
        {
			LOG_ERROR("Failed to verify the ProcessEvent address.\nThe specified address does not appear to point to a valid function.\n\nMecchaCheatV will now unload automatically to prevent a crash."); 
			LOG_RELEASE(FOREGROUND_RED, "Failed to verify the ProcessEvent address.\nThe specified address does not appear to point to a valid function.\n\nMecchaCheatV will now unload automatically to prevent a crash."); 
            goto failedaddr;
        }

        AHK( hookingInstance->OriginalPresent , Hooks::HkPresent );
        AHK( hookingInstance->OriginalResizeBuffers , Hooks::HkResizeBuffers );
        AHK( Globals::hookedProcessEvent , Hooks::HkProcessEvent );

        hookingInstance->ApplyHooks();
        hooksApplied = true;

        if ( WebPanel::Start() )
            LOG_INFO( "Web panel started at %s" , WebPanel::GetPanelUrl().c_str() );
        else
            LOG_ERROR( "Failed to start web panel on port %u" , WebPanelPort );

        LOG_RELEASE(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY, "Welcome to xv_meca\n");
        LOG_RELEASE(FOREGROUND_BLUE | FOREGROUND_INTENSITY, std::string(32, '-').c_str(), "\n");
        LOG_RELEASE(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY, "Web panel:\n");
        LOG_RELEASE(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY, ( WebPanel::GetPanelUrl() + "\n" ).c_str() );
        LOG_RELEASE(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY, "ESP overlay: http://127.0.0.1:17777/web\n");
        LOG_RELEASE(FOREGROUND_BLUE | FOREGROUND_INTENSITY, std::string(32, '-').c_str(), "\n");

        LOG_INFO( "Cheat injected. Web panel: %s" , WebPanel::GetPanelUrl().c_str() );

        while (Globals::CheatWork)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR(std::string("Exception in main thread: ") + e.what());
    }

    LOG_INFO("Starting cleanup...");

    WebPanel::Stop();

failedaddr:

    if (hooksApplied && hookingInstance)
    {
        try {
            hookingInstance->RemoveHooks();
        }
        catch (...) {}
        hooksApplied = false;
    }

    hookingInstance.reset();
    rendererInstance.reset();
    featureInstance.reset();

    LOG_INFO("Cleanup completed");

    loggerInstance.reset();

    FreeLibraryAndExitThread(Globals::globalModule, NULL);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
    {
        Globals::CheatWork = true;
        DisableThreadLibraryCalls(hModule);
        Globals::globalModule = hModule;

        HANDLE hThread = CreateThread(nullptr, 0,
            reinterpret_cast<LPTHREAD_START_ROUTINE>(MecchaCheatVThread),
            nullptr, 0, nullptr);

        if (hThread)
            CloseHandle(hThread);

        break;
    }

    case DLL_PROCESS_DETACH:
    {
        Globals::CheatWork = false;
        Sleep(100);
        break;
    }
    }

    return TRUE;
}