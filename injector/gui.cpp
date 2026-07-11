#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <gdiplus.h>

#include <string>

#include "inject.hpp"

#pragma comment( lib , "gdiplus.lib" )
#pragma comment( lib , "dwmapi.lib" )
#pragma comment( lib , "version.lib" )

namespace
{
	constexpr wchar_t kWindowClassName[] = L"MecchaCheatVInjector";
	constexpr int kWindowWidth = 500;
	constexpr int kWindowHeight = 380;

	constexpr UINT WM_APP_REFRESH = WM_APP + 1;
	constexpr UINT WM_APP_INJECT_DONE = WM_APP + 2;
	constexpr UINT_PTR kTimerId = 1;

	struct GuiState
	{
		HWND Window = nullptr;
		Gdiplus::GdiplusStartupInput GdiplusInput{};
		ULONG_PTR GdiplusToken = 0;

		InjectState State{};
		bool PayloadReady = false;
		bool AutoReinject = true;
		bool InjectInProgress = false;
		bool GameLaunchStarted = false;
		bool InjectButtonHover = false;
		bool AutoToggleHover = false;

		RECT InjectButtonRect{};
		RECT AutoToggleRect{};

		std::wstring FooterText = L"Payload: waiting...";
	};

	GuiState g_Gui{};

	auto SetDarkTitleBar( HWND window ) -> void
	{
		const BOOL useDarkMode = TRUE;
		DwmSetWindowAttribute( window , 20 , &useDarkMode , sizeof( useDarkMode ) );
	}

	auto Color( BYTE r , BYTE g , BYTE b , BYTE a = 255 ) -> Gdiplus::Color
	{
		return Gdiplus::Color( a , r , g , b );
	}

	auto FillRoundedRect( Gdiplus::Graphics& graphics , const Gdiplus::RectF& rect , float radius , const Gdiplus::Brush& brush ) -> void
	{
		Gdiplus::GraphicsPath path;
		const float diameter = radius * 2.f;

		path.AddArc( rect.X , rect.Y , diameter , diameter , 180.f , 90.f );
		path.AddArc( rect.GetRight() - diameter , rect.Y , diameter , diameter , 270.f , 90.f );
		path.AddArc( rect.GetRight() - diameter , rect.GetBottom() - diameter , diameter , diameter , 0.f , 90.f );
		path.AddArc( rect.X , rect.GetBottom() - diameter , diameter , diameter , 90.f , 90.f );
		path.CloseFigure();

		graphics.FillPath( &brush , &path );
	}

	auto DrawCenteredText(
		Gdiplus::Graphics& graphics ,
		const wchar_t* text ,
		const Gdiplus::Font& font ,
		const Gdiplus::Brush& brush ,
		const Gdiplus::RectF& rect ) -> void
	{
		Gdiplus::StringFormat format;
		format.SetAlignment( Gdiplus::StringAlignmentCenter );
		format.SetLineAlignment( Gdiplus::StringAlignmentCenter );
		graphics.DrawString( text , -1 , &font , rect , &format , &brush );
	}

	auto DrawGlowOrb( Gdiplus::Graphics& graphics , float x , float y , float size , Gdiplus::Color inner , Gdiplus::Color outer ) -> void
	{
		Gdiplus::GraphicsPath path;
		path.AddEllipse( x - size * 0.5f , y - size * 0.5f , size , size );

		Gdiplus::PathGradientBrush brush( &path );
		int count = 1;
		brush.SetCenterColor( inner );
		brush.SetSurroundColors( &outer , &count );
		graphics.FillPath( &brush , &path );
	}

	auto UpdateLayout( RECT clientRect ) -> void
	{
		const int width = clientRect.right - clientRect.left;
		const int height = clientRect.bottom - clientRect.top;
		const int buttonWidth = 220;
		const int buttonHeight = 48;

		g_Gui.InjectButtonRect = {
			( width - buttonWidth ) / 2 ,
			height - 118 ,
			( width + buttonWidth ) / 2 ,
			height - 118 + buttonHeight ,
		};

		g_Gui.AutoToggleRect = {
			( width - 220 ) / 2 ,
			height - 58 ,
			( width + 220 ) / 2 ,
			height - 34 ,
		};
	}

	auto PointInRect( const RECT& rect , int x , int y ) -> bool
	{
		return x >= rect.left && x < rect.right && y >= rect.top && y < rect.bottom;
	}

