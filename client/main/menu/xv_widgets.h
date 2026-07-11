#pragma once

#include "Includes.h"

namespace MecchaCheatV::XvWidgets
{
	inline ImU32 AccentU32()
	{
		return ImGui::ColorConvertFloat4ToU32(accentPurple);
	}

	inline ImVec4 AccentHover()
	{
		return ImVec4(
			accentPurple.x * 0.85f + 0.15f ,
			accentPurple.y * 0.85f + 0.12f ,
			accentPurple.z * 0.85f + 0.2f ,
			1.f );
	}

	inline bool RenderToggleSwitch( const char* id , bool& value )
	{
		const ImVec2 size( 42.f * dpiScale , 22.f * dpiScale );
		const ImVec2 pos = ImGui::GetCursorScreenPos();

		if ( ImGui::InvisibleButton( id , size ) )
			value = !value;

		auto* draw = ImGui::GetWindowDrawList();
		const ImU32 bg = value ? AccentU32() : IM_COL32( 48 , 48 , 58 , 255 );
		draw->AddRectFilled( pos , pos + size , bg , size.y * 0.5f );

		const float knobRadius = size.y * 0.5f - 3.f;
		const float knobX = value ? pos.x + size.x - knobRadius - 4.f : pos.x + knobRadius + 4.f;
		draw->AddCircleFilled( ImVec2( knobX , pos.y + size.y * 0.5f ) , knobRadius , IM_COL32( 255 , 255 , 255 , 255 ) );

		return ImGui::IsItemClicked();
	}

	inline void RenderKeyChip( const char* label )
	{
		ImGui::PushStyleColor( ImGuiCol_Button , ImVec4( 0.14f , 0.14f , 0.17f , 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered , ImVec4( 0.18f , 0.16f , 0.24f , 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_Text , ImVec4( 0.82f , 0.82f , 0.9f , 1.f ) );
		ImGui::SmallButton( label );
		ImGui::PopStyleColor( 3 );
	}

	inline bool BeginFeatureCard( const char* id , const char* title , bool& enabled )
	{
		ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding , 12.f * dpiScale );
		ImGui::PushStyleColor( ImGuiCol_ChildBg , ImVec4( 0.085f , 0.085f , 0.105f , 1.f ) );
		ImGui::PushStyleColor(
			ImGuiCol_Border ,
			ImVec4( accentPurple.x * 0.35f , accentPurple.y * 0.35f , accentPurple.z * 0.35f , 0.35f ) );
		ImGui::BeginChild( id , ImVec2( -1.f , 0.f ) , ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY );

		ImGui::TextColored( accentPurple , "%s" , title );
		ImGui::SameLine( ImGui::GetContentRegionAvail().x - 48.f * dpiScale );
		RenderToggleSwitch( ( std::string( "##toggle_" ) + id ).c_str() , enabled );

		ImGui::Separator();
		ImGui::Spacing();

		return true;
	}

	inline void EndFeatureCard()
	{
		const ImVec2 cardPos = ImGui::GetWindowPos();
		const ImVec2 cardSize = ImGui::GetWindowSize();
		auto* draw = ImGui::GetWindowDrawList();

		draw->AddRectFilledMultiColor(
			cardPos ,
			cardPos + ImVec2( 4.f * dpiScale , cardSize.y ) ,
			AccentU32() ,
			ImGui::ColorConvertFloat4ToU32( accentPurpleLight ) ,
			ImGui::ColorConvertFloat4ToU32( accentPurpleLight ) ,
			AccentU32() );

		ImGui::EndChild();
		ImGui::PopStyleColor( 2 );
		ImGui::PopStyleVar();
	}

	inline bool RenderNavButton( const char* label , bool selected , const ImVec2& size )
	{
		ImGui::PushStyleColor( ImGuiCol_Button , selected ? accentPurpleDark : ImVec4( 0.f , 0.f , 0.f , 0.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered , selected ? accentPurple : ImVec4( 0.15f , 0.14f , 0.18f , 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive , accentPurpleLight );
		ImGui::PushStyleColor( ImGuiCol_Text , selected ? ImVec4( 1 , 1 , 1 , 1 ) : ImVec4( 0.75f , 0.75f , 0.85f , 1.f ) );

		const bool clicked = ImGui::Button( label , size );

		if ( selected )
		{
			const ImVec2 min = ImGui::GetItemRectMin();
			const ImVec2 max = ImGui::GetItemRectMax();
			ImGui::GetWindowDrawList()->AddLine(
				ImVec2( min.x + 8.f , max.y + 2.f ) ,
				ImVec2( max.x - 8.f , max.y + 2.f ) ,
				AccentU32() ,
				2.f );
		}

		ImGui::PopStyleColor( 4 );
		return clicked;
	}

	inline bool RenderCategoryTile( const char* icon , const char* label , const ImVec2& size )
	{
		const bool clicked = ImGui::Button( ( std::string( "##tile_" ) + label ).c_str() , size );
		const ImVec2 pos = ImGui::GetItemRectMin();
		const ImVec2 itemSize = ImGui::GetItemRectSize();
		const ImVec2 center( pos.x + itemSize.x * 0.5f , pos.y + itemSize.y * 0.5f );
		auto* draw = ImGui::GetWindowDrawList();

		draw->AddRectFilled( pos , pos + itemSize , IM_COL32( 18 , 16 , 26 , 220 ) , 12.f );
		draw->AddRect(
			pos ,
			pos + itemSize ,
			ImGui::ColorConvertFloat4ToU32( ImVec4( accentPurple.x , accentPurple.y , accentPurple.z , 0.35f ) ) ,
			12.f ,
			0 ,
			1.5f );

		ImGui::PushFont( ImGui::GetIO().Fonts->Fonts[2] );
		const ImVec2 iconSize = ImGui::CalcTextSize( icon );
		draw->AddText(
			ImVec2( center.x - iconSize.x * 0.5f , center.y - iconSize.y * 0.5f - 8.f * dpiScale ) ,
			AccentU32() ,
			icon );
		ImGui::PopFont();

		const ImVec2 textSize = ImGui::CalcTextSize( label );
		draw->AddText(
			ImVec2( center.x - textSize.x * 0.5f , pos.y + itemSize.y - textSize.y - 10.f * dpiScale ) ,
			IM_COL32( 230 , 230 , 245 , 255 ) ,
			label );

		return clicked;
	}
}
