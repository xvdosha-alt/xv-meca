#pragma once
#include "Includes.h"
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#include <atomic>

namespace MecchaCheatV
{
	using Id3DPresent = HRESULT(__stdcall*)(IDXGISwapChain* this_, UINT sync_, UINT flags_);
	using Id3DResizeBuffers = HRESULT(__stdcall*)(
		IDXGISwapChain* swapChain ,
		UINT bufferCount ,
		UINT width ,
		UINT height ,
		DXGI_FORMAT newFormat ,
		UINT swapChainFlags );

	class Renderer
	{
		const D3D_DRIVER_TYPE KDriverType[4] = {
			D3D_DRIVER_TYPE_REFERENCE, D3D_DRIVER_TYPE_SOFTWARE, D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP
		};

	public:
		explicit Renderer();
		~Renderer();
		bool GetSwapChain(IDXGISwapChain** swapChain, ID3D11Device** device) const;
		Id3DPresent GetPresent() const;
		Id3DResizeBuffers GetResizeBuffers() const;
		static void ReleaseRenderTarget();
		static void ReleaseOverlayResources();
		static bool CreateOverlayRenderTarget(IDXGISwapChain* swapChain);
		static bool RecreateOverlayAfterResize(IDXGISwapChain* swapChain);
		static void MarkPendingResize();
		static bool ConsumePendingResize();
		static bool CanRenderOverlay();

		static inline std::atomic<bool> PendingResize{ false };
		static inline std::atomic<bool> DeviceObjectsReady{ false };

		static inline IDXGISwapChain* Swapchain;
		static inline HWND Window;
		static inline ID3D11Texture2D* HiddenBackBuffer = nullptr;
		static inline ID3D11RenderTargetView* HiddenTargetView = nullptr;
		static inline ID3D11Device* Device;
		static inline ID3D11DeviceContext* Context;
		static inline ID3D11RenderTargetView* TargetView;
		static inline UINT BufferWidth = 0;
		static inline UINT BufferHeight = 0;
	};

	inline Renderer* renderer{};
}