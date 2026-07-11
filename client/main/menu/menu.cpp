#include "menu.h"
#include "styles.h"
#include "xv_widgets.h"
#include "menu_keys.h"
#include "../features/features_includes.h"

using namespace MecchaCheatV;
using namespace MecchaCheatV::XvWidgets;

namespace
{
	constexpr float kMenuWidth = 920.f;
	constexpr float kMenuHeight = 640.f;
	constexpr float kTopNavHeight = 52.f;

	void RenderTopNav()
	{
		const float navWidth = ImGui::GetContentRegionAvail().x;

		ImGui::PushStyleColor( ImGuiCol_ChildBg , ImVec4( 0.055f , 0.055f , 0.065f , 1.f ) );
		ImGui::BeginChild( "##xv_TopNav" , ImVec2( navWidth , kTopNavHeight * dpiScale ) , ImGuiChildFlags_Borders );

		ImGui::SetCursorPosY( 8.f * dpiScale );
		ImGui::PushFont( ImGui::GetIO().Fonts->Fonts[1] );
		ImGui::TextColored( accentPurple , CheatName );
		ImGui::PopFont();

		const ImVec2 buttonSize( 110.f * dpiScale , 34.f * dpiScale );
		float x = navWidth - buttonSize.x * 3.f - 36.f * dpiScale;
		ImGui::SameLine( x );

		if ( RenderNavButton( "Features##nav0" , menu.currentTab == 0 , buttonSize ) )
			menu.currentTab = 0;
		ImGui::SameLine();
		if ( RenderNavButton( "Settings##nav1" , menu.currentTab == 1 , buttonSize ) )
			menu.currentTab = 1;
		ImGui::SameLine();
		if ( RenderNavButton( "About##nav2" , menu.currentTab == 2 , buttonSize ) )
			menu.currentTab = 2;
		if ( IsDebugging )
		{
			ImGui::SameLine();
			if ( RenderNavButton( "TEST##nav3" , menu.currentTab == 3 , buttonSize ) )
				menu.currentTab = 3;
		}

		ImGui::EndChild();
		ImGui::PopStyleColor();
	}

