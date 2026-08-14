#include "UnlockDialog.h"
#include "SecurityManager.h"
#include "ConfigManager.h"
#include "UIComponents.h"
#include <commctrl.h>
#include <gdiplus.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

#define IDC_EDIT_PASS 2001
#define IDC_BTN_UNLOCK_PIN 2002
#define IDC_BTN_UNLOCK_WIN 2003
#define IDC_BTN_CANCEL 2004

static bool s_authSuccess = false;
static bool s_dialogOpen = false;
static std::wstring s_targetAppName = L"";
static std::wstring s_targetAppPath = L"";
static std::wstring s_errorText = L"";

static HBRUSH s_hBgBrush = NULL;
static HBRUSH s_hCardBrush = NULL;
static HFONT s_hFontTitle = NULL;
static HFONT s_hFontMain = NULL;
static HFONT s_hFontSub = NULL;

static LRESULT CALLBACK UnlockWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        s_hBgBrush = CreateSolidBrush(RGB(20, 20, 23));     // macOS Dark Slate (#141417)
        s_hCardBrush = CreateSolidBrush(RGB(44, 44, 46));   // macOS Card Dark (#2C2C2E)

        s_hFontTitle = CreateFontW(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        s_hFontMain = CreateFontW(14, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        s_hFontSub = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

        const auto& settings = ConfigManager::GetInstance().GetSettings();
        std::wstring winUser = SecurityManager::GetInstance().GetCurrentWindowsUsername();

        int topY = 115;
        if (settings.useCustomPin && SecurityManager::GetInstance().HasCustomPin()) {
            HWND hEdit = CreateWindowExW(0, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_PASSWORD | ES_AUTOHSCROLL | ES_CENTER,
                30, topY, 340, 36, hWnd, (HMENU)IDC_EDIT_PASS, GetModuleHandle(NULL), NULL);
            UIComponents::ApplyRoundedRegion(hEdit, 8);
            SendMessageW(hEdit, EM_SETCUEBANNER, TRUE, (LPARAM)L"Enter Master Passcode");
            SendMessageW(hEdit, WM_SETFONT, (WPARAM)s_hFontMain, TRUE);
            SetFocus(hEdit);
            topY += 48;

            HWND hBtnPin = CreateWindowExW(0, L"BUTTON", L"Unlock with Master Passcode",
                WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                30, topY, 340, 38, hWnd, (HMENU)IDC_BTN_UNLOCK_PIN, GetModuleHandle(NULL), NULL);
            UIComponents::ApplyRoundedRegion(hBtnPin, 10);
            topY += 48;
        }

        if (settings.useWindowsAuth) {
            std::wstring winBtnText = L"Unlock with Windows Password / PIN (" + winUser + L")";
            HWND hBtnWin = CreateWindowExW(0, L"BUTTON", winBtnText.c_str(),
                WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                30, topY, 340, 38, hWnd, (HMENU)IDC_BTN_UNLOCK_WIN, GetModuleHandle(NULL), NULL);
            UIComponents::ApplyRoundedRegion(hBtnWin, 10);
            topY += 48;
        }

        HWND hBtnCancel = CreateWindowExW(0, L"BUTTON", L"Cancel",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            30, topY, 340, 34, hWnd, (HMENU)IDC_BTN_CANCEL, GetModuleHandle(NULL), NULL);
        UIComponents::ApplyRoundedRegion(hBtnCancel, 10);

        s_errorText = L"";
        return 0;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(248, 250, 252));
        return (INT_PTR)s_hBgBrush;
    }
    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, OPAQUE);
        SetBkColor(hdc, RGB(44, 44, 46));
        SetTextColor(hdc, RGB(255, 255, 255));
        return (INT_PTR)s_hCardBrush;
    }
    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lParam;
        wchar_t btnText[128] = { 0 };
        GetWindowTextW(pdis->hwndItem, btnText, 128);

        ButtonVariant variant = ButtonVariant::Secondary;
        if (pdis->CtlID == IDC_BTN_UNLOCK_PIN || pdis->CtlID == IDC_BTN_UNLOCK_WIN) {
            variant = ButtonVariant::Primary;
        }

        UIComponents::DrawButton(pdis->hDC, pdis, btnText, variant, Color(255, 20, 20, 23));
        return TRUE;
    }
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        if (wmId == IDC_BTN_UNLOCK_PIN) {
            wchar_t passBuf[128] = { 0 };
            GetDlgItemTextW(hWnd, IDC_EDIT_PASS, passBuf, 128);
            if (SecurityManager::GetInstance().VerifyCustomPin(passBuf)) {
                s_authSuccess = true;
                DestroyWindow(hWnd);
            } else {
                s_errorText = L"Incorrect Master Passcode! Please try again.";
                InvalidateRect(hWnd, NULL, TRUE);
            }
        }
        else if (wmId == IDC_BTN_UNLOCK_WIN) {
            std::wstring errMsg = L"";
            if (SecurityManager::GetInstance().VerifyWindowsCredentials(hWnd, errMsg)) {
                s_authSuccess = true;
                DestroyWindow(hWnd);
            } else {
                if (!errMsg.empty()) {
                    s_errorText = errMsg;
                    InvalidateRect(hWnd, NULL, TRUE);
                }
            }
        }
        else if (wmId == IDC_BTN_CANCEL) {
            s_authSuccess = false;
            DestroyWindow(hWnd);
        }
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        Graphics graphics(hdc);
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);

        RECT rect;
        GetClientRect(hWnd, &rect);
        SolidBrush bgBrush(Color(255, 20, 20, 23));
        Rect clientRect((int)rect.left, (int)rect.top, (int)(rect.right - rect.left), (int)(rect.bottom - rect.top));
        graphics.FillRectangle(&bgBrush, clientRect);

        // Draw Card Outer Border
        Pen borderPen(Color(255, 255, 255, 25), 1.0f);
        graphics.DrawRectangle(&borderPen, 0, 0, (int)(rect.right - 1), (int)(rect.bottom - 1));

        // Vector Smooth Anti-Aliased macOS Traffic Lights
        UIComponents::DrawTrafficLights(graphics, 20, 18);

        // Header Title
        SelectObject(hdc, s_hFontTitle);
        SetTextColor(hdc, RGB(255, 255, 255));
        SetBkMode(hdc, TRANSPARENT);
        RECT rcTitle = { 20, 38, 380, 68 };
        DrawTextW(hdc, L"Application Locked", -1, &rcTitle, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

        // Subtitle / Target app name
        SelectObject(hdc, s_hFontMain);
        SetTextColor(hdc, RGB(161, 161, 170));
        std::wstring subStr = s_targetAppName + L" requires authorization to unlock";
        RECT rcSub = { 20, 70, 380, 95 };
        DrawTextW(hdc, subStr.c_str(), -1, &rcSub, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

        // Error message
        if (!s_errorText.empty()) {
            SelectObject(hdc, s_hFontSub);
            SetTextColor(hdc, RGB(255, 69, 58)); // Apple Red
            RECT rcErr = { 20, rect.bottom - 36, 380, rect.bottom - 10 };
            DrawTextW(hdc, s_errorText.c_str(), -1, &rcErr, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        }

        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_CLOSE:
        s_authSuccess = false;
        DestroyWindow(hWnd);
        return 0;
    case WM_DESTROY: {
        if (s_hBgBrush) { DeleteObject(s_hBgBrush); s_hBgBrush = NULL; }
        if (s_hCardBrush) { DeleteObject(s_hCardBrush); s_hCardBrush = NULL; }
        if (s_hFontTitle) { DeleteObject(s_hFontTitle); s_hFontTitle = NULL; }
        if (s_hFontMain) { DeleteObject(s_hFontMain); s_hFontMain = NULL; }
        if (s_hFontSub) { DeleteObject(s_hFontSub); s_hFontSub = NULL; }
        s_dialogOpen = false;
        return 0;
    }
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

bool UnlockDialog::Show(HWND hParent, const std::wstring& appName, const std::wstring& appPath) {
    if (s_dialogOpen) return false;

    s_dialogOpen = true;
    s_authSuccess = false;
    s_targetAppName = appName;
    s_targetAppPath = appPath;

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = UnlockWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"SurakshaMacUnlockClass";
    wc.hbrBackground = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassExW(&wc);

    int dlgWidth = 400;
    int dlgHeight = 350;

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenW - dlgWidth) / 2;
    int posY = (screenH - dlgHeight) / 2;

    HWND hwndDlg = CreateWindowExW(
        WS_EX_TOPMOST,
        wc.lpszClassName,
        L"Suraksha — Passcode Required",
        WS_POPUP,
        posX, posY, dlgWidth, dlgHeight,
        hParent, NULL, GetModuleHandle(NULL), NULL
    );

    if (!hwndDlg) {
        s_dialogOpen = false;
        return false;
    }

    UIComponents::ApplyRoundedRegion(hwndDlg, 14);

    ShowWindow(hwndDlg, SW_SHOW);
    UpdateWindow(hwndDlg);
    SetForegroundWindow(hwndDlg);

    MSG msg;
    while (IsWindow(hwndDlg) && GetMessageW(&msg, NULL, 0, 0)) {
        if (msg.message == WM_QUIT) {
            PostQuitMessage((int)msg.wParam);
            break;
        }
        if (!IsDialogMessageW(hwndDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    UnregisterClassW(wc.lpszClassName, GetModuleHandle(NULL));
    s_dialogOpen = false;
    return s_authSuccess;
}

