#pragma once
#include "Includes.h"

using Id3DPresent = HRESULT( __stdcall* )( IDXGISwapChain* this_ , UINT sync_ , UINT flags_ );
using Id3DResizeBuffers = HRESULT( __stdcall* )(
	IDXGISwapChain* swapChain ,
	UINT bufferCount ,
	UINT width ,
	UINT height ,
	DXGI_FORMAT newFormat ,
	UINT swapChainFlags );

namespace MecchaCheatV
{
	class Hooking
	{
		friend Hooks;

	public:
		explicit Hooking();
		~Hooking();

		void AddHook( const std::string& functionName , PVOID* originalFunction , PVOID hookFunction );
		void ApplyHooks() const;
		void RemoveHooks() const;
		Id3DPresent OriginalPresent{};
		Id3DResizeBuffers OriginalResizeBuffers{};
		WNDPROC OriginalWndproc{};

	private:
		std::vector<std::tuple<std::string, PVOID*, PVOID>> hooks_;
	};

	inline Hooking* hooking{};
}

#define AHK(orig, hook) MecchaCheatV::hooking->AddHook(#orig, reinterpret_cast<PVOID*>(&(orig)), reinterpret_cast<PVOID>(hook))
#define AHKA(name) AHK(SDK::name, Hooks::hk##name)