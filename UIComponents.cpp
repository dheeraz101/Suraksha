#include "UIComponents.h"
#include "FontManager.h"
#include <cmath>
#include <algorithm>

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

// ═══════════════════════════════════════════════════════════════
// Pure High-Precision Anti-Aliased Vector Icon Suite
// 100% standalone, zero font dependencies, pixel-perfect at any DPI
// ═══════════════════════════════════════════════════════════════

void UIComponents::DrawIconLock(Graphics& graphics, int x, int y, int size, Color color) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    float cx = x + size / 2.0f;
    float w = size * 0.72f;
    float h = size * 0.54f;
    float bx = cx - w / 2.0f;
    float by = y + size * 0.42f;

    // Shackle
    float sw = size * 0.42f;
    float sx = cx - sw / 2.0f;
    float sy = y + size * 0.10f;
    float sh = size * 0.44f;
    Pen shkPen(color, (std::max)(1.6f, size * 0.12f));
    shkPen.SetStartCap(LineCapRound);
    shkPen.SetEndCap(LineCapRound);
    graphics.DrawArc(&shkPen, sx, sy, sw, sh, 180, 180);
    graphics.DrawLine(&shkPen, sx, sy + sh / 2.0f, sx, by);
    graphics.DrawLine(&shkPen, sx + sw, sy + sh / 2.0f, sx + sw, by);

    // Body
    GraphicsPath* body = CreateRoundedRectPath((int)bx, (int)by, (int)w, (int)h, (std::max)(2, (int)(size * 0.14f)));
    SolidBrush brush(color);
    graphics.FillPath(&brush, body);
    delete body;

    // Keyhole
    SolidBrush holeBrush(Color(255, 20, 20, 24));
    graphics.FillEllipse(&holeBrush, cx - size * 0.07f, by + h * 0.28f, size * 0.14f, size * 0.14f);
    Pen stemPen(Color(255, 20, 20, 24), (std::max)(1.4f, size * 0.09f));
    graphics.DrawLine(&stemPen, cx, by + h * 0.35f, cx, by + h * 0.65f);
}

void UIComponents::DrawIconShield(Graphics& graphics, int x, int y, int size, Color color) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    float cx = x + size / 2.0f;
    float topY = (float)(y + size * 0.08f);
    float botY = (float)(y + size * 0.92f);
    float leftX = (float)(x + size * 0.12f);
    float rightX = (float)(x + size * 0.88f);
    float midY = (float)(y + size * 0.50f);

    GraphicsPath path;
    path.AddBezier(PointF(leftX, topY + size * 0.12f), PointF(leftX, topY), PointF(cx - size * 0.10f, topY), PointF(cx, topY));
    path.AddBezier(PointF(cx, topY), PointF(cx + size * 0.10f, topY), PointF(rightX, topY), PointF(rightX, topY + size * 0.12f));
    path.AddBezier(PointF(rightX, topY + size * 0.12f), PointF(rightX, midY), PointF(cx + size * 0.15f, botY - size * 0.10f), PointF(cx, botY));
    path.AddBezier(PointF(cx, botY), PointF(cx - size * 0.15f, botY - size * 0.10f), PointF(leftX, midY), PointF(leftX, topY + size * 0.12f));
    path.CloseFigure();

    Pen pen(color, (std::max)(1.6f, size * 0.10f));
    graphics.DrawPath(&pen, &path);

    Pen innerPen(color, (std::max)(1.4f, size * 0.09f));
    innerPen.SetStartCap(LineCapRound);
    innerPen.SetEndCap(LineCapRound);
    graphics.DrawLine(&innerPen, cx, topY + size * 0.22f, cx, botY - size * 0.25f);
}

