#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace MecchaCheatV::OverlayCanvas
{
	struct LineCmd
	{
		float x1{}, y1{}, x2{}, y2{}, thickness{};
		uint32_t color{};
	};

	struct RectCmd
	{
		float x1{}, y1{}, x2{}, y2{}, thickness{}, rounding{};
		uint32_t color{};
	};

	struct TextCmd
	{
		float x{}, y{}, size{};
		uint32_t color{};
		std::string text;
	};

	void BeginFrame( float width , float height );
	void AddLine( float x1 , float y1 , float x2 , float y2 , uint32_t color , float thickness );
	void AddRect( float x1 , float y1 , float x2 , float y2 , uint32_t color , float thickness , float rounding = 0.f );
	void AddText( float x , float y , uint32_t color , const char* text , float size );
	std::string BuildFrameJson();
}
