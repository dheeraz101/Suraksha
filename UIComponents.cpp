#include "UIComponents.h"

// ═══════════════════════════════════════════════════════════════
// Windows Segoe MDL2 Assets Icon Font Renderer
// High-quality, pixel-perfect, anti-aliased native Windows icons
// ═══════════════════════════════════════════════════════════════

static void DrawMDL2Glyph(Graphics& graphics, int x, int y, int size, Color color, const wchar_t* glyph) {
    graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);

    // Try Segoe Fluent Icons (Win11) first, fallback to Segoe MDL2 Assets (Win10)
    FontFamily fontFamily(L"Segoe Fluent Icons");
    if (!fontFamily.IsAvailable()) {
        FontFamily fallback(L"Segoe MDL2 Assets");
        Font font(&fallback, (REAL)(size * 0.88f), FontStyleRegular, UnitPixel);
        SolidBrush brush(color);
        StringFormat format;
        format.SetAlignment(StringAlignmentCenter);
        format.SetLineAlignment(StringAlignmentCenter);
        RectF rect((REAL)x, (REAL)y, (REAL)size, (REAL)size);
        graphics.DrawString(glyph, -1, &font, rect, &format, &brush);
        return;
    }

    Font font(&fontFamily, (REAL)(size * 0.88f), FontStyleRegular, UnitPixel);
    SolidBrush brush(color);
    StringFormat format;
    format.SetAlignment(StringAlignmentCenter);
    format.SetLineAlignment(StringAlignmentCenter);
    RectF rect((REAL)x, (REAL)y, (REAL)size, (REAL)size);
    graphics.DrawString(glyph, -1, &font, rect, &format, &brush);
}

void UIComponents::ApplyRoundedRegion(HWND hWndControl, int radius) {
    if (!hWndControl) return;
    RECT rc;
    GetClientRect(hWndControl, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    if (w <= 0 || h <= 0) return;
    HRGN hRgn = CreateRoundRectRgn(0, 0, w + 1, h + 1, radius * 2, radius * 2);
    SetWindowRgn(hWndControl, hRgn, TRUE);
}

GraphicsPath* UIComponents::CreateRoundedRectPath(int x, int y, int w, int h, int r) {
    GraphicsPath* path = new GraphicsPath();
    if (r <= 0) {
        path->AddRectangle(Rect(x, y, w, h));
        return path;
    }
    int d = r * 2;
    if (d > w) d = w;
    if (d > h) d = h;
    path->AddArc(x, y, d, d, 180, 90);
    path->AddArc(x + w - d, y, d, d, 270, 90);
    path->AddArc(x + w - d, y + h - d, d, d, 0, 90);
    path->AddArc(x, y + h - d, d, d, 90, 90);
    path->CloseFigure();
    return path;
}

void UIComponents::DrawAppLogo(Graphics& graphics, int x, int y, int size) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);

    // Shield path with modern Apple/Fluent curvature
    GraphicsPath shieldPath;
    float cx = x + size / 2.0f;
    float topY = (float)y;
    float botY = y + size * 0.95f;
    float leftX = (float)x + size * 0.08f;
    float rightX = x + size * 0.92f;
    float midY = y + size * 0.52f;

    shieldPath.AddBezier(PointF(leftX, topY + size * 0.15f), PointF(leftX, topY + size * 0.04f), PointF(cx - size * 0.12f, topY), PointF(cx, topY));
    shieldPath.AddBezier(PointF(cx, topY), PointF(cx + size * 0.12f, topY), PointF(rightX, topY + size * 0.04f), PointF(rightX, topY + size * 0.15f));
    shieldPath.AddBezier(PointF(rightX, topY + size * 0.15f), PointF(rightX, midY), PointF(cx + size * 0.18f, botY - size * 0.12f), PointF(cx, botY));
    shieldPath.AddBezier(PointF(cx, botY), PointF(cx - size * 0.18f, botY - size * 0.12f), PointF(leftX, midY), PointF(leftX, topY + size * 0.15f));
    shieldPath.CloseFigure();

    // Vibrant Royal Blue to Deep Cobalt Linear Gradient
    LinearGradientBrush shieldGrad(
        Point(x, y), Point(x, y + size),
        Color(255, 10, 132, 255), // System Blue
        Color(255, 0, 75, 175)    // Deep Indigo
    );
    graphics.FillPath(&shieldGrad, &shieldPath);

    // Neon accent outline
    Pen borderPen(Color(180, 120, 210, 255), 1.5f);
    graphics.DrawPath(&borderPen, &shieldPath);

    // Centered White Padlock Shackle
    int shkSize = (int)(size * 0.28f);
    int shkX = (int)(cx - shkSize / 2.0f);
    int shkY = (int)(y + size * 0.23f);
    Pen shkPen(Color(255, 255, 255, 255), 2.2f);
    shkPen.SetStartCap(LineCapRound);
    shkPen.SetEndCap(LineCapRound);
    graphics.DrawArc(&shkPen, shkX, shkY, shkSize, (int)(shkSize * 1.1f), 180, 180);

    // Pure White Lock Body Card
    int lockW = (int)(size * 0.40f);
    int lockH = (int)(size * 0.32f);
    int lockX = (int)(cx - lockW / 2.0f);
    int lockY = (int)(y + size * 0.44f);
    DrawCanvasCard(graphics, lockX, lockY, lockW, lockH, Color(255, 255, 255, 255), Color(30, 0, 0, 0), 4);

    // Keyhole
    SolidBrush keyHoleBrush(Color(255, 10, 132, 255));
    graphics.FillEllipse(&keyHoleBrush, (int)(cx - 2.5f), lockY + 5, 5, 5);
    Pen keyStemPen(Color(255, 10, 132, 255), 2.0f);
    keyStemPen.SetStartCap(LineCapRound);
    keyStemPen.SetEndCap(LineCapRound);
    graphics.DrawLine(&keyStemPen, (int)cx, lockY + 8, (int)cx, lockY + 13);
}

