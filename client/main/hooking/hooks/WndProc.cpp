#include "Includes.h"

using namespace MecchaCheatV;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT __stdcall Hooks::HkWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN)
    {
		if (wParam == VK_DELETE)
		{
            Globals::IsDebugging = true;
            Globals::IsCalledLogs = true;
			NOTIFY_INFO_QUICK("Test mode and called logs switched.");
			return TRUE;
		}
    }

    const WNDPROC nextProc = hooking ? hooking->OriginalWndproc : nullptr;
    if ( !nextProc || nextProc == HkWndProc )
        return DefWindowProcW( hWnd , uMsg , wParam , lParam );

    return CallWindowProcW( nextProc , hWnd , uMsg , wParam , lParam );
}