	auto PaintWindow( HDC deviceContext , const RECT& clientRect ) -> void
	{
		const int width = clientRect.right - clientRect.left;
		const int height = clientRect.bottom - clientRect.top;

		HDC memoryDc = CreateCompatibleDC( deviceContext );
		HBITMAP bitmap = CreateCompatibleBitmap( deviceContext , width , height );
		const HGDIOBJ oldBitmap = SelectObject( memoryDc , bitmap );

		{
			Gdiplus::Graphics graphics( memoryDc );
			graphics.SetSmoothingMode( Gdiplus::SmoothingModeAntiAlias );
			graphics.SetTextRenderingHint( Gdiplus::TextRenderingHintClearTypeGridFit );

			Gdiplus::RectF backgroundRect( 0.f , 0.f , static_cast<Gdiplus::REAL>( width ) , static_cast<Gdiplus::REAL>( height ) );
			Gdiplus::LinearGradientBrush backgroundBrush(
				Gdiplus::PointF( 0.f , 0.f ) ,
				Gdiplus::PointF( static_cast<Gdiplus::REAL>( width ) , static_cast<Gdiplus::REAL>( height ) ) ,
				Color( 8 , 6 , 14 ) ,
				Color( 16 , 12 , 28 ) );
			graphics.FillRectangle( &backgroundBrush , backgroundRect );

			DrawGlowOrb( graphics , width * 0.18f , height * 0.22f , 180.f , Color( 120 , 50 , 200 , 70 ) , Color( 120 , 50 , 200 , 0 ) );
			DrawGlowOrb( graphics , width * 0.82f , height * 0.28f , 150.f , Color( 70 , 120 , 255 , 55 ) , Color( 70 , 120 , 255 , 0 ) );
			DrawGlowOrb( graphics , width * 0.55f , height * 0.78f , 200.f , Color( 168 , 85 , 247 , 45 ) , Color( 168 , 85 , 247 , 0 ) );

			Gdiplus::FontFamily titleFamily( L"Segoe UI" );
			Gdiplus::Font titleFont( &titleFamily , 30.f , Gdiplus::FontStyleBold , Gdiplus::UnitPixel );
			Gdiplus::Font subtitleFont( &titleFamily , 14.f , Gdiplus::FontStyleRegular , Gdiplus::UnitPixel );
			Gdiplus::Font statusFont( &titleFamily , 15.f , Gdiplus::FontStyleRegular , Gdiplus::UnitPixel );
			Gdiplus::Font smallFont( &titleFamily , 12.f , Gdiplus::FontStyleRegular , Gdiplus::UnitPixel );
			Gdiplus::Font buttonFont( &titleFamily , 18.f , Gdiplus::FontStyleBold , Gdiplus::UnitPixel );

			Gdiplus::SolidBrush titleBrush( Color( 235 , 235 , 245 ) );
			Gdiplus::SolidBrush accentBrush( Color( 168 , 85 , 247 ) );
			Gdiplus::SolidBrush mutedBrush( Color( 150 , 150 , 170 ) );
			Gdiplus::SolidBrush statusBrush( Color( 210 , 210 , 225 ) );
			Gdiplus::SolidBrush successBrush( Color( 90 , 220 , 140 ) );
			Gdiplus::SolidBrush errorBrush( Color( 255 , 110 , 110 ) );

			DrawCenteredText( graphics , L"xv_meca" , titleFont , titleBrush , Gdiplus::RectF( 0.f , 28.f , static_cast<Gdiplus::REAL>( width ) , 42.f ) );
			DrawCenteredText( graphics , L"Launcher" , subtitleFont , accentBrush , Gdiplus::RectF( 0.f , 68.f , static_cast<Gdiplus::REAL>( width ) , 24.f ) );

			Gdiplus::RectF statusCard( 36.f , 112.f , static_cast<Gdiplus::REAL>( width - 72 ) , 88.f );
			Gdiplus::SolidBrush cardBrush( Color( 18 , 16 , 26 , 210 ) );
			Gdiplus::Pen cardBorder( Color( 70 , 45 , 110 , 120 ) , 1.f );
			FillRoundedRect( graphics , statusCard , 14.f , cardBrush );

			{
				Gdiplus::GraphicsPath cardPath;
				const float radius = 14.f;
				const float diameter = radius * 2.f;
				cardPath.AddArc( statusCard.X , statusCard.Y , diameter , diameter , 180.f , 90.f );
				cardPath.AddArc( statusCard.GetRight() - diameter , statusCard.Y , diameter , diameter , 270.f , 90.f );
				cardPath.AddArc( statusCard.GetRight() - diameter , statusCard.GetBottom() - diameter , diameter , diameter , 0.f , 90.f );
				cardPath.AddArc( statusCard.X , statusCard.GetBottom() - diameter , diameter , diameter , 90.f , 90.f );
				cardPath.CloseFigure();
				graphics.DrawPath( &cardBorder , &cardPath );
			}

			const Gdiplus::Brush* statusColor = &statusBrush;
			if ( g_Gui.State.Status == InjectStatus::Injected || g_Gui.State.Status == InjectStatus::AlreadyInjected )
				statusColor = &successBrush;
			else if ( g_Gui.State.Status == InjectStatus::Failed )
				statusColor = &errorBrush;

			DrawCenteredText( graphics , g_Gui.State.StatusText.c_str() , statusFont , *statusColor , Gdiplus::RectF( statusCard.X , statusCard.Y + 8.f , statusCard.Width , 36.f ) );

			wchar_t pidText[96]{};
			if ( g_Gui.State.ProcessId != 0 )
				swprintf_s( pidText , L"PID %lu" , g_Gui.State.ProcessId );
			else
				wcscpy_s( pidText , L"PenguinHotel-Win64-Shipping.exe" );

			DrawCenteredText( graphics , pidText , smallFont , mutedBrush , Gdiplus::RectF( statusCard.X , statusCard.Y + 46.f , statusCard.Width , 24.f ) );

			const Gdiplus::RectF buttonRect(
				static_cast<Gdiplus::REAL>( g_Gui.InjectButtonRect.left ) ,
				static_cast<Gdiplus::REAL>( g_Gui.InjectButtonRect.top ) ,
				static_cast<Gdiplus::REAL>( g_Gui.InjectButtonRect.right - g_Gui.InjectButtonRect.left ) ,
				static_cast<Gdiplus::REAL>( g_Gui.InjectButtonRect.bottom - g_Gui.InjectButtonRect.top ) );

			const bool buttonDisabled = g_Gui.InjectInProgress || !g_Gui.PayloadReady;
			const Gdiplus::Color buttonLeft = buttonDisabled ? Color( 70 , 70 , 85 ) : ( g_Gui.InjectButtonHover ? Color( 190 , 110 , 255 ) : Color( 168 , 85 , 247 ) );
			const Gdiplus::Color buttonRight = buttonDisabled ? Color( 55 , 55 , 68 ) : ( g_Gui.InjectButtonHover ? Color( 130 , 70 , 220 ) : Color( 110 , 55 , 200 ) );

			Gdiplus::LinearGradientBrush buttonBrush(
				Gdiplus::PointF( buttonRect.X , buttonRect.Y ) ,
				Gdiplus::PointF( buttonRect.GetRight() , buttonRect.GetBottom() ) ,
				buttonLeft ,
				buttonRight );
			FillRoundedRect( graphics , buttonRect , 12.f , buttonBrush );

			const wchar_t* buttonLabel = g_Gui.InjectInProgress ? L"INJECTING..." : L"LAUNCH + INJECT";
			Gdiplus::SolidBrush buttonTextBrush( Color( 255 , 255 , 255 ) );
			DrawCenteredText( graphics , buttonLabel , buttonFont , buttonTextBrush , buttonRect );

			const Gdiplus::RectF toggleRect(
				static_cast<Gdiplus::REAL>( g_Gui.AutoToggleRect.left ) ,
				static_cast<Gdiplus::REAL>( g_Gui.AutoToggleRect.top ) ,
				static_cast<Gdiplus::REAL>( g_Gui.AutoToggleRect.right - g_Gui.AutoToggleRect.left ) ,
				static_cast<Gdiplus::REAL>( g_Gui.AutoToggleRect.bottom - g_Gui.AutoToggleRect.top ) );

			const float switchWidth = 42.f;
			const float switchHeight = 22.f;
			const float switchX = toggleRect.GetRight() - switchWidth;
			const float switchY = toggleRect.Y + ( toggleRect.Height - switchHeight ) * 0.5f;

			Gdiplus::SolidBrush toggleLabelBrush( g_Gui.AutoToggleHover ? Color( 220 , 220 , 235 ) : Color( 170 , 170 , 190 ) );
			Gdiplus::RectF toggleLabelRect( toggleRect.X , toggleRect.Y , toggleRect.Width - switchWidth - 8.f , toggleRect.Height );
			Gdiplus::StringFormat leftFormat;
			leftFormat.SetAlignment( Gdiplus::StringAlignmentNear );
			leftFormat.SetLineAlignment( Gdiplus::StringAlignmentCenter );
			graphics.DrawString( L"Auto re-inject" , -1 , &smallFont , toggleLabelRect , &leftFormat , &toggleLabelBrush );

			Gdiplus::SolidBrush switchBgBrush( g_Gui.AutoReinject ? Color( 168 , 85 , 247 ) : Color( 55 , 55 , 68 ) );
			FillRoundedRect( graphics , Gdiplus::RectF( switchX , switchY , switchWidth , switchHeight ) , switchHeight * 0.5f , switchBgBrush );

			const float knobSize = switchHeight - 6.f;
			const float knobX = g_Gui.AutoReinject ? switchX + switchWidth - knobSize - 3.f : switchX + 3.f;
			Gdiplus::SolidBrush knobBrush( Color( 255 , 255 , 255 ) );
			graphics.FillEllipse( &knobBrush , knobX , switchY + 3.f , knobSize , knobSize );

			DrawCenteredText( graphics , g_Gui.FooterText.c_str() , smallFont , mutedBrush , Gdiplus::RectF( 24.f , static_cast<Gdiplus::REAL>( height - 24.f ) , static_cast<Gdiplus::REAL>( width - 48 ) , 18.f ) );
		}

		BitBlt( deviceContext , 0 , 0 , width , height , memoryDc , 0 , 0 , SRCCOPY );
		SelectObject( memoryDc , oldBitmap );
		DeleteObject( bitmap );
		DeleteDC( memoryDc );
	}