void UIComponents::DrawIconLogs(Graphics& graphics, int x, int y, int size, Color color) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    float w = size * 0.65f;
    float h = size * 0.82f;
    float lx = x + (size - w) / 2.0f;
    float ly = y + (size - h) / 2.0f;
    float fold = size * 0.22f;

    GraphicsPath path;
    path.AddLine(lx, ly, lx + w - fold, ly);
    path.AddLine(lx + w - fold, ly, lx + w, ly + fold);
    path.AddLine(lx + w, ly + fold, lx + w, ly + h);
    path.AddLine(lx + w, ly + h, lx, ly + h);
    path.CloseFigure();

    Pen pen(color, (std::max)(1.5f, size * 0.10f));
    pen.SetStartCap(LineCapRound);
    pen.SetEndCap(LineCapRound);
    graphics.DrawPath(&pen, &path);

    graphics.DrawLine(&pen, lx + w - fold, ly, lx + w - fold, ly + fold);
    graphics.DrawLine(&pen, lx + w - fold, ly + fold, lx + w, ly + fold);

    Pen linePen(color, (std::max)(1.3f, size * 0.08f));
    linePen.SetStartCap(LineCapRound);
    linePen.SetEndCap(LineCapRound);
    graphics.DrawLine(&linePen, lx + size * 0.12f, ly + size * 0.36f, lx + w - size * 0.12f, ly + size * 0.36f);
    graphics.DrawLine(&linePen, lx + size * 0.12f, ly + size * 0.52f, lx + w - size * 0.12f, ly + size * 0.52f);
    graphics.DrawLine(&linePen, lx + size * 0.12f, ly + size * 0.68f, lx + w - size * 0.24f, ly + size * 0.68f);
}

void UIComponents::DrawIconInfo(Graphics& graphics, int x, int y, int size, Color color) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    float cx = x + size / 2.0f;
    float cy = y + size / 2.0f;
    float r = size * 0.40f;

    Pen pen(color, (std::max)(1.6f, size * 0.10f));
    graphics.DrawEllipse(&pen, cx - r, cy - r, r * 2.0f, r * 2.0f);

    SolidBrush brush(color);
    float dotR = (std::max)(1.2f, size * 0.07f);
    graphics.FillEllipse(&brush, cx - dotR, cy - size * 0.22f - dotR, dotR * 2.0f, dotR * 2.0f);

    Pen stemPen(color, (std::max)(1.6f, size * 0.10f));
    stemPen.SetStartCap(LineCapRound);
    stemPen.SetEndCap(LineCapRound);
    graphics.DrawLine(&stemPen, cx, cy - size * 0.06f, cx, cy + size * 0.20f);
}

void UIComponents::DrawIconWindows(Graphics& graphics, int x, int y, int size, Color color) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    float cx = x + size / 2.0f;
    float cy = y + size / 2.0f;
    float half = size * 0.36f;
    float gap = (std::max)(1.5f, size * 0.08f);
    float pane = half - gap / 2.0f;

    SolidBrush brush(color);
    graphics.FillRectangle(&brush, cx - gap / 2.0f - pane, cy - gap / 2.0f - pane, pane, pane);
    graphics.FillRectangle(&brush, cx + gap / 2.0f, cy - gap / 2.0f - pane, pane, pane);
    graphics.FillRectangle(&brush, cx - gap / 2.0f - pane, cy + gap / 2.0f, pane, pane);
    graphics.FillRectangle(&brush, cx + gap / 2.0f, cy + gap / 2.0f, pane, pane);
}

void UIComponents::DrawIconKey(Graphics& graphics, int x, int y, int size, Color color) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    Pen pen(color, (std::max)(1.6f, size * 0.10f));
    pen.SetStartCap(LineCapRound);
    pen.SetEndCap(LineCapRound);

    float headR = size * 0.20f;
    float hx = x + size * 0.32f;
    float hy = y + size * 0.35f;

    graphics.DrawEllipse(&pen, hx - headR, hy - headR, headR * 2.0f, headR * 2.0f);

    float stemStartX = hx + headR * 0.707f;
    float stemStartY = hy + headR * 0.707f;
    float stemEndX = x + size * 0.85f;
    float stemEndY = y + size * 0.85f;

    graphics.DrawLine(&pen, stemStartX, stemStartY, stemEndX, stemEndY);
    graphics.DrawLine(&pen, stemEndX - size * 0.12f, stemEndY - size * 0.12f, stemEndX - size * 0.02f, stemEndY - size * 0.22f);
    graphics.DrawLine(&pen, stemEndX, stemEndY, stemEndX + size * 0.08f, stemEndY - size * 0.08f);
}

void UIComponents::DrawIconDocument(Graphics& graphics, int x, int y, int size, Color color) {
    DrawIconLogs(graphics, x, y, size, color);
}

