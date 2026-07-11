#pragma once

#include "Includes.h"

namespace MecchaCheatV
{
	inline bool g_MenuKeyStates[256]{};

	inline bool IsMenuToggleVirtualKey( int key )
	{
		return key == VK_INSERT || key == VK_HOME || key == VK_RSHIFT || key == MenuToggleKey;
	}

	inline bool IsRightShiftMessage( WPARAM wParam , LPARAM lParam )
	{
		if ( wParam == VK_RSHIFT )
			return true;

		if ( wParam == VK_SHIFT || wParam == VK_LSHIFT || wParam == VK_RSHIFT )
		{
			const auto scanCode = static_cast<UINT>( ( lParam >> 16 ) & 0xFF );
			return scanCode == 0x36;
		}

		return false;
	}

	inline void ProcessMenuToggleKeys()
	{
		const int keys[] = { VK_INSERT , VK_HOME , VK_RSHIFT , MenuToggleKey };

		for ( const int key : keys )
		{
			if ( key == 0 )
				continue;

			const bool keyDown = ( GetAsyncKeyState( key ) & 0x8000 ) != 0;
			if ( keyDown && !g_MenuKeyStates[key] )
				menu.Toggle();

			g_MenuKeyStates[key] = keyDown;
		}
	}
}
