#include "renderer.h"

using namespace MecchaCheatV;

Renderer::Renderer()
{
	renderer = this;
}

Renderer::~Renderer()
{
	renderer = nullptr;
}

bool Renderer::GetSwapChain(IDXGISwapChain** swapChain, ID3D11Device** device) const
{
	WNDCLASSEX wc{ 0 };
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = DefWindowProc;
	wc.lpszClassName = TEXT("MecchaCheatV");
	wc.hInstance = GetModuleHandle(nullptr);

	if (!RegisterClassEx(&wc))
	{
		return false;
	}

	HWND hwnd = CreateWindowEx(
		0,
		wc.lpszClassName,
		TEXT(""),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		100,
		100,
		nullptr,
		nullptr,
		wc.hInstance,
		nullptr);

	if (!hwnd)
	{
		UnregisterClass(wc.lpszClassName, wc.hInstance);
		return false;
	}

	DXGI_SWAP_CHAIN_DESC description{};
	description.BufferCount = 1;
	description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	description.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	description.OutputWindow = hwnd;
	description.SampleDesc.Count = 1;
	description.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	description.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	description.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	description.Windowed = TRUE;

	D3D_FEATURE_LEVEL level;
	bool success = false;

	for (const auto& driverType : KDriverType)
	{
		const HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, driverType, nullptr, 0, nullptr, 0,
			D3D11_SDK_VERSION, &description, swapChain, device, &level,
			nullptr);

		if (SUCCEEDED(hr))
		{
			success = true;
			break;
		}
	}

	DestroyWindow(hwnd);
	UnregisterClass(wc.lpszClassName, wc.hInstance);

	return success;
}

Id3DPresent Renderer::GetPresent() const
{
	IDXGISwapChain* swapChain;
	ID3D11Device* device;

	if (GetSwapChain(&swapChain, &device))
	{
		void** vmt = *reinterpret_cast<void***>(swapChain);

		if (swapChain)
		{
			swapChain->Release();
			swapChain = nullptr;
		}
		if (device)
		{
			device->Release();
			device = nullptr;
		}

		return reinterpret_cast<Id3DPresent>(vmt[8]);
	}

	return nullptr;
}

Id3DResizeBuffers Renderer::GetResizeBuffers() const
{
	IDXGISwapChain* swapChain = nullptr;
	ID3D11Device* device = nullptr;

	if ( !GetSwapChain( &swapChain , &device ) )
		return nullptr;

	void** vmt = *reinterpret_cast<void***>( swapChain );

	if ( swapChain )
		swapChain->Release();
	if ( device )
		device->Release();

	return reinterpret_cast<Id3DResizeBuffers>( vmt[13] );
}

void Renderer::ReleaseRenderTarget()
{
	if ( Context )
	{
		Context->OMSetRenderTargets( 0 , nullptr , nullptr );
		Context->Flush();
	}

	if ( TargetView )
	{
		TargetView->Release();
		TargetView = nullptr;
	}

	BufferWidth = 0;
	BufferHeight = 0;
	DeviceObjectsReady.store( false , std::memory_order_release );
}

void Renderer::ReleaseOverlayResources()
{
	ReleaseRenderTarget();

	if ( Menu::Initialized )
	{
		LOG_HOOKS( "[DXGI] Releasing overlay for shutdown" );
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
		Menu::Initialized = false;
	}

	PendingResize.store( false , std::memory_order_release );
}

void Renderer::MarkPendingResize()
{
	PendingResize.store( true , std::memory_order_release );
	DeviceObjectsReady.store( false , std::memory_order_release );
}

bool Renderer::ConsumePendingResize()
{
	return PendingResize.exchange( false , std::memory_order_acq_rel );
}

bool Renderer::CanRenderOverlay()
{
	return TargetView != nullptr && DeviceObjectsReady.load( std::memory_order_acquire );
}

bool Renderer::RecreateOverlayAfterResize( IDXGISwapChain* swapChain )
{
	if ( !swapChain || !Device )
		return false;

	if ( Menu::Initialized )
		ImGui_ImplDX11_InvalidateDeviceObjects();

	ReleaseRenderTarget();

	if ( !CreateOverlayRenderTarget( swapChain ) )
		return false;

	if ( Menu::Initialized && !ImGui_ImplDX11_CreateDeviceObjects() )
	{
		ReleaseRenderTarget();
		return false;
	}

	DeviceObjectsReady.store( true , std::memory_order_release );
	return true;
}

bool Renderer::CreateOverlayRenderTarget( IDXGISwapChain* swapChain )
{
	if ( !swapChain || !Device )
		return false;

	DXGI_SWAP_CHAIN_DESC description{};
	if ( FAILED( swapChain->GetDesc( &description ) ) )
		return false;

	ID3D11Texture2D* backBuffer = nullptr;
	if ( FAILED( swapChain->GetBuffer( 0 , __uuidof( ID3D11Texture2D ) , reinterpret_cast<void**>( &backBuffer ) ) ) )
	{
		LOG_ERROR( "[DXGI] GetBuffer failed during RTV create" );
		return false;
	}

	const HRESULT createResult = Device->CreateRenderTargetView( backBuffer , nullptr , &TargetView );
	backBuffer->Release();

	if ( FAILED( createResult ) )
	{
		LOG_ERROR( "[DXGI] CreateRenderTargetView failed: 0x" , std::hex , static_cast<unsigned>( createResult ) );
		TargetView = nullptr;
		return false;
	}

	BufferWidth = description.BufferDesc.Width;
	BufferHeight = description.BufferDesc.Height;
	LOG_HOOKS( "[DXGI] Created RTV " , BufferWidth , "x" , BufferHeight );
	return true;
}