	auto RefreshStatus() -> void
	{
		if ( !g_Gui.PayloadReady )
			return;

		RefreshProcessState( g_Gui.State );

		if ( g_Gui.AutoReinject && g_Gui.State.ProcessId != 0 && !g_Gui.State.DllLoaded && !g_Gui.InjectInProgress )
		{
			std::wstring error;
			PerformInject( g_Gui.State , error );
		}
	}

	auto InjectThread( LPVOID ) -> DWORD
	{
		std::wstring error;
		PerformInject( g_Gui.State , error );
		g_Gui.InjectInProgress = false;
		PostMessageW( g_Gui.Window , WM_APP_INJECT_DONE , 0 , 0 );
		return 0;
	}

	auto StartInject() -> void
	{
		if ( g_Gui.InjectInProgress || !g_Gui.PayloadReady )
			return;

		g_Gui.InjectInProgress = true;
		g_Gui.State.Status = InjectStatus::Injecting;
		g_Gui.State.StatusText = L"Injecting...";
		InvalidateRect( g_Gui.Window , nullptr , FALSE );
		CreateThread( nullptr , 0 , InjectThread , nullptr , 0 , nullptr );
	}

	auto LaunchGameOnStartupAsync( LPVOID ) -> DWORD
	{
		if ( g_Gui.GameLaunchStarted )
			return 0;

		g_Gui.GameLaunchStarted = true;
		RefreshProcessState( g_Gui.State );

		if ( g_Gui.State.ProcessId == 0 )
		{
			std::wstring error;
			EnsureGameRunning( g_Gui.State , error );
		}

		if ( g_Gui.AutoReinject && g_Gui.State.ProcessId != 0 && !g_Gui.State.DllLoaded && !g_Gui.InjectInProgress )
		{
			std::wstring error;
			PerformInject( g_Gui.State , error );
		}

		PostMessageW( g_Gui.Window , WM_APP_REFRESH , 0 , 0 );
		return 0;
	}