void UIComponents::DrawCloseButton(Graphics& graphics, int x, int y, int size, bool isHovered) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    Color bg = isHovered ? Color(255, 255, 69, 58) : Color(255, 255, 95, 86);
    SolidBrush brush(bg);
    graphics.FillEllipse(&brush, x, y, size, size);
    if (isHovered) {
        Pen xPen(Color(220, 60, 0, 0), 1.2f);
        xPen.SetStartCap(LineCapRound);
        xPen.SetEndCap(LineCapRound);
        int pad = (int)(size * 0.28f);
        graphics.DrawLine(&xPen, x + pad, y + pad, x + size - pad, y + size - pad);
        graphics.DrawLine(&xPen, x + size - pad, y + pad, x + pad, y + size - pad);
    }
}

// ═══════════════════════════════════════════════════════════════
// High-Quality MDL2 Icon Suite (Segoe MDL2 Assets / Segoe Fluent Icons)
// ═══════════════════════════════════════════════════════════════

void UIComponents::DrawIconLock(Graphics& graphics, int x, int y, int size, Color color) {
    DrawMDL2Glyph(graphics, x, y, size, color, L"\xE72E");
}

void UIComponents::DrawIconShield(Graphics& graphics, int x, int y, int size, Color color) {
    DrawMDL2Glyph(graphics, x, y, size, color, L"\xEA18");
}

void UIComponents::DrawIconLogs(Graphics& graphics, int x, int y, int size, Color color) {
    DrawMDL2Glyph(graphics, x, y, size, color, L"\xE8A5");
}

void UIComponents::DrawIconInfo(Graphics& graphics, int x, int y, int size, Color color) {
    DrawMDL2Glyph(graphics, x, y, size, color, L"\xE946");
}

void UIComponents::DrawIconWindows(Graphics& graphics, int x, int y, int size, Color color) {
    DrawMDL2Glyph(graphics, x, y, size, color, L"\xE782");
}

void UIComponents::DrawIconKey(Graphics& graphics, int x, int y, int size, Color color) {
    DrawMDL2Glyph(graphics, x, y, size, color, L"\xE8D7");
}

void UIComponents::DrawIconDocument(Graphics& graphics, int x, int y, int size, Color color) {
    DrawMDL2Glyph(graphics, x, y, size, color, L"\xE8A5");
}