	void RenderSettingsTab()
	{
		ImGui::BeginChild( "##xv_SettingsScroll" , ImVec2( 0 , 0 ) , ImGuiChildFlags_None , ImGuiWindowFlags_AlwaysVerticalScrollbar );
		ImGui::SetCursorPos( ImVec2( 20.f * dpiScale , 16.f * dpiScale ) );

		bool menuCardEnabled = true;
		if ( BeginFeatureCard( "##CardMenuSettings" , "Menu Settings" , menuCardEnabled ) )
		{
			ImGui::Text( "Menu keys" );
			RenderKeyChip( "INS" );
			ImGui::SameLine();
			RenderKeyChip( "HOME" );
			ImGui::SameLine();
			RenderKeyChip( "RSHIFT" );
			ImGui::SameLine();
			RenderKeyChip( Utils::getKeyName( MenuToggleKey ).c_str() );

			ImGui::Spacing();
			ImGui::Text( "Custom toggle key" );
			ImGui::PushStyleColor( ImGuiCol_Button , accentPurpleDark );
			ImGui::PushStyleColor( ImGuiCol_ButtonHovered , accentPurple );
			if ( ImGui::Button( "Set key" , ImVec2( 140.f * dpiScale , 28.f * dpiScale ) ) )
				ImGui::OpenPopup( "Set menu key" );
			ImGui::PopStyleColor( 2 );
			ImGui::SameLine();
			ImGui::TextColored( accentPurpleLight , "%s" , Utils::getKeyName( MenuToggleKey ).c_str() );

			if ( ImGui::BeginPopupModal( "Set menu key" , nullptr , ImGuiWindowFlags_AlwaysAutoResize ) )
			{
				ImGui::Text( "Press key (not INS / HOME / RSHIFT)" );
				ImGui::Separator();
				if ( ImGui::Button( "Cancel" ) )
					ImGui::CloseCurrentPopup();

				for ( int key = 0; key < 256; ++key )
				{
					if ( IsMenuToggleVirtualKey( key ) && key != MenuToggleKey )
						continue;

					if ( GetAsyncKeyState( key ) & 0x8000 )
					{
						MenuToggleKey = key;
						ImGui::CloseCurrentPopup();
						break;
					}
				}
				ImGui::EndPopup();
			}

			EndFeatureCard();
		}

		ImGui::Spacing();

		bool colorsCardEnabled = true;
		if ( BeginFeatureCard( "##CardColors" , "Theme Colors" , colorsCardEnabled ) )
		{
			bool changed = false;
			changed |= ImGui::ColorEdit4( "Accent" , (float*)&accentPurple );
			changed |= ImGui::ColorEdit4( "Accent Dark" , (float*)&accentPurpleDark );
			changed |= ImGui::ColorEdit4( "Accent Light" , (float*)&accentPurpleLight );
			changed |= ImGui::ColorEdit4( "Window BG" , (float*)&darkBg );
			changed |= ImGui::ColorEdit4( "Child BG" , (float*)&darkerBg );
			changed |= ImGui::ColorEdit4( "Card BG" , (float*)&cardBg );

			if ( ImGui::Button( "Save Colors" , ImVec2( 160.f * dpiScale , 30.f * dpiScale ) ) )
				{ }
			ImGui::SameLine();
			if ( ImGui::Button( "Reset Colors" , ImVec2( 140.f * dpiScale , 30.f * dpiScale ) ) )
			{
				ResetColors();
				SetMenuDefaultStyle();
			}

			(void)changed;

			EndFeatureCard();
		}

		ImGui::Spacing();

		bool toolsCardEnabled = true;
		if ( BeginFeatureCard( "##CardTools" , "Tools" , toolsCardEnabled ) )
		{
			if ( ImGui::Button( "Clear log cache" , ImVec2( 180.f * dpiScale , 30.f * dpiScale ) ) )
			{
				const std::string logsPath = Utils::GetCheatDirectory() + "\\logs";
				try
				{
					int deleted = 0;
					for ( const auto& entry : std::filesystem::directory_iterator( logsPath ) )
					{
						if ( entry.is_regular_file() && std::filesystem::remove( entry.path() ) )
							++deleted;
					}
					LOG_INFO( "Logs cleaned: " , deleted );
				}
				catch ( ... ) {}
			}
			EndFeatureCard();
		}

		ImGui::EndChild();
	}

	void RenderAboutTab()
	{
		ImGui::BeginChild( "##xv_AboutScroll" , ImVec2( 0 , 0 ) , ImGuiChildFlags_None , ImGuiWindowFlags_AlwaysVerticalScrollbar );
		ImGui::SetCursorPos( ImVec2( 20.f * dpiScale , 16.f * dpiScale ) );

		bool aboutCard = true;
		if ( BeginFeatureCard( "##CardAbout" , "Open Source" , aboutCard ) )
		{
			ImGui::TextColored( ImVec4( 0.6f , 0.8f , 1.f , 1.f ) , "GitHub Repository" );
			if ( ImGui::Button( "Open GitHub repository" ) )
				ShellExecuteA( 0 , "open" , GitHubRepository , 0 , 0 , SW_SHOW );

			ImGui::Spacing();
			ImGui::TextWrapped(
				"This project is open source and available under MIT License. Feel free to contribute!" );
			EndFeatureCard();
		}

		ImGui::Spacing();

		bool creditsCard = true;
		if ( BeginFeatureCard( "##CardCredits" , "Credits" , creditsCard ) )
		{
			ImGui::BulletText( "ViniLog" );
			ImGui::Spacing();
			ImGui::TextColored( ImVec4( 0.6f , 0.8f , 1.f , 1.f ) , "Special Thanks" );
			ImGui::TextWrapped( "Evelien (feature system), .gashopeless, crymore.vip" );
			ImGui::Spacing();
			ImGui::TextDisabled( "%s | MIT License" , Version.c_str() );
			EndFeatureCard();
		}

		ImGui::EndChild();
	}