	auto InitializePayloadAsync( LPVOID ) -> DWORD
	{
		std::wstring error;
		g_Gui.PayloadReady = InitializePayload( error );

		if ( !g_Gui.PayloadReady )
		{
			g_Gui.State.Status = InjectStatus::Failed;
			g_Gui.State.StatusText = error;
		}
		else
		{
			g_Gui.State.Status = InjectStatus::Ready;
			g_Gui.State.StatusText = L"Ready. Launching game with -dx11...";
			g_Gui.FooterText = L"Steam launch + auto inject";
			RefreshStatus();
			CreateThread( nullptr , 0 , LaunchGameOnStartupAsync , nullptr , 0 , nullptr );
		}

		PostMessageW( g_Gui.Window , WM_APP_REFRESH , 0 , 0 );
		return 0;
	}

	auto WindowProc( HWND window , UINT message , WPARAM wParam , LPARAM lParam ) -> LRESULT
	{
		switch ( message )
		{
		case WM_CREATE:
			g_Gui.Window = window;
			{
				RECT clientRect{};
				GetClientRect( window , &clientRect );
				UpdateLayout( clientRect );
			}
			SetTimer( window , kTimerId , 1500 , nullptr );
			CreateThread( nullptr , 0 , InitializePayloadAsync , nullptr , 0 , nullptr );
			return 0;

		case WM_SIZE:
			{
				RECT clientRect{};
				GetClientRect( window , &clientRect );
				UpdateLayout( clientRect );
				InvalidateRect( window , nullptr , FALSE );
			}
			return 0;

		case WM_TIMER:
			if ( wParam == kTimerId )
			{
				RefreshStatus();
				InvalidateRect( window , nullptr , FALSE );
			}
			return 0;

		case WM_APP_REFRESH:
		case WM_APP_INJECT_DONE:
			InvalidateRect( window , nullptr , FALSE );
			return 0;

		case WM_ERASEBKGND:
			return 1;

		case WM_PAINT:
			{
				PAINTSTRUCT paintStruct{};
				const HDC deviceContext = BeginPaint( window , &paintStruct );
				PaintWindow( deviceContext , paintStruct.rcPaint );
				EndPaint( window , &paintStruct );
			}
			return 0;

		case WM_MOUSEMOVE:
			{
				const int x = GET_X_LPARAM( lParam );
				const int y = GET_Y_LPARAM( lParam );
				const bool hoverInject = PointInRect( g_Gui.InjectButtonRect , x , y );
				const bool hoverToggle = PointInRect( g_Gui.AutoToggleRect , x , y );

				if ( hoverInject != g_Gui.InjectButtonHover || hoverToggle != g_Gui.AutoToggleHover )
				{
					g_Gui.InjectButtonHover = hoverInject;
					g_Gui.AutoToggleHover = hoverToggle;
					InvalidateRect( window , nullptr , FALSE );
				}

				TRACKMOUSEEVENT trackEvent{};
				trackEvent.cbSize = sizeof( trackEvent );
				trackEvent.dwFlags = TME_LEAVE;
				trackEvent.hwndTrack = window;
				TrackMouseEvent( &trackEvent );
			}
			return 0;

		case WM_MOUSELEAVE:
			g_Gui.InjectButtonHover = false;
			g_Gui.AutoToggleHover = false;
			InvalidateRect( window , nullptr , FALSE );
			return 0;

		case WM_LBUTTONUP:
			{
				const int x = GET_X_LPARAM( lParam );
				const int y = GET_Y_LPARAM( lParam );

				if ( PointInRect( g_Gui.InjectButtonRect , x , y ) )
					StartInject();
				else if ( PointInRect( g_Gui.AutoToggleRect , x , y ) )
				{
					g_Gui.AutoReinject = !g_Gui.AutoReinject;
					InvalidateRect( window , nullptr , FALSE );
				}
			}
			return 0;

		case WM_DESTROY:
			KillTimer( window , kTimerId );
			PostQuitMessage( 0 );
			return 0;
		}

		return DefWindowProcW( window , message , wParam , lParam );
	}
}

