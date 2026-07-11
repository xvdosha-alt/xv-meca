#include "Includes.h"

using namespace MecchaCheatV;

HRESULT __stdcall Hooks::HkResizeBuffers(
	IDXGISwapChain* swapChain ,
	UINT bufferCount ,
	UINT width ,
	UINT height ,
	DXGI_FORMAT newFormat ,
	UINT swapChainFlags )
{
	LOG_HOOKS( "[DXGI] ResizeBuffers -> " , width , "x" , height , " format=" , static_cast<int>( newFormat ) );
	LOG_INFO( "[DXGI] ResizeBuffers -> " , width , "x" , height );

	if ( Menu::Initialized )
		ImGui_ImplDX11_InvalidateDeviceObjects();

	Renderer::ReleaseRenderTarget();

	const HRESULT result = hooking->OriginalResizeBuffers(
		swapChain ,
		bufferCount ,
		width ,
		height ,
		newFormat ,
		swapChainFlags );

	if ( FAILED( result ) )
		LOG_ERROR( "[DXGI] ResizeBuffers failed: 0x" , std::hex , static_cast<unsigned>( result ) );
	else
	{
		LOG_HOOKS( "[DXGI] ResizeBuffers OK" );
		Renderer::MarkPendingResize();
	}

	return result;
}
