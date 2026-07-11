#pragma once

#include "imgui/imgui.h"

namespace MecchaCheatV::VisualDraw
{
	bool IsWebOnly();
	void AddLine( float x1 , float y1 , float x2 , float y2 , ImU32 color , float thickness );
	void AddRect( float x1 , float y1 , float x2 , float y2 , ImU32 color , float thickness , float rounding = 0.f );
	void AddText( float x , float y , ImU32 color , const char* text , float fontSize );
	ImVec2 MeasureText( const char* text , float fontSize );
}