auto WINAPI wWinMain( HINSTANCE instance , HINSTANCE , PWSTR , int showCommand ) -> int
{
	Gdiplus::GdiplusStartup( &g_Gui.GdiplusToken , &g_Gui.GdiplusInput , nullptr );

	WNDCLASSEXW windowClass{};
	windowClass.cbSize = sizeof( windowClass );
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = instance;
	windowClass.hCursor = LoadCursorW( nullptr , MAKEINTRESOURCEW( 32512 ) );
	windowClass.lpszClassName = kWindowClassName;

	RegisterClassExW( &windowClass );

	const int screenWidth = GetSystemMetrics( SM_CXSCREEN );
	const int screenHeight = GetSystemMetrics( SM_CYSCREEN );

	HWND window = CreateWindowExW(
		WS_EX_APPWINDOW ,
		kWindowClassName ,
		L"xv_meca" ,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX ,
		( screenWidth - kWindowWidth ) / 2 ,
		( screenHeight - kWindowHeight ) / 2 ,
		kWindowWidth ,
		kWindowHeight ,
		nullptr ,
		nullptr ,
		instance ,
		nullptr );

	if ( !window )
	{
		Gdiplus::GdiplusShutdown( g_Gui.GdiplusToken );
		return 1;
	}

	SetDarkTitleBar( window );
	ShowWindow( window , showCommand );
	UpdateWindow( window );

	MSG msg{};
	while ( GetMessageW( &msg , nullptr , 0 , 0 ) )
	{
		TranslateMessage( &msg );
		DispatchMessageW( &msg );
	}

	Gdiplus::GdiplusShutdown( g_Gui.GdiplusToken );
	return static_cast<int>( msg.wParam );
}
