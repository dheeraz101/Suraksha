#include "UnlockDialog.h"
#include "SecurityManager.h"
#include "ConfigManager.h"
#include "UIComponents.h"
#include <windowsx.h>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

#define ID_DLG_BTN_UNLOCK_PIN 2002
#define ID_DLG_BTN_UNLOCK_WIN 2003
#define ID_DLG_BTN_CANCEL     2004

static bool s_authSuccess = false;
static bool s_dialogOpen = false;
static std::wstring s_targetAppName = L"";
static std::wstring s_targetAppPath = L"";
static std::wstring s_errorText = L"";
static std::wstring s_enteredPin = L"";

static int s_dlgHoverId = 0;
static int s_dlgPressedId = 0;
static bool s_closeHover = false;
static bool s_inputFocused = true;

static bool PtInRectStruct(const RECT& r, int x, int y) {
    return (x >= r.left && x <= r.right && y >= r.top && y <= r.bottom);
}

static LRESULT CALLBACK UnlockWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    const auto& settings = ConfigManager::GetInstance().GetSettings();
    bool showPin = (settings.useCustomPin && SecurityManager::GetInstance().HasCustomPin());

    int curY = showPin ? 180 : 130;
    RECT rcBtnPin = { 30, curY, 390, curY + 40 };
    if (showPin) curY += 48;

    RECT rcBtnWin = { 30, curY, 390, curY + 40 };
    if (settings.useWindowsAuth) curY += 48;

    RECT rcBtnCancel = { 30, curY, 390, curY + 36 };
    RECT rcInput = { 30, 122, 390, 164 };

    switch (uMsg) {
    case WM_CREATE: {
        s_authSuccess = false;
        s_errorText = L"";
        s_enteredPin = L"";
        s_closeHover = false;
        s_inputFocused = true;
        SetFocus(hWnd);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1; // Prevent white erasure

    case WM_SETCURSOR:
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return TRUE;

    case WM_CHAR: {
        wchar_t ch = (wchar_t)wParam;
        if (ch == VK_BACK) {
            if (!s_enteredPin.empty()) {
                s_enteredPin.pop_back();
                s_errorText = L"";
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
        else if (ch == VK_RETURN) {
            if (showPin) {
                if (SecurityManager::GetInstance().VerifyCustomPin(s_enteredPin)) {
                    s_authSuccess = true;
                    DestroyWindow(hWnd);
                } else {
                    s_errorText = L"Incorrect Master Passcode! Please try again.";
                    s_enteredPin = L"";
                    InvalidateRect(hWnd, NULL, FALSE);
                }
            }
        }
        else if (ch == VK_ESCAPE) {
            s_authSuccess = false;
            DestroyWindow(hWnd);
        }
        else if (ch >= 32 && ch < 127) {
            if (s_enteredPin.length() < 64) {
                s_enteredPin.push_back(ch);
                s_errorText = L"";
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);

        int oldHover = s_dlgHoverId;
        bool oldCloseHover = s_closeHover;

        s_dlgHoverId = 0;
        s_closeHover = (x >= 18 && x <= 34 && y >= 16 && y <= 32);

        if (showPin && PtInRectStruct(rcBtnPin, x, y)) s_dlgHoverId = ID_DLG_BTN_UNLOCK_PIN;
        else if (settings.useWindowsAuth && PtInRectStruct(rcBtnWin, x, y)) s_dlgHoverId = ID_DLG_BTN_UNLOCK_WIN;
        else if (PtInRectStruct(rcBtnCancel, x, y)) s_dlgHoverId = ID_DLG_BTN_CANCEL;

        if (oldHover != s_dlgHoverId || oldCloseHover != s_closeHover) {
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);

        // Single Red Close Dot
        if (x >= 18 && x <= 34 && y >= 16 && y <= 32) {
            s_authSuccess = false;
            DestroyWindow(hWnd);
            return 0;
        }

        s_dlgPressedId = s_dlgHoverId;
        if (PtInRectStruct(rcInput, x, y)) {
            s_inputFocused = true;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        SetFocus(hWnd);
        return 0;
    }

    case WM_LBUTTONUP: {
        int clickedId = s_dlgHoverId;
        s_dlgPressedId = 0;

        if (clickedId == ID_DLG_BTN_UNLOCK_PIN) {
            if (SecurityManager::GetInstance().VerifyCustomPin(s_enteredPin)) {
                s_authSuccess = true;
                DestroyWindow(hWnd);
            } else {
                s_errorText = L"Incorrect Master Passcode! Please try again.";
                s_enteredPin = L"";
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
        else if (clickedId == ID_DLG_BTN_UNLOCK_WIN) {
            std::wstring errMsg = L"";
            if (SecurityManager::GetInstance().VerifyWindowsCredentials(hWnd, errMsg)) {
                s_authSuccess = true;
                DestroyWindow(hWnd);
            } else {
                if (!errMsg.empty()) {
                    s_errorText = errMsg;
                    InvalidateRect(hWnd, NULL, FALSE);
                }
            }
        }
        else if (clickedId == ID_DLG_BTN_CANCEL) {
            s_authSuccess = false;
            DestroyWindow(hWnd);
        }
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT rect;
        GetClientRect(hWnd, &rect);
        int winW = rect.right - rect.left;
        int winH = rect.bottom - rect.top;

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP hMemBmp = CreateCompatibleBitmap(hdc, winW, winH);
        HBITMAP hOldBmp = (HBITMAP)SelectObject(memDC, hMemBmp);

        // Dark Slate Canvas Background (#141417)
        HBRUSH hBg = CreateSolidBrush(RGB(20, 20, 23));
        FillRect(memDC, &rect, hBg);
        DeleteObject(hBg);

        {
            Graphics graphics(memDC);
            graphics.SetSmoothingMode(SmoothingModeAntiAlias);
            graphics.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

            // Outer Card Border
            Pen borderPen(Color(20, 255, 255, 255), 1.0f);
            graphics.DrawRectangle(&borderPen, 0, 0, winW - 1, winH - 1);

            // Single Red Close Dot
            UIComponents::DrawCloseButton(graphics, 20, 18, 13, s_closeHover);

            // App Logo
            UIComponents::DrawAppLogo(graphics, (winW - 38) / 2, 20, 38);

            // Premium Modern Typography Stack with Segoe UI Variable fallback
            FontFamily fontFamDisplay(L"Segoe UI Variable Display");
            FontFamily fontFamText(L"Segoe UI Variable Text");
            FontFamily fontFamFallback(L"Segoe UI");

            const FontFamily* pDisplayFam = fontFamDisplay.IsAvailable() ? &fontFamDisplay : &fontFamFallback;
            const FontFamily* pTextFam = fontFamText.IsAvailable() ? &fontFamText : &fontFamFallback;

            Font fontTitle(pDisplayFam, 14.0f, FontStyleBold, UnitPoint);
            Font fontSub(pTextFam, 9.5f, FontStyleRegular, UnitPoint);
            Font fontErr(pTextFam, 9.0f, FontStyleBold, UnitPoint);

            SolidBrush whiteBrush(Color(255, 248, 250, 252));
            SolidBrush mutedBrush(Color(255, 148, 163, 184));
            SolidBrush errorBrush(Color(255, 255, 69, 58));

            StringFormat formatCenter;
            formatCenter.SetAlignment(StringAlignmentCenter);
            formatCenter.SetLineAlignment(StringAlignmentCenter);

            // Title
            RectF rcTitle(20.0f, 66.0f, (REAL)(winW - 40), 24.0f);
            graphics.DrawString(L"Application Locked", -1, &fontTitle, rcTitle, &formatCenter, &whiteBrush);

            // Subtitle
            std::wstring subStr = s_targetAppName + L" is protected by Suraksha";
            RectF rcSub(20.0f, 90.0f, (REAL)(winW - 40), 20.0f);
            graphics.DrawString(subStr.c_str(), -1, &fontSub, rcSub, &formatCenter, &mutedBrush);

            // Master Passcode Input Box
            if (showPin) {
                Color inputBorder = s_inputFocused ? Color(255, 10, 132, 255) : Color(20, 255, 255, 255);
                UIComponents::DrawCanvasCard(graphics, rcInput.left, rcInput.top, rcInput.right - rcInput.left, rcInput.bottom - rcInput.top,
                    Color(255, 36, 36, 40), inputBorder, 10);

                // Draw vector key icon inside input box
                UIComponents::DrawIconKey(graphics, rcInput.left + 12, rcInput.top + 13, 16, Color(255, 148, 163, 184));

                std::wstring masked = L"";
                for (size_t i = 0; i < s_enteredPin.length(); ++i) masked += L"\x25CF  "; // Bullet dots

                if (masked.empty()) {
                    RectF rcCue((REAL)(rcInput.left + 36), (REAL)rcInput.top, (REAL)(rcInput.right - rcInput.left - 46), (REAL)(rcInput.bottom - rcInput.top));
                    StringFormat formatLeft;
                    formatLeft.SetAlignment(StringAlignmentNear);
                    formatLeft.SetLineAlignment(StringAlignmentCenter);
                    graphics.DrawString(L"Enter Master Passcode...", -1, &fontSub, rcCue, &formatLeft, &mutedBrush);
                } else {
                    Font fontBullet(pDisplayFam, 12.0f, FontStyleBold, UnitPoint);
                    RectF rcMasked((REAL)(rcInput.left + 36), (REAL)rcInput.top, (REAL)(rcInput.right - rcInput.left - 46), (REAL)(rcInput.bottom - rcInput.top));
                    StringFormat formatLeft;
                    formatLeft.SetAlignment(StringAlignmentNear);
                    formatLeft.SetLineAlignment(StringAlignmentCenter);
                    graphics.DrawString(masked.c_str(), -1, &fontBullet, rcMasked, &formatLeft, &whiteBrush);
                }

                // Unlock with Passcode Button
                bool hovPin = (s_dlgHoverId == ID_DLG_BTN_UNLOCK_PIN);
                bool prsPin = (s_dlgPressedId == ID_DLG_BTN_UNLOCK_PIN);
                UIComponents::DrawCanvasButton(graphics, rcBtnPin.left, rcBtnPin.top, rcBtnPin.right - rcBtnPin.left, rcBtnPin.bottom - rcBtnPin.top,
                    L"Unlock with Master Passcode", ButtonVariant::Primary, hovPin, prsPin, VectorIcon::Key);
            }

            // Windows Password / PIN Button
            if (settings.useWindowsAuth) {
                std::wstring winUser = SecurityManager::GetInstance().GetCurrentWindowsUsername();
                std::wstring winBtnText = L"Unlock with Windows PIN / Hello (" + winUser + L")";
                bool hovWin = (s_dlgHoverId == ID_DLG_BTN_UNLOCK_WIN);
                bool prsWin = (s_dlgPressedId == ID_DLG_BTN_UNLOCK_WIN);
                UIComponents::DrawCanvasButton(graphics, rcBtnWin.left, rcBtnWin.top, rcBtnWin.right - rcBtnWin.left, rcBtnWin.bottom - rcBtnWin.top,
                    winBtnText.c_str(), ButtonVariant::Primary, hovWin, prsWin, VectorIcon::Windows);
            }

            // Cancel Button
            bool hovCan = (s_dlgHoverId == ID_DLG_BTN_CANCEL);
            bool prsCan = (s_dlgPressedId == ID_DLG_BTN_CANCEL);
            UIComponents::DrawCanvasButton(graphics, rcBtnCancel.left, rcBtnCancel.top, rcBtnCancel.right - rcBtnCancel.left, rcBtnCancel.bottom - rcBtnCancel.top,
                L"Cancel", ButtonVariant::Secondary, hovCan, prsCan);

            // Error Text
            if (!s_errorText.empty()) {
                RectF rcErr(20.0f, (REAL)(winH - 28), (REAL)(winW - 40), 20.0f);
                graphics.DrawString(s_errorText.c_str(), -1, &fontErr, rcErr, &formatCenter, &errorBrush);
            }
        }

        BitBlt(hdc, 0, 0, winW, winH, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, hOldBmp);
        DeleteObject(hMemBmp);
        DeleteDC(memDC);

        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_CLOSE:
        s_authSuccess = false;
        DestroyWindow(hWnd);
        return 0;

    case WM_DESTROY:
        s_dialogOpen = false;
        return 0;
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

bool UnlockDialog::Show(HWND hParent, const std::wstring& appName, const std::wstring& appPath) {
    if (s_dialogOpen) return false;

    s_dialogOpen = true;
    s_authSuccess = false;
    s_targetAppName = appName;
    s_targetAppPath = appPath;

    static bool s_classRegistered = false;
    if (!s_classRegistered) {
        WNDCLASSEXW wc = { sizeof(wc) };
        wc.lpfnWndProc = UnlockWndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = L"SurakshaDirectCanvasUnlockClass";
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        RegisterClassExW(&wc);
        s_classRegistered = true;
    }

    const auto& settings = ConfigManager::GetInstance().GetSettings();
    bool showPin = (settings.useCustomPin && SecurityManager::GetInstance().HasCustomPin());

    int dlgWidth = 420;
    int dlgHeight = 150;
    if (showPin) dlgHeight += 98;
    if (settings.useWindowsAuth) dlgHeight += 48;
    dlgHeight += 46; // Cancel button & padding

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenW - dlgWidth) / 2;
    int posY = (screenH - dlgHeight) / 2;

    HWND hwndDlg = CreateWindowExW(
        WS_EX_TOPMOST,
        L"SurakshaDirectCanvasUnlockClass",
        L"Suraksha - Passcode Required",
        WS_POPUP,
        posX, posY, dlgWidth, dlgHeight,
        hParent, NULL, GetModuleHandle(NULL), NULL
    );

    if (!hwndDlg) {
        s_dialogOpen = false;
        return false;
    }

    UIComponents::ApplyRoundedRegion(hwndDlg, 20);

    ShowWindow(hwndDlg, SW_SHOW);
    UpdateWindow(hwndDlg);

    // Force dialog to foreground safely
    HWND hForeground = GetForegroundWindow();
    if (hForeground) {
        DWORD foregroundThread = GetWindowThreadProcessId(hForeground, NULL);
        DWORD currentThread = GetCurrentThreadId();
        if (foregroundThread != currentThread) {
            AttachThreadInput(currentThread, foregroundThread, TRUE);
            BringWindowToTop(hwndDlg);
            SetForegroundWindow(hwndDlg);
            SetFocus(hwndDlg);
            AttachThreadInput(currentThread, foregroundThread, FALSE);
        } else {
            SetForegroundWindow(hwndDlg);
            SetFocus(hwndDlg);
        }
    } else {
        SetForegroundWindow(hwndDlg);
        SetFocus(hwndDlg);
    }

    MSG msg;
    while (IsWindow(hwndDlg) && GetMessageW(&msg, NULL, 0, 0)) {
        if (msg.message == WM_QUIT) {
            PostQuitMessage((int)msg.wParam);
            break;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    s_dialogOpen = false;
    return s_authSuccess;
}
