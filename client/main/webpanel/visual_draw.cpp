#include "visual_draw.h"

#include <cstring>

#include "../features/features.h"
#include "overlay_canvas.h"

namespace MecchaCheatV::VisualDraw
{
	bool IsWebOnly()
	{
		return GET_FEATURE_HANDLER() && GET_FEATURE_HANDLER()->IsWebOnlyEnabled();
	}

	void AddLine( float x1 , float y1 , float x2 , float y2 , ImU32 color , float thickness )
	{
		if ( IsWebOnly() )
		{
			OverlayCanvas::AddLine( x1 , y1 , x2 , y2 , color , thickness );
			return;
		}

		if ( ImDrawList* drawList = ImGui::GetBackgroundDrawList() )
			drawList->AddLine( ImVec2( x1 , y1 ) , ImVec2( x2 , y2 ) , color , thickness );
	}

	void AddRect( float x1 , float y1 , float x2 , float y2 , ImU32 color , float thickness , float rounding )
	{
		if ( IsWebOnly() )
		{
			OverlayCanvas::AddRect( x1 , y1 , x2 , y2 , color , thickness , rounding );
			return;
		}

		if ( ImDrawList* drawList = ImGui::GetBackgroundDrawList() )
			drawList->AddRect( ImVec2( x1 , y1 ) , ImVec2( x2 , y2 ) , color , rounding , 0 , thickness );
	}

	void AddText( float x , float y , ImU32 color , const char* text , float fontSize )
	{
		if ( !text )
			return;

		if ( IsWebOnly() )
		{
			OverlayCanvas::AddText( x , y , color , text , fontSize );
			return;
		}

		if ( ImDrawList* drawList = ImGui::GetBackgroundDrawList() )
		{
			ImFont* font = ImGui::GetFont();
			drawList->AddText( font , fontSize , ImVec2( x , y ) , color , text );
		}
	}

	ImVec2 MeasureText( const char* text , float fontSize )
	{
		if ( !text )
			return ImVec2( 0.f , 0.f );

		if ( IsWebOnly() )
		{
			const float width = static_cast<float>( strlen( text ) ) * fontSize * 0.55f;
			return ImVec2( width , fontSize );
		}

		return ImGui::CalcTextSize( text );
	}
}
