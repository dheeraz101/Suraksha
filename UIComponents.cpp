#include "UIComponents.h"

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

    // Draw White Padlock Body
    int shackleW = (int)(size * 0.5f);
    int shackleH = (int)(size * 0.45f);
    int shackleX = x + (size - shackleW) / 2;
    int shackleY = y;

    // Outer Shackle Arc
    Pen shacklePen(Color(255, 245, 245, 245), (REAL)(size * 0.1f));
    shacklePen.SetStartCap(LineCapRound);
    shacklePen.SetEndCap(LineCapRound);
    graphics.DrawArc(&shacklePen, shackleX, shackleY, shackleW, shackleH, 180, 180);

    // Lock Base Rect (White Body)
    int bodyY = y + (int)(size * 0.35f);
    int bodyW = size;
    int bodyH = (int)(size * 0.65f);

    DrawCanvasCard(graphics, x, bodyY, bodyW, bodyH, Color(255, 255, 255, 255), Color(200, 220, 220, 220), 4);

    // Draw Bold Green 'S' in center
    FontFamily fontFamily(L"Segoe UI");
    Font font(&fontFamily, (REAL)(size * 0.42f), FontStyleBold, UnitPoint);
    SolidBrush greenBrush(Color(255, 0, 200, 100)); // Vibrant Green #00C864

    StringFormat format;
    format.SetAlignment(StringAlignmentCenter);
    format.SetLineAlignment(StringAlignmentCenter);

    RectF textRect((REAL)x, (REAL)bodyY, (REAL)bodyW, (REAL)bodyH);
    graphics.DrawString(L"S", -1, &font, textRect, &format, &greenBrush);
}

void UIComponents::DrawCanvasCard(Graphics& graphics, int x, int y, int w, int h, Color bg, Color border, int radius) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);

    GraphicsPath* path = CreateRoundedRectPath(x, y, w, h, radius);
    SolidBrush bgBrush(bg);
    Pen borderPen(border, 1.0f);

    graphics.FillPath(&bgBrush, path);
    graphics.DrawPath(&borderPen, path);
    delete path;
}

void UIComponents::DrawButton(HDC hdc, LPDRAWITEMSTRUCT pdis, const std::wstring& text, ButtonVariant variant, Color parentBg) {
    Graphics graphics(hdc);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);

    int x = pdis->rcItem.left;
    int y = pdis->rcItem.top;
    int w = pdis->rcItem.right - x;
    int h = pdis->rcItem.bottom - y;

    // Erase background
    SolidBrush eraseBrush(parentBg);
    graphics.FillRectangle(&eraseBrush, x, y, w, h);

    bool isHovered = false;
    bool isPressed = (pdis->itemState & ODS_SELECTED);
    DrawCanvasButton(graphics, x, y, w, h, text, variant, isHovered, isPressed);
}

void UIComponents::DrawCanvasButton(Graphics& graphics, int x, int y, int w, int h, const std::wstring& text, ButtonVariant variant, bool isHovered, bool isPressed) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);

    Color bg;
    Color border = Color(255, 255, 255, 20);

    switch (variant) {
    case ButtonVariant::Primary:
        if (isPressed) bg = Color(255, 0, 100, 224);
        else if (isHovered) bg = Color(255, 64, 156, 255);
        else bg = Color(255, 10, 132, 255); // Apple System Blue #0A84FF
        break;
    case ButtonVariant::Danger:
        if (isPressed) bg = Color(255, 215, 0, 0);
        else if (isHovered) bg = Color(255, 255, 90, 80);
        else bg = Color(255, 255, 69, 58); // Apple System Red #FF453A
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
        else bg = Color(255, 44, 44, 46); // Apple Dark Gray #2C2C2E
        break;
    }

    DrawCanvasCard(graphics, x, y, w, h, bg, border, 10);

    // Draw Button Text (Enforce single-line no-wrap!)
    FontFamily fontFamily(L"Segoe UI");
    Font font(&fontFamily, 9.5f, FontStyleBold, UnitPoint);
    SolidBrush textBrush(Color(255, 248, 250, 252));

    StringFormat format;
    format.SetAlignment(StringAlignmentCenter);
    format.SetLineAlignment(StringAlignmentCenter);
    format.SetFormatFlags(StringFormatFlagsNoWrap);

    RectF textRect((REAL)x, (REAL)y, (REAL)w, (REAL)h);
    graphics.DrawString(text.c_str(), -1, &font, textRect, &format, &textBrush);
}

