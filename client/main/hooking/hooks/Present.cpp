#include "Includes.h"

using namespace MecchaCheatV;

namespace
{
	bool g_WndProcHooked = false;
}

HRESULT __stdcall Hooks::HkPresent( IDXGISwapChain* pSwapChain , UINT SyncInterval , UINT Flags )
{
	if ( !menu.Initialized )
	{
		if ( SUCCEEDED( pSwapChain->GetDevice( __uuidof( ID3D11Device ) , reinterpret_cast<void**>( &Renderer::Device ) ) ) )
		{
			LOG_INFO( "[DXGI] Initializing overlay on Present" );

			Renderer::Swapchain = pSwapChain;
			Renderer::Device->GetImmediateContext( &Renderer::Context );

			DXGI_SWAP_CHAIN_DESC description{};
			pSwapChain->GetDesc( &description );
			Renderer::Window = description.OutputWindow;

			ImGui::CreateContext();
			ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
			ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
			ImGui_ImplWin32_Init( Renderer::Window );
			ImGui_ImplDX11_Init( Renderer::Device , Renderer::Context );
			ImGui::GetIO().FontGlobalScale = dpiScale;

			if ( !Renderer::CreateOverlayRenderTarget( pSwapChain ) )
			{
				LOG_ERROR( "[DXGI] Overlay init failed, skipping frame" );
				return hooking->OriginalPresent( pSwapChain , SyncInterval , Flags );
			}

			if ( !ImGui_ImplDX11_CreateDeviceObjects() )
			{
				LOG_ERROR( "[DXGI] ImGui device objects failed during init" );
				Renderer::ReleaseRenderTarget();
				return hooking->OriginalPresent( pSwapChain , SyncInterval , Flags );
			}

			Renderer::DeviceObjectsReady.store( true , std::memory_order_release );

			if ( !g_WndProcHooked && Renderer::Window )
			{
				const auto previousWndProc = reinterpret_cast<WNDPROC>( SetWindowLongPtr(
					Renderer::Window ,
					GWLP_WNDPROC ,
					reinterpret_cast<LONG_PTR>( HkWndProc ) ) );

				if ( previousWndProc && previousWndProc != HkWndProc )
					hooking->OriginalWndproc = previousWndProc;

				g_WndProcHooked = true;
			}

			Menu::Initialize();
		}
		else
		{
			return hooking->OriginalPresent( pSwapChain , SyncInterval , Flags );
		}
	}

	if ( !Renderer::Context || !Renderer::Device )
		return hooking->OriginalPresent( pSwapChain , SyncInterval , Flags );

	const bool webOnly = GET_FEATURE_HANDLER() && GET_FEATURE_HANDLER()->IsWebOnlyEnabled();

	if ( webOnly )
	{
		if ( Renderer::ConsumePendingResize() )
		{
			DXGI_SWAP_CHAIN_DESC resizeDesc{};
			if ( SUCCEEDED( pSwapChain->GetDesc( &resizeDesc ) ) )
			{
				Renderer::BufferWidth = resizeDesc.BufferDesc.Width;
				Renderer::BufferHeight = resizeDesc.BufferDesc.Height;
			}
		}
	}
	else if ( Renderer::ConsumePendingResize() || !Renderer::TargetView )
	{
		if ( !Renderer::RecreateOverlayAfterResize( pSwapChain ) )
			return hooking->OriginalPresent( pSwapChain , SyncInterval , Flags );
	}

	DXGI_SWAP_CHAIN_DESC frameDesc{};
	pSwapChain->GetDesc( &frameDesc );
	float frameW = static_cast<float>( frameDesc.BufferDesc.Width );
	float frameH = static_cast<float>( frameDesc.BufferDesc.Height );
	if ( frameW <= 0.f )
		frameW = static_cast<float>( Renderer::BufferWidth );
	if ( frameH <= 0.f )
		frameH = static_cast<float>( Renderer::BufferHeight );

	if ( webOnly )
	{
		Renderer::BufferWidth = static_cast<UINT>( frameW );
		Renderer::BufferHeight = static_cast<UINT>( frameH );
		Globals::WebEspFrameDone.store( false , std::memory_order_release );
		ImGui::GetIO().DisplaySize = ImVec2( frameW , frameH );
		return hooking->OriginalPresent( pSwapChain , SyncInterval , Flags );
	}

	if ( !Renderer::CanRenderOverlay() )
		return hooking->OriginalPresent( pSwapChain , SyncInterval , Flags );

	ID3D11RenderTargetView* originalTarget = nullptr;
	ID3D11DepthStencilView* originalDepth = nullptr;
	Renderer::Context->OMGetRenderTargets( 1 , &originalTarget , &originalDepth );
	Renderer::Context->OMSetRenderTargets( 1 , &Renderer::TargetView , nullptr );

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if ( GET_FEATURE_HANDLER() )
		GET_FEATURE_HANDLER()->RenderAll();

	Notifications::RenderNotifications();

	ImGui::EndFrame();
	ImGui::Render();

	ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData() );
	Renderer::Context->OMSetRenderTargets( 1 , &originalTarget , originalDepth );
	if ( originalTarget )
		originalTarget->Release();
	if ( originalDepth )
		originalDepth->Release();

	return hooking->OriginalPresent( pSwapChain , SyncInterval , Flags );
}