void UIComponents::DrawIconGlobe(Graphics& graphics, int x, int y, int size, Color color) {
    DrawMDL2Glyph(graphics, x, y, size, color, L"\xE774");
}

void UIComponents::DrawIconTerminal(Graphics& graphics, int x, int y, int size, Color color) {
    DrawMDL2Glyph(graphics, x, y, size, color, L"\xE756");
}

void UIComponents::DrawIconCalculator(Graphics& graphics, int x, int y, int size, Color color) {
    DrawMDL2Glyph(graphics, x, y, size, color, L"\xE8EF");
}

void UIComponents::DrawIconPlus(Graphics& graphics, int x, int y, int size, Color color) {
    DrawMDL2Glyph(graphics, x, y, size, color, L"\xE710");
}

void UIComponents::DrawIconTrash(Graphics& graphics, int x, int y, int size, Color color) {
    DrawMDL2Glyph(graphics, x, y, size, color, L"\xE74D");
}

void UIComponents::DrawIconExternalLink(Graphics& graphics, int x, int y, int size, Color color) {
    DrawMDL2Glyph(graphics, x, y, size, color, L"\xE8A7");
}

void UIComponents::DrawIconUpdate(Graphics& graphics, int x, int y, int size, Color color) {
    DrawMDL2Glyph(graphics, x, y, size, color, L"\xE895"); // Sync / Refresh Arrow
}

void UIComponents::DrawIconDownload(Graphics& graphics, int x, int y, int size, Color color) {
    DrawMDL2Glyph(graphics, x, y, size, color, L"\xE896"); // Download Arrow
}

void UIComponents::DrawIconCheck(Graphics& graphics, int x, int y, int size, Color color) {
    DrawMDL2Glyph(graphics, x, y, size, color, L"\xE73E"); // Checkmark
}

void UIComponents::DrawIconWarning(Graphics& graphics, int x, int y, int size, Color color) {
    DrawMDL2Glyph(graphics, x, y, size, color, L"\xE7BA"); // Warning triangle
}

void UIComponents::DrawIconClock(Graphics& graphics, int x, int y, int size, Color color) {
    DrawMDL2Glyph(graphics, x, y, size, color, L"\xE823"); // Clock / Time
}

void UIComponents::DrawIconExport(Graphics& graphics, int x, int y, int size, Color color) {
    DrawMDL2Glyph(graphics, x, y, size, color, L"\xEDE1"); // Export / Upload
}

void UIComponents::DrawIconImport(Graphics& graphics, int x, int y, int size, Color color) {
    DrawMDL2Glyph(graphics, x, y, size, color, L"\xE8B5"); // Import / Folder Open
}

void UIComponents::DrawIconLanguage(Graphics& graphics, int x, int y, int size, Color color) {
    DrawMDL2Glyph(graphics, x, y, size, color, L"\xE774"); // Globe / Language
}

void UIComponents::DrawProgressBar(Graphics& graphics, int x, int y, int w, int h, int percent) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);

    int r = h / 2;
    // Background track
    GraphicsPath* trackPath = CreateRoundedRectPath(x, y, w, h, r);
    SolidBrush trackBrush(Color(255, 44, 44, 48));
    graphics.FillPath(&trackBrush, trackPath);
    delete trackPath;

    // Active fill
    int clamped = percent < 0 ? 0 : (percent > 100 ? 100 : percent);
    int fillW = (int)((w * clamped) / 100.0f);
    if (fillW > 4) {
        GraphicsPath* fillPath = CreateRoundedRectPath(x, y, fillW, h, r);
        LinearGradientBrush fillGrad(
            Point(x, y), Point(x + fillW, y),
            Color(255, 10, 132, 255), // System Blue
            Color(255, 0, 180, 255)   // Cyan glow
        );
        graphics.FillPath(&fillGrad, fillPath);
        delete fillPath;
    }
}