void UIComponents::DrawCanvasToggle(Graphics& graphics, int x, int y, int w, int h, const std::wstring& text, bool isON, bool isHovered) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);

    int switchW = 44;
    int switchH = 24;
    int switchY = y + (h - switchH) / 2;

    Color switchBg = isON ? Color(255, 52, 199, 89) : Color(255, 58, 58, 60); // Apple Green #34C759 / Slate #3A3A3C
    if (isHovered && !isON) switchBg = Color(255, 75, 75, 78);

    Color switchBorder = isON ? Color(255, 48, 180, 80) : Color(255, 85, 85, 88);

    // Draw macOS Pill Track
    GraphicsPath* pillPath = CreateRoundedRectPath(x, switchY, switchW, switchH, 12);
    SolidBrush bgBrush(switchBg);
    Pen borderPen(switchBorder, 1.0f);

    graphics.FillPath(&bgBrush, pillPath);
    graphics.DrawPath(&borderPen, pillPath);
    delete pillPath;

    // Draw Sliding White Circular Knob (20x20px)
    int knobSize = 20;
    int knobX = isON ? (x + switchW - knobSize - 2) : (x + 2);
    int knobY = switchY + 2;

    SolidBrush knobBrush(Color(255, 255, 255, 255));
    Pen knobPen(Color(80, 0, 0, 0), 1.0f);

    graphics.FillEllipse(&knobBrush, knobX, knobY, knobSize, knobSize);
    graphics.DrawEllipse(&knobPen, knobX, knobY, knobSize, knobSize);

    // Draw Toggle Label Text
    FontFamily fontFamily(L"Segoe UI");
    Font font(&fontFamily, 10.0f, FontStyleRegular, UnitPoint);
    SolidBrush textBrush(Color(255, 248, 250, 252));

    StringFormat format;
    format.SetAlignment(StringAlignmentNear);
    format.SetLineAlignment(StringAlignmentCenter);

    RectF textRect((REAL)(x + switchW + 12), (REAL)y, (REAL)(w - switchW - 12), (REAL)h);
    graphics.DrawString(text.c_str(), -1, &font, textRect, &format, &textBrush);
}

void UIComponents::DrawCanvasListItem(Graphics& graphics, int x, int y, int w, int h, const std::wstring& text, bool isSelected, bool isHovered) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);

    Color bg;
    if (isSelected) {
        bg = Color(255, 10, 132, 255); // Apple System Blue #0A84FF
    } else if (isHovered) {
        bg = Color(255, 48, 48, 54);
    } else {
        bg = Color(255, 36, 36, 40); // Sidebar Item Slate
    }

    Color border = isSelected ? Color(255, 64, 156, 255) : Color(255, 255, 255, 12);
    DrawCanvasCard(graphics, x, y, w, h, bg, border, 8);

    // Text
    FontFamily fontFamily(L"Segoe UI");
    Font font(&fontFamily, 10.0f, isSelected ? FontStyleBold : FontStyleRegular, UnitPoint);
    SolidBrush textBrush(isSelected ? Color(255, 255, 255, 255) : Color(255, 248, 250, 252));

    StringFormat format;
    format.SetAlignment(StringAlignmentNear);
    format.SetLineAlignment(StringAlignmentCenter);

    RectF textRect((REAL)(x + 14), (REAL)y, (REAL)(w - 20), (REAL)h);
    graphics.DrawString(text.c_str(), -1, &font, textRect, &format, &textBrush);
}

void UIComponents::DrawTrafficLights(Graphics& graphics, int startX, int startY) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);

    SolidBrush redBrush(Color(255, 255, 95, 86));    // Apple Red #FF5F56
    SolidBrush yellowBrush(Color(255, 255, 189, 46)); // Apple Yellow #FFBD2E
    SolidBrush greenBrush(Color(255, 39, 201, 63));  // Apple Green #27C93F

    graphics.FillEllipse(&redBrush, startX, startY, 12, 12);
    graphics.FillEllipse(&yellowBrush, startX + 18, startY, 12, 12);
    graphics.FillEllipse(&greenBrush, startX + 36, startY, 12, 12);
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
    int centerY = y + h / 2 - 20;

    // Vector Shield Icon
    Pen shieldPen(Color(180, 148, 163, 184), 2.0f);
    SolidBrush shieldBrush(Color(30, 148, 163, 184));

    GraphicsPath path;
    path.AddLine(centerX - 18, centerY - 20, centerX + 18, centerY - 20);
    path.AddLine(centerX + 18, centerY - 20, centerX + 18, centerY);
    path.AddBezier(centerX + 18, centerY, centerX + 18, centerY + 12, centerX + 8, centerY + 24, centerX, centerY + 28);
    path.AddBezier(centerX, centerY + 28, centerX - 8, centerY + 24, centerX - 18, centerY + 12, centerX - 18, centerY);
    path.AddLine(centerX - 18, centerY, centerX - 18, centerY - 20);
    path.CloseFigure();

    graphics.FillPath(&shieldBrush, &path);
    graphics.DrawPath(&shieldPen, &path);

    // Text
    FontFamily fontFamily(L"Segoe UI");
    Font titleFont(&fontFamily, 11.0f, FontStyleBold, UnitPoint);
    Font subFont(&fontFamily, 9.5f, FontStyleRegular, UnitPoint);

    SolidBrush titleBrush(Color(255, 248, 250, 252));
    SolidBrush subBrush(Color(255, 148, 163, 184));

    StringFormat format;
    format.SetAlignment(StringAlignmentCenter);
    format.SetLineAlignment(StringAlignmentCenter);

    RectF titleRect((REAL)x, (REAL)(centerY + 36), (REAL)w, 24.0f);
    RectF subRect((REAL)x, (REAL)(centerY + 60), (REAL)w, 20.0f);

    graphics.DrawString(title.c_str(), -1, &titleFont, titleRect, &format, &titleBrush);
    graphics.DrawString(subtitle.c_str(), -1, &subFont, subRect, &format, &subBrush);
}