void UIComponents::DrawIconGlobe(Graphics& graphics, int x, int y, int size, Color color) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    float cx = x + size / 2.0f;
    float cy = y + size / 2.0f;
    float r = size * 0.38f;

    Pen pen(color, (std::max)(1.5f, size * 0.10f));
    graphics.DrawEllipse(&pen, cx - r, cy - r, r * 2.0f, r * 2.0f);
    graphics.DrawLine(&pen, cx - r, cy, cx + r, cy);
    graphics.DrawEllipse(&pen, cx - r * 0.50f, cy - r, r, r * 2.0f);
}

void UIComponents::DrawIconTerminal(Graphics& graphics, int x, int y, int size, Color color) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    float w = size * 0.80f;
    float h = size * 0.65f;
    float tx = x + (size - w) / 2.0f;
    float ty = y + (size - h) / 2.0f;

    GraphicsPath* box = CreateRoundedRectPath((int)tx, (int)ty, (int)w, (int)h, (std::max)(2, (int)(size * 0.12f)));
    Pen pen(color, (std::max)(1.5f, size * 0.10f));
    graphics.DrawPath(&pen, box);
    delete box;

    Pen chevPen(color, (std::max)(1.5f, size * 0.10f));
    chevPen.SetStartCap(LineCapRound);
    chevPen.SetEndCap(LineCapRound);
    graphics.DrawLine(&chevPen, tx + size * 0.18f, ty + size * 0.20f, tx + size * 0.32f, ty + size * 0.33f);
    graphics.DrawLine(&chevPen, tx + size * 0.32f, ty + size * 0.33f, tx + size * 0.18f, ty + size * 0.46f);
    graphics.DrawLine(&chevPen, tx + size * 0.40f, ty + size * 0.46f, tx + size * 0.60f, ty + size * 0.46f);
}

void UIComponents::DrawIconCalculator(Graphics& graphics, int x, int y, int size, Color color) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    float w = size * 0.70f;
    float h = size * 0.85f;
    float cx = x + (size - w) / 2.0f;
    float cy = y + (size - h) / 2.0f;

    GraphicsPath* body = CreateRoundedRectPath((int)cx, (int)cy, (int)w, (int)h, (std::max)(2, (int)(size * 0.12f)));
    Pen pen(color, (std::max)(1.5f, size * 0.10f));
    graphics.DrawPath(&pen, body);
    delete body;

    graphics.DrawLine(&pen, cx + size * 0.12f, cy + size * 0.22f, cx + w - size * 0.12f, cy + size * 0.22f);

    SolidBrush dotBrush(color);
    float dR = size * 0.06f;
    graphics.FillEllipse(&dotBrush, cx + size * 0.18f, cy + size * 0.42f, dR * 2.0f, dR * 2.0f);
    graphics.FillEllipse(&dotBrush, cx + w - size * 0.30f, cy + size * 0.42f, dR * 2.0f, dR * 2.0f);
    graphics.FillEllipse(&dotBrush, cx + size * 0.18f, cy + size * 0.62f, dR * 2.0f, dR * 2.0f);
    graphics.FillEllipse(&dotBrush, cx + w - size * 0.30f, cy + size * 0.62f, dR * 2.0f, dR * 2.0f);
}

void UIComponents::DrawIconPlus(Graphics& graphics, int x, int y, int size, Color color) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    float cx = x + size / 2.0f;
    float cy = y + size / 2.0f;
    float arm = size * 0.32f;

    Pen pen(color, (std::max)(1.8f, size * 0.12f));
    pen.SetStartCap(LineCapRound);
    pen.SetEndCap(LineCapRound);
    graphics.DrawLine(&pen, cx - arm, cy, cx + arm, cy);
    graphics.DrawLine(&pen, cx, cy - arm, cx, cy + arm);
}