void UIComponents::DrawVectorRadio(Graphics& graphics, int x, int y, int size, bool isSelected, Color activeColor) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);

    // Outer circle
    Pen ringPen(isSelected ? activeColor : Color(120, 255, 255, 255), 1.5f);
    graphics.DrawEllipse(&ringPen, x, y, size, size);

    if (isSelected) {
        // Inner filled dot
        int dotInset = size / 4;
        int dotSize = size - (dotInset * 2);
        SolidBrush dotBrush(activeColor);
        graphics.FillEllipse(&dotBrush, x + dotInset, y + dotInset, dotSize, dotSize);
    }
}

// ═══════════════════════════════════════════════════════════════
// Composite UI Components
// ═══════════════════════════════════════════════════════════════

void UIComponents::DrawCanvasCard(Graphics& graphics, int x, int y, int w, int h, Color bg, Color border, int radius) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    GraphicsPath* path = CreateRoundedRectPath(x, y, w, h, radius);
    SolidBrush bgBrush(bg);
    Pen borderPen(border, 1.0f);
    graphics.FillPath(&bgBrush, path);
    graphics.DrawPath(&borderPen, path);
    delete path;
}

void UIComponents::DrawCanvasButton(Graphics& graphics, int x, int y, int w, int h, const std::wstring& text, ButtonVariant variant, bool isHovered, bool isPressed, VectorIcon icon) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);

    Color bg;
    Color border = Color(25, 255, 255, 255);

    switch (variant) {
    case ButtonVariant::Primary:
        if (isPressed) bg = Color(255, 0, 100, 224);
        else if (isHovered) bg = Color(255, 64, 156, 255);
        else bg = Color(255, 10, 132, 255);
        border = Color(255, 10, 132, 255);
        break;
    case ButtonVariant::Danger:
        if (isPressed) bg = Color(255, 215, 0, 0);
        else if (isHovered) bg = Color(255, 255, 90, 80);
        else bg = Color(255, 255, 69, 58);
        border = Color(255, 255, 69, 58);
        break;
    case ButtonVariant::Ghost:
        if (isPressed) bg = Color(50, 255, 255, 255);
        else if (isHovered) bg = Color(25, 255, 255, 255);
        else bg = Color(0, 0, 0, 0);
        break;
    case ButtonVariant::Secondary:
    default:
        if (isPressed) bg = Color(255, 58, 58, 62);
        else if (isHovered) bg = Color(255, 60, 60, 66);
        else bg = Color(255, 44, 44, 46);
        break;
    }

    DrawCanvasCard(graphics, x, y, w, h, bg, border, 10);

    Color textCol = Color(255, 248, 250, 252);
    int iconOffset = 0;

    if (icon != VectorIcon::None) {
        int iconSize = 16;
        int iconX = x + 12;
        int iconY = y + (h - iconSize) / 2;
        iconOffset = 18;

        switch (icon) {
        case VectorIcon::Lock:         DrawIconLock(graphics, iconX, iconY, iconSize, textCol); break;
        case VectorIcon::Shield:       DrawIconShield(graphics, iconX, iconY, iconSize, textCol); break;
        case VectorIcon::Windows:      DrawIconWindows(graphics, iconX, iconY, iconSize, textCol); break;
        case VectorIcon::Key:          DrawIconKey(graphics, iconX, iconY, iconSize, textCol); break;
        case VectorIcon::Document:     DrawIconDocument(graphics, iconX, iconY, iconSize, textCol); break;
        case VectorIcon::Globe:        DrawIconGlobe(graphics, iconX, iconY, iconSize, textCol); break;
        case VectorIcon::Terminal:     DrawIconTerminal(graphics, iconX, iconY, iconSize, textCol); break;
        case VectorIcon::Calculator:   DrawIconCalculator(graphics, iconX, iconY, iconSize, textCol); break;
        case VectorIcon::Plus:         DrawIconPlus(graphics, iconX, iconY, iconSize, textCol); break;
        case VectorIcon::Trash:        DrawIconTrash(graphics, iconX, iconY, iconSize, textCol); break;
        case VectorIcon::ExternalLink: DrawIconExternalLink(graphics, iconX, iconY, iconSize, textCol); break;
        case VectorIcon::Update:       DrawIconUpdate(graphics, iconX, iconY, iconSize, textCol); break;
        case VectorIcon::Download:     DrawIconDownload(graphics, iconX, iconY, iconSize, textCol); break;
        case VectorIcon::Check:        DrawIconCheck(graphics, iconX, iconY, iconSize, textCol); break;
        case VectorIcon::Warning:      DrawIconWarning(graphics, iconX, iconY, iconSize, textCol); break;
        case VectorIcon::Clock:        DrawIconClock(graphics, iconX, iconY, iconSize, textCol); break;
        case VectorIcon::Export:       DrawIconExport(graphics, iconX, iconY, iconSize, textCol); break;
        case VectorIcon::Import:       DrawIconImport(graphics, iconX, iconY, iconSize, textCol); break;
        case VectorIcon::LanguageIcon: DrawIconLanguage(graphics, iconX, iconY, iconSize, textCol); break;
        default: break;
        }
    }

    FontFamily fontFamily(L"Segoe UI");
    Font font(&fontFamily, 9.5f, FontStyleBold, UnitPoint);
    SolidBrush textBrush(textCol);
    StringFormat format;
    format.SetAlignment(StringAlignmentCenter);
    format.SetLineAlignment(StringAlignmentCenter);
    format.SetFormatFlags(StringFormatFlagsNoWrap);
    RectF textRect((REAL)(x + (iconOffset > 0 ? 10 : 0)), (REAL)y, (REAL)(w - (iconOffset > 0 ? 10 : 0)), (REAL)h);
    graphics.DrawString(text.c_str(), -1, &font, textRect, &format, &textBrush);
}

