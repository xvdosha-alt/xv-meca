#include "hooking.h"

using namespace MecchaCheatV;

Hooking::Hooking()
{
    hooking = this;
}

Hooking::~Hooking()
{
    if (renderer && renderer->Window)
        SetWindowLongPtr(renderer->Window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(OriginalWndproc));

    hooking = nullptr;
}

void Hooking::AddHook(const std::string& name, PVOID* original, PVOID hook)
{
    hooks_.emplace_back(name, original, hook);
}

void Hooking::ApplyHooks() const
{
#if defined(__MINGW32__)
    LOG_HOOKS("MinHook begin");

    if (MH_Initialize() != MH_OK)
        throw std::runtime_error("MinHook init fail");

    for (auto& h : hooks_)
    {
        LOG_HOOKS("Create: " + std::get<0>(h));
        const auto target = *std::get<1>(h);
        const auto status = MH_CreateHook(target, std::get<2>(h), std::get<1>(h));
        if (status != MH_OK)
            LOG_ERROR("Create fail: " + std::get<0>(h) + " err=" + std::to_string(status));
    }

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
        throw std::runtime_error("MinHook enable fail");

    LOG_HOOKS("MinHook enabled");
#else
    LOG_HOOKS("Detour begin");

    if (DetourTransactionBegin() != NO_ERROR)
        throw std::runtime_error("Detour begin fail");

    if (DetourUpdateThread(GetCurrentThread()) != NO_ERROR)
        throw std::runtime_error("Detour thread fail");

    for (auto& h : hooks_)
    {
        LOG_HOOKS("Attach: " + std::get<0>(h));
        auto e = DetourAttach(std::get<1>(h), std::get<2>(h));
        if (e != NO_ERROR)
            LOG_ERROR("Attach fail: " + std::get<0>(h) + " err=" + std::to_string(e));
    }

    if (DetourTransactionCommit() != NO_ERROR)
        throw std::runtime_error("Detour commit fail");

    LOG_HOOKS("Detour commit");
#endif
}

void Hooking::RemoveHooks() const
{
#if defined(__MINGW32__)
    LOG_HOOKS("MinHook remove begin");

    if (hooks_.empty())
    {
        LOG_HOOKS("No hooks");
        return;
    }

    if (MH_DisableHook(MH_ALL_HOOKS) != MH_OK)
        LOG_ERROR("MinHook disable fail");

    if (MH_Uninitialize() != MH_OK)
        LOG_ERROR("MinHook uninit fail");

    LOG_HOOKS("MinHook remove done");
#else
    LOG_HOOKS("Detour remove begin");

    if (hooks_.empty())
    {
        LOG_HOOKS("No hooks");
        return;
    }

    if (DetourTransactionBegin() != NO_ERROR)
        return;

    if (DetourUpdateThread(GetCurrentThread()) != NO_ERROR)
    {
        DetourTransactionAbort();
        return;
    }

    for (auto& h : hooks_)
    {
        LOG_HOOKS("Detach: " + std::get<0>(h));
        auto e = DetourDetach(std::get<1>(h), std::get<2>(h));
        if (e != NO_ERROR)
            LOG_ERROR("Detach fail: " + std::get<0>(h) + " err=" + std::to_string(e));
    }

    if (DetourTransactionCommit() != NO_ERROR)
        LOG_ERROR("Detour remove commit fail");

    LOG_HOOKS("Detour remove done");
#endif
}