	void RenderTestTab()
	{
		ImGui::BeginChild( "##xv_TestScroll" , ImVec2( 0 , 0 ) , true );
		if ( ImGui::Button( "Crash game" ) )
			*(int*)0 = 42;

		if ( ImGui::Button( "Test IsServer" ) )
		{
			const bool isServer = SDK::UKismetSystemLibrary::IsServer( SDK::UWorld::GetWorld() );
			LOG_INFO( isServer ? "yes" : "no" );
		}

		if ( ImGui::Button( "Call test" ) )
			ForTests = true;

		ImGui::EndChild();
	}
}

void Menu::Initialize()
{
	SetMenuDefaultStyle();
	InitFonts();
	Initialized = true;
}

void Menu::NewYear()
{
}

void Menu::Render()
{
	constexpr ImGuiWindowFlags windowFlags =
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse;

	ImGui::SetNextWindowSize( ImVec2( kMenuWidth * dpiScale , kMenuHeight * dpiScale ) , ImGuiCond_Once );
	ImGui::SetNextWindowBgAlpha( 1.f );

	ImGui::Begin( CheatName , nullptr , windowFlags );

	const ImVec2 windowPos = ImGui::GetWindowPos();
	const ImVec2 windowSize = ImGui::GetWindowSize();
	ImGui::GetWindowDrawList()->AddRectFilledMultiColor(
		windowPos ,
		windowPos + ImVec2( windowSize.x , 3.f * dpiScale ) ,
		AccentU32() ,
		ImGui::ColorConvertFloat4ToU32( accentPurpleLight ) ,
		ImGui::ColorConvertFloat4ToU32( accentPurpleLight ) ,
		AccentU32() );

	RenderTopNav();

	const float contentHeight = ImGui::GetContentRegionAvail().y;
	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding , ImVec2( 14.f * dpiScale , 10.f * dpiScale ) );
	ImGui::BeginChild( "##xv_ContentScroll" , ImVec2( -1.f , contentHeight ) , ImGuiChildFlags_None , ImGuiWindowFlags_AlwaysVerticalScrollbar );

	switch ( menu.currentTab )
	{
	case 0:
		if ( GET_FEATURE_HANDLER() )
			GET_FEATURE_HANDLER()->RenderMenu();
		else
			ImGui::TextColored( ImVec4( 1.f , 0.4f , 0.4f , 1.f ) , "No features..." );
		break;
	case 1:
		RenderSettingsTab();
		break;
	case 2:
		RenderAboutTab();
		break;
	case 3:
		if ( IsDebugging )
			RenderTestTab();
		break;
	default:
		break;
	}

	ImGui::EndChild();
	ImGui::PopStyleVar();

	ImGui::SetCursorPosY( windowSize.y - 42.f * dpiScale );
	ImGui::Separator();
	ImGui::PushStyleColor( ImGuiCol_Button , ImVec4( 0.45f , 0.12f , 0.12f , 0.85f ) );
	ImGui::PushStyleColor( ImGuiCol_ButtonHovered , ImVec4( 0.65f , 0.18f , 0.18f , 1.f ) );
	ImGui::PushStyleColor( ImGuiCol_ButtonActive , ImVec4( 0.8f , 0.22f , 0.22f , 1.f ) );
	if ( ImGui::Button( "Unload Cheat" , ImVec2( 160.f * dpiScale , 30.f * dpiScale ) ) )
	{
		menu.Toggle();
		CheatWork = false;
	}
	ImGui::PopStyleColor( 3 );

	ImGui::End();
}

void Menu::Toggle()
{
}