void UIComponents::DrawCanvasToggle(Graphics& graphics, int x, int y, int w, int h, const std::wstring& text, bool isON, bool isHovered) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);

    int switchW = 44;
    int switchH = 24;
    int switchY = y + (h - switchH) / 2;

    Color switchBg = isON ? Color(255, 52, 199, 89) : Color(255, 58, 58, 60);
    if (isHovered && !isON) switchBg = Color(255, 75, 75, 78);
    Color switchBorder = isON ? Color(255, 48, 180, 80) : Color(255, 85, 85, 88);

    GraphicsPath* pillPath = CreateRoundedRectPath(x, switchY, switchW, switchH, 12);
    SolidBrush bgBrush(switchBg);
    Pen borderPen(switchBorder, 1.0f);
    graphics.FillPath(&bgBrush, pillPath);
    graphics.DrawPath(&borderPen, pillPath);
    delete pillPath;

    int knobSize = 20;
    int knobX = isON ? (x + switchW - knobSize - 2) : (x + 2);
    int knobY = switchY + 2;
    SolidBrush knobBrush(Color(255, 255, 255, 255));
    Pen knobPen(Color(80, 0, 0, 0), 1.0f);
    graphics.FillEllipse(&knobBrush, knobX, knobY, knobSize, knobSize);
    graphics.DrawEllipse(&knobPen, knobX, knobY, knobSize, knobSize);

    FontFamily fontFamily(L"Segoe UI");
    Font font(&fontFamily, 10.0f, FontStyleRegular, UnitPoint);
    SolidBrush textBrush(Color(255, 248, 250, 252));
    StringFormat format;
    format.SetAlignment(StringAlignmentNear);
    format.SetLineAlignment(StringAlignmentCenter);
    RectF textRect((REAL)(x + switchW + 12), (REAL)y, (REAL)(w - switchW - 12), (REAL)h);
    graphics.DrawString(text.c_str(), -1, &font, textRect, &format, &textBrush);
}

