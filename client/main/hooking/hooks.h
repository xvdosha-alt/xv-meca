#pragma once
#include "Includes.h"

namespace MecchaCheatV
{
	struct Hooks
	{
		static HRESULT HkPresent( IDXGISwapChain* pSwapChain , UINT SyncInterval , UINT Flags );
		static HRESULT HkResizeBuffers(
			IDXGISwapChain* swapChain ,
			UINT bufferCount ,
			UINT width ,
			UINT height ,
			DXGI_FORMAT newFormat ,
			UINT swapChainFlags );
		static LRESULT HkWndProc( HWND hWnd , UINT uMsg , WPARAM wParam , LPARAM lParam );
		static void HkProcessEvent( SDK::UObject* Object , SDK::UFunction* Function , void* Params );
	};
}
