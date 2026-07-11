#pragma once
#include "Includes.h"
#include "../res/fonts/DefFont.hpp"
#include "../res/fonts/HeadFont.hpp"
#include "../res/fonts/VCustomFont.hpp"

ImVec4 ToLinear(ImVec4 c)
{
    return ImVec4(
        powf(c.x, 2.2f),
        powf(c.y, 2.2f),
        powf(c.z, 2.2f),
        c.w
    );
}

inline void SetMenuDefaultStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();
    const float roundness = 8.f;

    style.WindowPadding = ImVec2( 0 , 0 );
    style.FramePadding = ImVec2( 8.f , 6.f );
    style.ItemSpacing = ImVec2( 8.f , 6.f );
    style.ItemInnerSpacing = ImVec2( 6.f , 4.f );
    style.IndentSpacing = 12.f;
    style.ScrollbarSize = 10.f;
    style.GrabMinSize = 12.f;

    style.WindowBorderSize = 0.f;
    style.ChildBorderSize = 1.f;
    style.PopupBorderSize = 1.f;
    style.FrameBorderSize = 0.f;
    style.TabBorderSize = 0.f;

    style.WindowRounding = roundness;
    style.ChildRounding = roundness;
    style.FrameRounding = roundness;
    style.PopupRounding = roundness;
    style.ScrollbarRounding = roundness;
    style.GrabRounding = roundness;
    style.TabRounding = roundness;

    ImVec4* colors = style.Colors;
    const ImVec4 accentFrame(
        accentPurple.x * 0.22f + 0.12f ,
        accentPurple.y * 0.22f + 0.1f ,
        accentPurple.z * 0.22f + 0.16f ,
        1.f );
    const ImVec4 accentFrameHover(
        accentPurple.x * 0.3f + 0.14f ,
        accentPurple.y * 0.3f + 0.12f ,
        accentPurple.z * 0.3f + 0.2f ,
        1.f );
    const ImVec4 accentHeader(
        accentPurple.x * 0.34f + 0.12f ,
        accentPurple.y * 0.34f + 0.1f ,
        accentPurple.z * 0.34f + 0.2f ,
        1.f );

    colors[ImGuiCol_Text] = ImVec4( 0.93f , 0.93f , 0.97f , 1.f );
    colors[ImGuiCol_TextDisabled] = ImVec4( 0.5f , 0.5f , 0.58f , 1.f );
    colors[ImGuiCol_WindowBg] = ImVec4( 0.055f , 0.055f , 0.065f , 0.98f );
    colors[ImGuiCol_ChildBg] = ImVec4( 0.09f , 0.09f , 0.11f , 1.f );
    colors[ImGuiCol_PopupBg] = ImVec4( 0.1f , 0.1f , 0.13f , 0.98f );
    colors[ImGuiCol_Border] = ImVec4( accentPurple.x * 0.35f , accentPurple.y * 0.35f , accentPurple.z * 0.35f , 0.45f );
    colors[ImGuiCol_BorderShadow] = ImVec4( 0.f , 0.f , 0.f , 0.f );
    colors[ImGuiCol_FrameBg] = accentFrame;
    colors[ImGuiCol_FrameBgHovered] = accentFrameHover;
    colors[ImGuiCol_FrameBgActive] = ImVec4( accentPurple.x * 0.38f + 0.16f , accentPurple.y * 0.38f + 0.14f , accentPurple.z * 0.38f + 0.24f , 1.f );
    colors[ImGuiCol_TitleBg] = ImVec4( 0.07f , 0.07f , 0.09f , 1.f );
    colors[ImGuiCol_TitleBgActive] = ImVec4( 0.09f , 0.09f , 0.12f , 1.f );
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4( 0.07f , 0.07f , 0.09f , 1.f );
    colors[ImGuiCol_MenuBarBg] = ImVec4( 0.1f , 0.1f , 0.13f , 1.f );
    colors[ImGuiCol_ScrollbarBg] = ImVec4( 0.08f , 0.08f , 0.1f , 0.f );
    colors[ImGuiCol_ScrollbarGrab] = ImVec4( accentPurple.x * 0.55f , accentPurple.y * 0.55f , accentPurple.z * 0.55f , 1.f );
    colors[ImGuiCol_ScrollbarGrabHovered] = accentPurple;
    colors[ImGuiCol_ScrollbarGrabActive] = accentPurpleLight;
    colors[ImGuiCol_CheckMark] = accentPurple;
    colors[ImGuiCol_SliderGrab] = accentPurple;
    colors[ImGuiCol_SliderGrabActive] = accentPurpleLight;
    colors[ImGuiCol_Button] = ImVec4( 0.15f , 0.14f , 0.18f , 1.f );
    colors[ImGuiCol_ButtonHovered] = ImVec4( accentPurple.x * 0.46f + 0.14f , accentPurple.y * 0.46f + 0.12f , accentPurple.z * 0.46f + 0.24f , 1.f );
    colors[ImGuiCol_ButtonActive] = accentPurpleLight;
    colors[ImGuiCol_Header] = accentHeader;
    colors[ImGuiCol_HeaderHovered] = ImVec4( accentPurple.x * 0.46f + 0.14f , accentPurple.y * 0.46f + 0.12f , accentPurple.z * 0.46f + 0.24f , 1.f );
    colors[ImGuiCol_HeaderActive] = accentPurpleLight;
    colors[ImGuiCol_Separator] = ImVec4( accentPurple.x * 0.35f , accentPurple.y * 0.35f , accentPurple.z * 0.35f , 0.55f );
    colors[ImGuiCol_SeparatorHovered] = ImVec4( accentPurple.x , accentPurple.y , accentPurple.z , 0.75f );
    colors[ImGuiCol_SeparatorActive] = accentPurpleLight;
    colors[ImGuiCol_ResizeGrip] = ImVec4( accentPurple.x , accentPurple.y , accentPurple.z , 0.2f );
    colors[ImGuiCol_ResizeGripHovered] = ImVec4( accentPurple.x , accentPurple.y , accentPurple.z , 0.45f );
    colors[ImGuiCol_ResizeGripActive] = ImVec4( accentPurple.x , accentPurple.y , accentPurple.z , 0.7f );
    colors[ImGuiCol_Tab] = ImVec4( 0.14f , 0.13f , 0.17f , 1.f );
    colors[ImGuiCol_TabHovered] = ImVec4( accentPurple.x * 0.46f + 0.14f , accentPurple.y * 0.46f + 0.12f , accentPurple.z * 0.46f + 0.24f , 1.f );
    colors[ImGuiCol_TabActive] = accentHeader;
    colors[ImGuiCol_TabUnfocused] = ImVec4( 0.11f , 0.1f , 0.14f , 1.f );
    colors[ImGuiCol_TabUnfocusedActive] = accentFrame;
    colors[ImGuiCol_TextSelectedBg] = ImVec4( accentPurple.x , accentPurple.y , accentPurple.z , 0.32f );
    colors[ImGuiCol_NavHighlight] = ImVec4( accentPurple.x * 0.85f + 0.15f , accentPurple.y * 0.85f + 0.12f , accentPurple.z * 0.85f + 0.2f , 1.f );
}

inline void InitFonts()
{
    ImGuiIO& io = ImGui::GetIO();

    ImFontConfig fontConfig;

    
    io.Fonts->AddFontFromMemoryCompressedTTF(
        DefFont_compressed_data,
        DefFont_compressed_size,
        19.f,
        &fontConfig
    );
    
    io.Fonts->AddFontFromMemoryCompressedTTF(
        HeadFont_compressed_data,
        HeadFont_compressed_size,
        20.f,
        &fontConfig
    );
    
    io.Fonts->AddFontFromMemoryCompressedTTF(
        VCustom_compressed_data,
        VCustom_compressed_size,
        35.f,
        &fontConfig
    );
    
    io.Fonts->AddFontFromMemoryCompressedTTF(
        DefFont_compressed_data,
        DefFont_compressed_size,
        25.f,
        &fontConfig
    );
    
    io.Fonts->AddFontFromMemoryCompressedTTF(
        VCustom_compressed_data,
        VCustom_compressed_size,
        15.f,
        &fontConfig
    );
    
    io.Fonts->AddFontFromMemoryCompressedTTF(
        DefFont_compressed_data,
        DefFont_compressed_size,
        20.f,
        &fontConfig
    );
}