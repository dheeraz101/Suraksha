#pragma once

#include <windows.h>
#include <objbase.h>
#include <gdiplus.h>
#include <string>

using namespace Gdiplus;

enum class ButtonVariant {
    Primary,    // Apple System Blue (#0A84FF)
    Secondary,  // Apple Dark Gray (#2C2C2E)
    Danger,     // Apple System Red (#FF453A)
    Ghost       // Transparent
};

class UIComponents {
public:
    static void ApplyRoundedRegion(HWND hWndControl, int radius);
    
    // Direct Canvas Component Renderer (Zero Child HWND Halos)
    static void DrawCanvasCard(Graphics& graphics, int x, int y, int w, int h, Color bg, Color border, int radius = 12);
    static void DrawCanvasButton(Graphics& graphics, int x, int y, int w, int h, const std::wstring& text, ButtonVariant variant, bool isHovered, bool isPressed);
    static void DrawCanvasToggle(Graphics& graphics, int x, int y, int w, int h, const std::wstring& text, bool isON, bool isHovered);
    static void DrawCanvasListItem(Graphics& graphics, int x, int y, int w, int h, const std::wstring& text, bool isSelected, bool isHovered);
    static void DrawTrafficLights(Graphics& graphics, int startX = 20, int startY = 20);
    static void DrawStatusBadge(Graphics& graphics, int x, int y, int w, int h, const std::wstring& text, bool isActive);
    static void DrawEmptyState(Graphics& graphics, int x, int y, int w, int h, const std::wstring& title, const std::wstring& subtitle);

    // Green 'S' Lock Logo Vector Drawing
    static void DrawAppLogo(Graphics& graphics, int x, int y, int size);

    // Modal Dialog HWND Button Renderer
    static void DrawButton(HDC hdc, LPDRAWITEMSTRUCT pdis, const std::wstring& text, ButtonVariant variant = ButtonVariant::Secondary, Color parentBg = Color(255, 20, 20, 23));

    static GraphicsPath* CreateRoundedRectPath(int x, int y, int w, int h, int r);
};