void UIComponents::DrawCanvasListItem(Graphics& graphics, int x, int y, int w, int h, const std::wstring& text, bool isSelected, bool isHovered, VectorIcon icon) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);

    Color bg;
    if (isSelected) bg = Color(255, 10, 132, 255);
    else if (isHovered) bg = Color(255, 48, 48, 54);
    else bg = Color(255, 36, 36, 40);

    Color border = isSelected ? Color(255, 64, 156, 255) : Color(20, 255, 255, 255);
    DrawCanvasCard(graphics, x, y, w, h, bg, border, 8);

    Color textCol = isSelected ? Color(255, 255, 255, 255) : Color(255, 248, 250, 252);
    int textStartX = x + 14;

    if (icon != VectorIcon::None) {
        int iconSize = 18;
        int iconY = y + (h - iconSize) / 2;
        int iconX = x + 12;

        switch (icon) {
        case VectorIcon::Lock:   DrawIconLock(graphics, iconX, iconY, iconSize, textCol); break;
        case VectorIcon::Shield: DrawIconShield(graphics, iconX, iconY, iconSize, textCol); break;
        case VectorIcon::Logs:   DrawIconLogs(graphics, iconX, iconY, iconSize, textCol); break;
        case VectorIcon::Info:   DrawIconInfo(graphics, iconX, iconY, iconSize, textCol); break;
        case VectorIcon::Update: DrawIconUpdate(graphics, iconX, iconY, iconSize, textCol); break;
        default: break;
        }
        textStartX = x + 38;
    }

    FontFamily fontFamily(L"Segoe UI");
    Font font(&fontFamily, 10.0f, isSelected ? FontStyleBold : FontStyleRegular, UnitPoint);
    SolidBrush textBrush(textCol);
    StringFormat format;
    format.SetAlignment(StringAlignmentNear);
    format.SetLineAlignment(StringAlignmentCenter);
    RectF textRect((REAL)textStartX, (REAL)y, (REAL)(w - (textStartX - x) - 10), (REAL)h);
    graphics.DrawString(text.c_str(), -1, &font, textRect, &format, &textBrush);
}

void UIComponents::DrawStatusBadge(Graphics& graphics, int x, int y, int w, int h, const std::wstring& text, bool isActive) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);

    Color bg = isActive ? Color(35, 52, 199, 89) : Color(35, 255, 69, 58);
    Color border = isActive ? Color(100, 52, 199, 89) : Color(100, 255, 69, 58);
    Color dotColor = isActive ? Color(255, 52, 199, 89) : Color(255, 255, 69, 58);

    DrawCanvasCard(graphics, x, y, w, h, bg, border, 12);

    SolidBrush dotBrush(dotColor);
    graphics.FillEllipse(&dotBrush, x + 10, y + (h - 8) / 2, 8, 8);

    FontFamily fontFamily(L"Segoe UI");
    Font font(&fontFamily, 9.5f, FontStyleBold, UnitPoint);
    SolidBrush textBrush(isActive ? Color(255, 52, 199, 89) : Color(255, 255, 69, 58));
    StringFormat format;
    format.SetAlignment(StringAlignmentCenter);
    format.SetLineAlignment(StringAlignmentCenter);
    RectF textRect((REAL)(x + 10), (REAL)y, (REAL)(w - 10), (REAL)h);
    graphics.DrawString(text.c_str(), -1, &font, textRect, &format, &textBrush);
}

void UIComponents::DrawEmptyState(Graphics& graphics, int x, int y, int w, int h, const std::wstring& title, const std::wstring& subtitle) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);

    int centerX = x + w / 2;
    int centerY = y + h / 2 - 30;

    // Large shield icon using MDL2
    DrawMDL2Glyph(graphics, centerX - 20, centerY - 20, 40, Color(120, 148, 163, 184), L"\xEA18");

    FontFamily fontFamily(L"Segoe UI");
    Font titleFont(&fontFamily, 11.0f, FontStyleBold, UnitPoint);
    Font subFont(&fontFamily, 9.5f, FontStyleRegular, UnitPoint);

    SolidBrush titleBrush(Color(255, 248, 250, 252));
    SolidBrush subBrush(Color(255, 148, 163, 184));

    StringFormat format;
    format.SetAlignment(StringAlignmentCenter);
    format.SetLineAlignment(StringAlignmentCenter);

    RectF titleRect((REAL)x, (REAL)(centerY + 30), (REAL)w, 24.0f);
    RectF subRect((REAL)x, (REAL)(centerY + 54), (REAL)w, 20.0f);

    graphics.DrawString(title.c_str(), -1, &titleFont, titleRect, &format, &titleBrush);
    graphics.DrawString(subtitle.c_str(), -1, &subFont, subRect, &format, &subBrush);
}
