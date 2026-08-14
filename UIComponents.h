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

enum class VectorIcon {
    None,
    Lock,
    Shield,
    Logs,
    Info,
    Windows,
    Key,
    Document,
    Globe,
    Terminal,
    Calculator,
    Plus,
    Trash,
    ExternalLink
};

class UIComponents {
public:
    static void ApplyRoundedRegion(HWND hWndControl, int radius);
    
    // Direct Canvas Component Renderer (Zero Child HWND Halos)
    static void DrawCanvasCard(Graphics& graphics, int x, int y, int w, int h, Color bg, Color border, int radius = 12);
    static void DrawCanvasButton(Graphics& graphics, int x, int y, int w, int h, const std::wstring& text, ButtonVariant variant, bool isHovered, bool isPressed, VectorIcon icon = VectorIcon::None);
    static void DrawCanvasToggle(Graphics& graphics, int x, int y, int w, int h, const std::wstring& text, bool isON, bool isHovered);
    static void DrawCanvasListItem(Graphics& graphics, int x, int y, int w, int h, const std::wstring& text, bool isSelected, bool isHovered, VectorIcon icon = VectorIcon::None);
    static void DrawCloseButton(Graphics& graphics, int x, int y, int size = 13, bool isHovered = false);
    static void DrawStatusBadge(Graphics& graphics, int x, int y, int w, int h, const std::wstring& text, bool isActive);
    static void DrawEmptyState(Graphics& graphics, int x, int y, int w, int h, const std::wstring& title, const std::wstring& subtitle);

    // Vector Icon Drawing Suite
    static void DrawAppLogo(Graphics& graphics, int x, int y, int size);
    static void DrawIconLock(Graphics& graphics, int x, int y, int size, Color color);
    static void DrawIconShield(Graphics& graphics, int x, int y, int size, Color color);
    static void DrawIconLogs(Graphics& graphics, int x, int y, int size, Color color);
    static void DrawIconInfo(Graphics& graphics, int x, int y, int size, Color color);
    static void DrawIconWindows(Graphics& graphics, int x, int y, int size, Color color);
    static void DrawIconKey(Graphics& graphics, int x, int y, int size, Color color);
    static void DrawIconDocument(Graphics& graphics, int x, int y, int size, Color color);
    static void DrawIconGlobe(Graphics& graphics, int x, int y, int size, Color color);
    static void DrawIconTerminal(Graphics& graphics, int x, int y, int size, Color color);
    static void DrawIconCalculator(Graphics& graphics, int x, int y, int size, Color color);
    static void DrawIconPlus(Graphics& graphics, int x, int y, int size, Color color);
    static void DrawIconTrash(Graphics& graphics, int x, int y, int size, Color color);
    static void DrawIconExternalLink(Graphics& graphics, int x, int y, int size, Color color);

    static GraphicsPath* CreateRoundedRectPath(int x, int y, int w, int h, int r);
};