void UIComponents::DrawIconTrash(Graphics& graphics, int x, int y, int size, Color color) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    float cx = x + size / 2.0f;
    float w = size * 0.54f;
    float h = size * 0.58f;
    float bx = cx - w / 2.0f;
    float by = y + size * 0.32f;

    Pen pen(color, (std::max)(1.5f, size * 0.10f));
    pen.SetStartCap(LineCapRound);
    pen.SetEndCap(LineCapRound);

    graphics.DrawLine(&pen, x + size * 0.15f, y + size * 0.25f, x + size * 0.85f, y + size * 0.25f);
    graphics.DrawArc(&pen, cx - size * 0.12f, y + size * 0.12f, size * 0.24f, size * 0.20f, 180, 180);

    GraphicsPath path;
    path.AddLine(bx, by, bx + size * 0.06f, by + h);
    path.AddLine(bx + size * 0.06f, by + h, bx + w - size * 0.06f, by + h);
    path.AddLine(bx + w - size * 0.06f, by + h, bx + w, by);
    graphics.DrawPath(&pen, &path);

    graphics.DrawLine(&pen, cx - size * 0.10f, by + size * 0.12f, cx - size * 0.08f, by + h - size * 0.10f);
    graphics.DrawLine(&pen, cx + size * 0.10f, by + size * 0.12f, cx + size * 0.08f, by + h - size * 0.10f);
}

void UIComponents::DrawIconExternalLink(Graphics& graphics, int x, int y, int size, Color color) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    float boxL = x + size * 0.15f;
    float boxT = y + size * 0.35f;
    float boxW = size * 0.50f;
    float boxH = size * 0.50f;

    Pen pen(color, (std::max)(1.5f, size * 0.10f));
    pen.SetStartCap(LineCapRound);
    pen.SetEndCap(LineCapRound);

    GraphicsPath path;
    path.AddLine(boxL + boxW * 0.50f, boxT, boxL, boxT);
    path.AddLine(boxL, boxT, boxL, boxT + boxH);
    path.AddLine(boxL, boxT + boxH, boxL + boxW, boxT + boxH);
    path.AddLine(boxL + boxW, boxT + boxH, boxL + boxW, boxT + boxH * 0.50f);
    graphics.DrawPath(&pen, &path);

    float ax = x + size * 0.85f;
    float ay = y + size * 0.15f;
    graphics.DrawLine(&pen, boxL + boxW * 0.40f, boxT + boxH * 0.60f, ax, ay);
    graphics.DrawLine(&pen, ax - size * 0.25f, ay, ax, ay);
    graphics.DrawLine(&pen, ax, ay, ax, ay + size * 0.25f);
}

void UIComponents::DrawIconUpdate(Graphics& graphics, int x, int y, int size, Color color) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    float cx = x + size / 2.0f;
    float cy = y + size / 2.0f;
    float r = size * 0.36f;

    Pen arcPen(color, (std::max)(1.6f, size * 0.11f));
    arcPen.SetStartCap(LineCapRound);
    arcPen.SetEndCap(LineCapRound);

    graphics.DrawArc(&arcPen, cx - r, cy - r, r * 2.0f, r * 2.0f, 30, 130);
    graphics.DrawArc(&arcPen, cx - r, cy - r, r * 2.0f, r * 2.0f, 210, 130);

    float a1x = cx + r * cosf(30.0f * 3.14159265f / 180.0f);
    float a1y = cy - r * sinf(30.0f * 3.14159265f / 180.0f);
    PointF arrow1[3] = {
        PointF(a1x - size * 0.18f, a1y - size * 0.08f),
        PointF(a1x + size * 0.04f, a1y),
        PointF(a1x - size * 0.06f, a1y + size * 0.18f)
    };
    SolidBrush brush(color);
    graphics.FillPolygon(&brush, arrow1, 3);

    float a2x = cx - r * cosf(30.0f * 3.14159265f / 180.0f);
    float a2y = cy + r * sinf(30.0f * 3.14159265f / 180.0f);
    PointF arrow2[3] = {
        PointF(a2x + size * 0.18f, a2y + size * 0.08f),
        PointF(a2x - size * 0.04f, a2y),
        PointF(a2x + size * 0.06f, a2y - size * 0.18f)
    };
    graphics.FillPolygon(&brush, arrow2, 3);
}

void UIComponents::DrawIconDownload(Graphics& graphics, int x, int y, int size, Color color) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    float cx = x + size / 2.0f;

    Pen pen(color, (std::max)(1.6f, size * 0.11f));
    pen.SetStartCap(LineCapRound);
    pen.SetEndCap(LineCapRound);

    graphics.DrawLine(&pen, cx, y + size * 0.15f, cx, y + size * 0.65f);
    graphics.DrawLine(&pen, cx - size * 0.22f, y + size * 0.45f, cx, y + size * 0.65f);
    graphics.DrawLine(&pen, cx + size * 0.22f, y + size * 0.45f, cx, y + size * 0.65f);

    graphics.DrawLine(&pen, x + size * 0.18f, y + size * 0.82f, x + size * 0.82f, y + size * 0.82f);
}

void UIComponents::DrawIconCheck(Graphics& graphics, int x, int y, int size, Color color) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    Pen pen(color, (std::max)(1.8f, size * 0.12f));
    pen.SetStartCap(LineCapRound);
    pen.SetEndCap(LineCapRound);

    graphics.DrawLine(&pen, x + size * 0.18f, y + size * 0.52f, x + size * 0.42f, y + size * 0.78f);
    graphics.DrawLine(&pen, x + size * 0.42f, y + size * 0.78f, x + size * 0.84f, y + size * 0.24f);
}

void UIComponents::DrawIconWarning(Graphics& graphics, int x, int y, int size, Color color) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    float cx = x + size / 2.0f;
    float topY = y + size * 0.12f;
    float botY = y + size * 0.88f;

    Pen pen(color, (std::max)(1.6f, size * 0.10f));
    pen.SetStartCap(LineCapRound);
    pen.SetEndCap(LineCapRound);

    GraphicsPath path;
    path.AddLine(cx, topY, x + size * 0.88f, botY);
    path.AddLine(x + size * 0.88f, botY, x + size * 0.12f, botY);
    path.CloseFigure();
    graphics.DrawPath(&pen, &path);

    graphics.DrawLine(&pen, cx, topY + size * 0.25f, cx, topY + size * 0.50f);
    SolidBrush brush(color);
    graphics.FillEllipse(&brush, cx - size * 0.05f, botY - size * 0.16f, size * 0.10f, size * 0.10f);
}

void UIComponents::DrawIconClock(Graphics& graphics, int x, int y, int size, Color color) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    float cx = x + size / 2.0f;
    float cy = y + size / 2.0f;
    float r = size * 0.40f;

    Pen pen(color, (std::max)(1.5f, size * 0.10f));
    pen.SetStartCap(LineCapRound);
    pen.SetEndCap(LineCapRound);
    graphics.DrawEllipse(&pen, cx - r, cy - r, r * 2.0f, r * 2.0f);

    graphics.DrawLine(&pen, cx, cy, cx, cy - size * 0.24f);
    graphics.DrawLine(&pen, cx, cy, cx + size * 0.18f, cy);
}

void UIComponents::DrawIconExport(Graphics& graphics, int x, int y, int size, Color color) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    float cx = x + size / 2.0f;

    Pen pen(color, (std::max)(1.5f, size * 0.10f));
    pen.SetStartCap(LineCapRound);
    pen.SetEndCap(LineCapRound);

    graphics.DrawLine(&pen, cx, y + size * 0.65f, cx, y + size * 0.15f);
    graphics.DrawLine(&pen, cx - size * 0.20f, y + size * 0.35f, cx, y + size * 0.15f);
    graphics.DrawLine(&pen, cx + size * 0.20f, y + size * 0.35f, cx, y + size * 0.15f);

    GraphicsPath tray;
    tray.AddLine(x + size * 0.20f, y + size * 0.55f, x + size * 0.20f, y + size * 0.85f);
    tray.AddLine(x + size * 0.20f, y + size * 0.85f, x + size * 0.80f, y + size * 0.85f);
    tray.AddLine(x + size * 0.80f, y + size * 0.85f, x + size * 0.80f, y + size * 0.55f);
    graphics.DrawPath(&pen, &tray);
}

void UIComponents::DrawIconImport(Graphics& graphics, int x, int y, int size, Color color) {
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    float cx = x + size / 2.0f;

    Pen pen(color, (std::max)(1.5f, size * 0.10f));
    pen.SetStartCap(LineCapRound);
    pen.SetEndCap(LineCapRound);

    graphics.DrawLine(&pen, cx, y + size * 0.15f, cx, y + size * 0.60f);
    graphics.DrawLine(&pen, cx - size * 0.20f, y + size * 0.40f, cx, y + size * 0.60f);
    graphics.DrawLine(&pen, cx + size * 0.20f, y + size * 0.40f, cx, y + size * 0.60f);

    GraphicsPath tray;
    tray.AddLine(x + size * 0.20f, y + size * 0.55f, x + size * 0.20f, y + size * 0.85f);
    tray.AddLine(x + size * 0.20f, y + size * 0.85f, x + size * 0.80f, y + size * 0.85f);
    tray.AddLine(x + size * 0.80f, y + size * 0.85f, x + size * 0.80f, y + size * 0.55f);
    graphics.DrawPath(&pen, &tray);
}

void UIComponents::DrawIconLanguage(Graphics& graphics, int x, int y, int size, Color color) {
    DrawIconGlobe(graphics, x, y, size, color);
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
    const FontFamily* pTextFam = FontManager::GetInstance().GetTextFamily();
    Font font(pTextFam, 10.0f, FontStyleBold, UnitPoint);
    SolidBrush textBrush(textCol);

    if (icon != VectorIcon::None) {
        int iconSize = 14;
        int gap = 8;

        RectF boundRect;
        graphics.MeasureString(text.c_str(), -1, &font, PointF(0, 0), &boundRect);
        float textW = boundRect.Width;
        float totalW = iconSize + gap + textW;
        float startX = x + (w - totalW) / 2.0f;
        if (startX < x + 8.0f) startX = x + 8.0f;

        int iconX = (int)startX;
        int iconY = y + (h - iconSize) / 2;

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

        StringFormat format;
        format.SetAlignment(StringAlignmentNear);
        format.SetLineAlignment(StringAlignmentCenter);
        format.SetFormatFlags(StringFormatFlagsNoWrap);
        RectF textRect(startX + iconSize + gap, (REAL)y, (REAL)(w - (startX - x + iconSize + gap) - 4), (REAL)h);
        graphics.DrawString(text.c_str(), -1, &font, textRect, &format, &textBrush);
    } else {
        StringFormat format;
        format.SetAlignment(StringAlignmentCenter);
        format.SetLineAlignment(StringAlignmentCenter);
        format.SetFormatFlags(StringFormatFlagsNoWrap);
        RectF textRect((REAL)x, (REAL)y, (REAL)w, (REAL)h);
        graphics.DrawString(text.c_str(), -1, &font, textRect, &format, &textBrush);
    }
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

    const FontFamily* pTextFam = FontManager::GetInstance().GetTextFamily();
    Font font(pTextFam, 10.5f, FontStyleRegular, UnitPoint);
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

    const FontFamily* pTextFam = FontManager::GetInstance().GetTextFamily();
    Font font(pTextFam, 10.5f, isSelected ? FontStyleBold : FontStyleRegular, UnitPoint);
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

    const FontFamily* pTextFam = FontManager::GetInstance().GetTextFamily();
    Font font(pTextFam, 9.5f, FontStyleBold, UnitPoint);
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

    // Large vector shield icon
    DrawIconShield(graphics, centerX - 22, centerY - 22, 44, Color(140, 148, 163, 184));

    const FontFamily* pDisplayFam = FontManager::GetInstance().GetDisplayFamily();
    const FontFamily* pTextFam = FontManager::GetInstance().GetTextFamily();

    Font titleFont(pDisplayFam, 12.5f, FontStyleBold, UnitPoint);
    Font subFont(pTextFam, 10.0f, FontStyleRegular, UnitPoint);

    SolidBrush titleBrush(Color(255, 248, 250, 252));
    SolidBrush subBrush(Color(255, 148, 163, 184));

    StringFormat format;
    format.SetAlignment(StringAlignmentCenter);
    format.SetLineAlignment(StringAlignmentCenter);

    RectF titleRect((REAL)x, (REAL)(centerY + 30), (REAL)w, 24.0f);
    RectF subRect((REAL)x, (REAL)(centerY + 56), (REAL)w, 20.0f);

    graphics.DrawString(title.c_str(), -1, &titleFont, titleRect, &format, &titleBrush);
    graphics.DrawString(subtitle.c_str(), -1, &subFont, subRect, &format, &subBrush);
}
