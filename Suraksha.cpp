// Suraksha.cpp : Entry point for Suraksha Windows App Locker (100% Enterprise Edition)
#include "framework.h"
#include "Suraksha.h"
#include "Resource.h"
#include "ConfigManager.h"
#include "SecurityManager.h"
#include "AppLockEngine.h"
#include "TrayIcon.h"
#include "UIComponents.h"
#include "AuditLogger.h"

#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <objbase.h>
#include <gdiplus.h>
#include <vector>
#include <string>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shell32.lib")

using namespace Gdiplus;

#define MAX_LOADSTRING 100

// Control IDs for Direct Canvas Hit Testing
#define ID_CANVAS_BTN_ADD          3001
#define ID_CANVAS_BTN_REMOVE       3002
#define ID_CANVAS_PRESET_NOTEPAD   3003
#define ID_CANVAS_PRESET_CHROME    3004
#define ID_CANVAS_PRESET_CMD       3005
#define ID_CANVAS_PRESET_CALC      3006
#define ID_CANVAS_TOGGLE_ENABLE    3007
#define ID_CANVAS_TOGGLE_WINAUTH   3008
#define ID_CANVAS_TOGGLE_CUSTOMPIN 3009
#define ID_CANVAS_TOGGLE_AUTOSTART 3010
#define ID_CANVAS_BTN_SETPIN       3011
#define ID_CANVAS_BTN_ABOUT        3012

// Global System Hotkey IDs
#define HOTKEY_LOCKALL   101 // Ctrl + Alt + L
#define HOTKEY_TOGGLEWIN 102 // Ctrl + Alt + S

// Global Variables:
HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];
HWND g_hWndMain = NULL;
UINT_PTR g_nTimerID = 1001;
ULONG_PTR g_gdiplusToken = 0;

// Interactive Canvas State
int g_hoverControlId = 0;
int g_pressedControlId = 0;
int g_selectedListIdx = -1;
int g_hoverListIdx = -1;

// Forward declarations
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void ApplyDarkThemeToWindow(HWND hWnd);
void UpdateTrayIconMetrics(HWND hWnd);
void PromptShowAboutModal(HWND hWndParent);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Single Instance Guard
    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"Global\\SurakshaAppLockerMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND hExisting = FindWindowW(L"SURAKSHA_APP_LOCKER", NULL);
        if (hExisting) {
            ShowWindow(hExisting, SW_RESTORE);
            SetForegroundWindow(hExisting);
        }
        return 0;
    }

    // Initialize Audit Logger Engine
    AuditLogger::GetInstance().Init();

    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);

    // Load Settings
    ConfigManager::GetInstance().LoadSettings();

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    wcscpy_s(szWindowClass, L"SURAKSHA_APP_LOCKER");
    MyRegisterClass(hInstance);

    if (!InitInstance (hInstance, nCmdShow)) {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SURAKSHA));
    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    if (hMutex) CloseHandle(hMutex);
    GdiplusShutdown(g_gdiplusToken);
    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SURAKSHA));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)GetStockObject(BLACK_BRUSH); // Black background to prevent white flash!
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

void ApplyDarkThemeToWindow(HWND hWnd) {
    BOOL useDarkMode = TRUE;
    DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));

    COLORREF darkBorder = RGB(36, 36, 40); // Dark Slate DWM Window Border!
    DwmSetWindowAttribute(hWnd, DWMWA_BORDER_COLOR, &darkBorder, sizeof(darkBorder));

    // Extend DWM Frame Margins for native glass shadow effects
    MARGINS margins = { 1, 1, 1, 1 };
    DwmExtendFrameIntoClientArea(hWnd, &margins);
}

void UpdateTrayIconMetrics(HWND hWnd) {
    const auto& settings = ConfigManager::GetInstance().GetSettings();
    int count = (int)settings.lockedApps.size();
    std::wstring statusStr = settings.protectionEnabled ? L"Active" : L"Paused";
    std::wstring tip = L"Suraksha \x2014 " + std::to_wstring(count) + L" Apps Protected | Status: " + statusStr;
    TrayIcon::GetInstance().UpdateTooltip(hWnd, tip);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
    hInst = hInstance;

    int winWidth = 780;
    int winHeight = 540;

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenW - winWidth) / 2;
    int posY = (screenH - winHeight) / 2;

    HWND hWnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        szWindowClass,
        L"Suraksha \x2014 Privacy \x0026 Security",
        WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX,
        posX, posY, winWidth, winHeight,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hWnd) return FALSE;

    g_hWndMain = hWnd;
    ApplyDarkThemeToWindow(hWnd);
    UIComponents::ApplyRoundedRegion(hWnd, 14);

    // Register Enterprise Global System Hotkeys
    RegisterHotKey(hWnd, HOTKEY_LOCKALL, MOD_CONTROL | MOD_ALT, 'L');  // Ctrl + Alt + L
    RegisterHotKey(hWnd, HOTKEY_TOGGLEWIN, MOD_CONTROL | MOD_ALT, 'S'); // Ctrl + Alt + S

    // Create System Tray Icon
    HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SURAKSHA));
    if (!hIcon) hIcon = LoadIcon(NULL, IDI_SHIELD);
    TrayIcon::GetInstance().Create(hWnd, 1, hIcon, L"Suraksha \x2014 Privacy \x0026 Security");
    UpdateTrayIconMetrics(hWnd);

    // Start App Monitoring Engine
    AppLockEngine::GetInstance().StartMonitoring(hWnd);

    // Set 50ms periodic timer for near-instant process scan!
    SetTimer(hWnd, g_nTimerID, 50, NULL);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

// ---------------- Modern macOS About Suraksha Modal ----------------
#define IDC_BTN_ABOUT_YABP 5001
#define IDC_BTN_ABOUT_DEV  5002
#define IDC_BTN_ABOUT_GIT  5003
#define IDC_BTN_ABOUT_OK   5004

static LRESULT CALLBACK AboutDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        HWND hBtnYABP = CreateWindowExW(0, L"BUTTON", L"YABP Website",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 25, 340, 110, 36, hWnd, (HMENU)IDC_BTN_ABOUT_YABP, GetModuleHandle(NULL), NULL);
        UIComponents::ApplyRoundedRegion(hBtnYABP, 8);

        HWND hBtnDev = CreateWindowExW(0, L"BUTTON", L"Developer",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 145, 340, 100, 36, hWnd, (HMENU)IDC_BTN_ABOUT_DEV, GetModuleHandle(NULL), NULL);
        UIComponents::ApplyRoundedRegion(hBtnDev, 8);

        HWND hBtnGit = CreateWindowExW(0, L"BUTTON", L"GitHub Repo",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 255, 340, 100, 36, hWnd, (HMENU)IDC_BTN_ABOUT_GIT, GetModuleHandle(NULL), NULL);
        UIComponents::ApplyRoundedRegion(hBtnGit, 8);

        HWND hBtnClose = CreateWindowExW(0, L"BUTTON", L"Close",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 365, 340, 70, 36, hWnd, (HMENU)IDC_BTN_ABOUT_OK, GetModuleHandle(NULL), NULL);
        UIComponents::ApplyRoundedRegion(hBtnClose, 8);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lParam;
        wchar_t btnText[128] = { 0 };
        GetWindowTextW(pdis->hwndItem, btnText, 128);

        ButtonVariant variant = ButtonVariant::Secondary;
        if (pdis->CtlID == IDC_BTN_ABOUT_YABP) variant = ButtonVariant::Primary;
        else if (pdis->CtlID == IDC_BTN_ABOUT_DEV || pdis->CtlID == IDC_BTN_ABOUT_GIT) variant = ButtonVariant::Secondary;

        UIComponents::DrawButton(pdis->hDC, pdis, btnText, variant, Color(255, 20, 20, 23));
        return TRUE;
    }
    case WM_NCHITTEST: {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(hWnd, &pt);
        if (pt.y <= 40) return HTCAPTION;
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
    case WM_LBUTTONDOWN: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        if (x >= 20 && x <= 32 && y >= 18 && y <= 30) {
            DestroyWindow(hWnd);
            return 0;
        }
        break;
    }
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        if (wmId == IDC_BTN_ABOUT_YABP) {
            ShellExecuteW(NULL, L"open", L"https://yabp.netlify.app/", NULL, NULL, SW_SHRECOVERY || SW_SHOWNORMAL);
        }
        else if (wmId == IDC_BTN_ABOUT_DEV) {
            ShellExecuteW(NULL, L"open", L"https://dheeraz.dpdns.org/", NULL, NULL, SW_SHOWNORMAL);
        }
        else if (wmId == IDC_BTN_ABOUT_GIT) {
            ShellExecuteW(NULL, L"open", L"https://github.com/dheeraz101", NULL, NULL, SW_SHOWNORMAL);
        }
        else if (wmId == IDC_BTN_ABOUT_OK) {
            DestroyWindow(hWnd);
        }
        break;
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

        HBRUSH hBg = CreateSolidBrush(RGB(20, 20, 23));
        FillRect(memDC, &rect, hBg);
        DeleteObject(hBg);

        {
            Graphics graphics(memDC);
            graphics.SetSmoothingMode(SmoothingModeAntiAlias);

            // Draw Card Outer Border
            Pen borderPen(Color(255, 255, 255, 25), 1.0f);
            graphics.DrawRectangle(&borderPen, 0, 0, (int)(rect.right - 1), (int)(rect.bottom - 1));

            // Red Close Dot
            SolidBrush redBrush(Color(255, 255, 95, 86));
            graphics.FillEllipse(&redBrush, 20, 18, 12, 12);

            // Draw Green 'S' Padlock Logo
            UIComponents::DrawAppLogo(graphics, 210, 48, 44);

            FontFamily fontFamily(L"Segoe UI");
            Font fontTitle(&fontFamily, 14.0f, FontStyleBold, UnitPoint);
            Font fontSub(&fontFamily, 10.5f, FontStyleBold, UnitPoint);
            Font fontBody(&fontFamily, 9.5f, FontStyleRegular, UnitPoint);
            Font fontMuted(&fontFamily, 8.5f, FontStyleRegular, UnitPoint);

            SolidBrush whiteBrush(Color(255, 248, 250, 252));
            SolidBrush blueBrush(Color(255, 10, 132, 255));
            SolidBrush greenBrush(Color(255, 52, 199, 89));
            SolidBrush mutedBrush(Color(255, 148, 163, 184));

            StringFormat formatCenter;
            formatCenter.SetAlignment(StringAlignmentCenter);
            formatCenter.SetLineAlignment(StringAlignmentCenter);

            // App Title
            RectF rcT(20.0f, 104.0f, 420.0f, 26.0f);
            graphics.DrawString(L"Suraksha \x2014 Privacy \x0026 Security", -1, &fontTitle, rcT, &formatCenter, &whiteBrush);

            // YABP Note
            RectF rcYABP(20.0f, 132.0f, 420.0f, 22.0f);
            graphics.DrawString(L"An YABP Initiative  (Yet Another Boring Project)", -1, &fontSub, rcYABP, &formatCenter, &blueBrush);

            // Developer Info
            RectF rcDev(20.0f, 162.0f, 420.0f, 20.0f);
            graphics.DrawString(L"Developed with \x2764 by Dheeraz", -1, &fontBody, rcDev, &formatCenter, &greenBrush);

            // License Title & Card Box
            UIComponents::DrawCanvasCard(graphics, 25, 195, 410, 125, Color(255, 26, 26, 30), Color(255, 255, 255, 15), 10);

            StringFormat formatLeft;
            formatLeft.SetAlignment(StringAlignmentNear);
            formatLeft.SetLineAlignment(StringAlignmentNear);

            RectF rcLicHead(40.0f, 205.0f, 380.0f, 20.0f);
            graphics.DrawString(L"GNU General Public License v3.0 (GPLv3)", -1, &fontSub, rcLicHead, &formatLeft, &whiteBrush);

            std::wstring licBody = L"This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.\n\nThis program is distributed WITHOUT ANY WARRANTY.";
            RectF rcLicBody(40.0f, 230.0f, 380.0f, 80.0f);
            graphics.DrawString(licBody.c_str(), -1, &fontMuted, rcLicBody, &formatLeft, &mutedBrush);
        }

        BitBlt(hdc, 0, 0, winW, winH, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, hOldBmp);
        DeleteObject(hMemBmp);
        DeleteDC(memDC);

        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

void PromptShowAboutModal(HWND hWndParent) {
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = AboutDialogProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"SurakshaAboutClass";
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassExW(&wc);

    HWND hwndDlg = CreateWindowExW(
        WS_EX_TOPMOST,
        wc.lpszClassName, L"About Suraksha",
        WS_POPUP,
        (GetSystemMetrics(SM_CXSCREEN) - 460) / 2,
        (GetSystemMetrics(SM_CYSCREEN) - 400) / 2,
        460, 400, hWndParent, NULL, GetModuleHandle(NULL), NULL
    );

    if (hwndDlg) {
        UIComponents::ApplyRoundedRegion(hwndDlg, 14);
        ShowWindow(hwndDlg, SW_SHOW);
        UpdateWindow(hwndDlg);
        MSG msg;
        while (GetMessageW(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    UnregisterClassW(wc.lpszClassName, GetModuleHandle(NULL));
}

// ---------------- Modern Frameless Master Passcode Modal (Direct Canvas) ----------------
static LRESULT CALLBACK SetPinDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        HWND hEdit1 = CreateWindowExW(0, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_PASSWORD | ES_CENTER, 30, 70, 280, 36, hWnd, (HMENU)1001, GetModuleHandle(NULL), NULL);
        HWND hEdit2 = CreateWindowExW(0, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_PASSWORD | ES_CENTER, 30, 118, 280, 36, hWnd, (HMENU)1002, GetModuleHandle(NULL), NULL);

        UIComponents::ApplyRoundedRegion(hEdit1, 8);
        UIComponents::ApplyRoundedRegion(hEdit2, 8);

        SendMessageW(hEdit1, EM_SETCUEBANNER, TRUE, (LPARAM)L"Enter New Passcode");
        SendMessageW(hEdit2, EM_SETCUEBANNER, TRUE, (LPARAM)L"Confirm New Passcode");

        HWND hBtnSave = CreateWindowExW(0, L"BUTTON", L"Save Master Passcode",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 30, 172, 280, 42, hWnd, (HMENU)IDOK, GetModuleHandle(NULL), NULL);
        UIComponents::ApplyRoundedRegion(hBtnSave, 10);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1; // Prevent white erasure!
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(248, 250, 252));
        static HBRUSH hStaticBg = CreateSolidBrush(RGB(20, 20, 23));
        return (INT_PTR)hStaticBg;
    }
    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, OPAQUE);
        SetBkColor(hdc, RGB(44, 44, 46));
        SetTextColor(hdc, RGB(255, 255, 255));
        static HBRUSH hEditBg = CreateSolidBrush(RGB(44, 44, 46));
        return (INT_PTR)hEditBg;
    }
    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lParam;
        wchar_t btnText[128] = { 0 };
        GetWindowTextW(pdis->hwndItem, btnText, 128);
        UIComponents::DrawButton(pdis->hDC, pdis, btnText, ButtonVariant::Primary, Color(255, 20, 20, 23));
        return TRUE;
    }
    case WM_NCHITTEST: {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(hWnd, &pt);
        if (pt.y <= 40) return HTCAPTION;
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
    case WM_LBUTTONDOWN: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        if (x >= 20 && x <= 32 && y >= 18 && y <= 30) {
            DestroyWindow(hWnd);
            return 0;
        }
        break;
    }
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        if (wmId == IDOK) {
            wchar_t pin1[128] = { 0 };
            wchar_t pin2[128] = { 0 };
            GetDlgItemTextW(hWnd, 1001, pin1, 128);
            GetDlgItemTextW(hWnd, 1002, pin2, 128);

            if (wcslen(pin1) == 0) {
                MessageBoxW(hWnd, L"Passcode cannot be empty.", L"Suraksha", MB_OK | MB_ICONWARNING);
                return 0;
            }
            if (wcscmp(pin1, pin2) != 0) {
                MessageBoxW(hWnd, L"Passcodes do not match.", L"Suraksha", MB_OK | MB_ICONWARNING);
                return 0;
            }
            SecurityManager::GetInstance().SetCustomPin(pin1);
            MessageBoxW(hWnd, L"Master Passcode successfully updated!", L"Suraksha", MB_OK | MB_ICONINFORMATION);
            DestroyWindow(hWnd);
        }
        break;
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

        HBRUSH hBg = CreateSolidBrush(RGB(20, 20, 23));
        FillRect(memDC, &rect, hBg);
        DeleteObject(hBg);

        {
            Graphics graphics(memDC);
            graphics.SetSmoothingMode(SmoothingModeAntiAlias);

            // Draw Card Outer Border
            Pen borderPen(Color(255, 255, 255, 25), 1.0f);
            graphics.DrawRectangle(&borderPen, 0, 0, (int)(rect.right - 1), (int)(rect.bottom - 1));

            // Single Red Close Dot
            SolidBrush redBrush(Color(255, 255, 95, 86));
            graphics.FillEllipse(&redBrush, 20, 18, 12, 12);

            // Header Title
            FontFamily fontFamily(L"Segoe UI");
            Font font(&fontFamily, 12.0f, FontStyleBold, UnitPoint);
            SolidBrush textBrush(Color(255, 248, 250, 252));

            StringFormat format;
            format.SetAlignment(StringAlignmentNear);
            format.SetLineAlignment(StringAlignmentCenter);

            RectF titleRect(45.0f, 12.0f, 270.0f, 26.0f);
            graphics.DrawString(L"Set Master Passcode", -1, &font, titleRect, &format, &textBrush);
        }

        BitBlt(hdc, 0, 0, winW, winH, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, hOldBmp);
        DeleteObject(hMemBmp);
        DeleteDC(memDC);

        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

void PromptSetCustomPin(HWND hWndParent) {
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = SetPinDialogProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"SurakshaSetPinClass";
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassExW(&wc);

    HWND hwndDlg = CreateWindowExW(
        WS_EX_TOPMOST,
        wc.lpszClassName, L"Set Master Passcode",
        WS_POPUP,
        (GetSystemMetrics(SM_CXSCREEN) - 340) / 2,
        (GetSystemMetrics(SM_CYSCREEN) - 240) / 2,
        340, 240, hWndParent, NULL, GetModuleHandle(NULL), NULL
    );

    if (hwndDlg) {
        UIComponents::ApplyRoundedRegion(hwndDlg, 14);
        ShowWindow(hwndDlg, SW_SHOW);
        UpdateWindow(hwndDlg);
        MSG msg;
        while (GetMessageW(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    UnregisterClassW(wc.lpszClassName, GetModuleHandle(NULL));
}

// Helper: Check if point is inside RECT
static bool PtInRectStruct(const RECT& r, int x, int y) {
    return (x >= r.left && x <= r.right && y >= r.top && y <= r.bottom);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    // Canvas Control Coordinates Definition (Adjusted for 330px width!)
    const RECT rcBtnAdd          = { 30, 420, 190, 458 };
    const RECT rcBtnRemove       = { 200, 420, 360, 458 };

    // Distributed Presets across 330px width (x: 30 to 360)
    const RECT rcPreset1         = { 30, 468, 108, 496 };  // + Notepad (78px)
    const RECT rcPreset2         = { 114, 468, 190, 496 }; // + Chrome (76px)
    const RECT rcPreset3         = { 196, 468, 274, 496 }; // + Terminal (78px)
    const RECT rcPreset4         = { 280, 468, 360, 496 }; // + Calculator (80px)

    const RECT rcToggleEnable    = { 415, 120, 735, 154 };
    const RECT rcToggleWinAuth   = { 415, 175, 735, 209 };
    const RECT rcToggleCustomPin = { 415, 230, 735, 264 };
    const RECT rcBtnSetPin       = { 415, 280, 715, 318 };
    const RECT rcToggleAutoStart = { 415, 335, 735, 369 };

    const RECT rcBtnAbout        = { 520, 14, 595, 42 };

    switch (message) {
    case WM_CREATE:
        return 0;

    case WM_ERASEBKGND:
        return 1; // Prevent white background erasure flash!

    case WM_HOTKEY: {
        if (wParam == HOTKEY_LOCKALL) { // Ctrl + Alt + L
            AppLockEngine::GetInstance().LockAllProcesses();
            AuditLogger::GetInstance().LogEvent(L"HOTKEY_TRIGGERED", L"Global Hotkey Ctrl+Alt+L triggered: All protected application sessions locked.");
            MessageBoxW(hWnd, L"All protected application sessions locked!", L"Suraksha Security", MB_OK | MB_ICONINFORMATION);
        }
        else if (wParam == HOTKEY_TOGGLEWIN) { // Ctrl + Alt + S
            if (IsWindowVisible(hWnd)) {
                ShowWindow(hWnd, SW_HIDE);
            } else {
                ShowWindow(hWnd, SW_RESTORE);
                SetForegroundWindow(hWnd);
            }
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);

        int oldHover = g_hoverControlId;
        int oldHoverList = g_hoverListIdx;

        g_hoverControlId = 0;
        g_hoverListIdx = -1;

        if (PtInRectStruct(rcBtnAdd, x, y)) g_hoverControlId = ID_CANVAS_BTN_ADD;
        else if (PtInRectStruct(rcBtnRemove, x, y)) g_hoverControlId = ID_CANVAS_BTN_REMOVE;
        else if (PtInRectStruct(rcPreset1, x, y)) g_hoverControlId = ID_CANVAS_PRESET_NOTEPAD;
        else if (PtInRectStruct(rcPreset2, x, y)) g_hoverControlId = ID_CANVAS_PRESET_CHROME;
        else if (PtInRectStruct(rcPreset3, x, y)) g_hoverControlId = ID_CANVAS_PRESET_CMD;
        else if (PtInRectStruct(rcPreset4, x, y)) g_hoverControlId = ID_CANVAS_PRESET_CALC;
        else if (PtInRectStruct(rcToggleEnable, x, y)) g_hoverControlId = ID_CANVAS_TOGGLE_ENABLE;
        else if (PtInRectStruct(rcToggleWinAuth, x, y)) g_hoverControlId = ID_CANVAS_TOGGLE_WINAUTH;
        else if (PtInRectStruct(rcToggleCustomPin, x, y)) g_hoverControlId = ID_CANVAS_TOGGLE_CUSTOMPIN;
        else if (PtInRectStruct(rcBtnSetPin, x, y)) g_hoverControlId = ID_CANVAS_BTN_SETPIN;
        else if (PtInRectStruct(rcToggleAutoStart, x, y)) g_hoverControlId = ID_CANVAS_TOGGLE_AUTOSTART;
        else if (PtInRectStruct(rcBtnAbout, x, y)) g_hoverControlId = ID_CANVAS_BTN_ABOUT;

        // ListBox Item Hover Hit Test (x: 35-355, y: 105-405)
        if (x >= 35 && x <= 355 && y >= 105 && y <= 405) {
            const auto& lockedApps = ConfigManager::GetInstance().GetSettings().lockedApps;
            int itemY = 105;
            for (size_t i = 0; i < lockedApps.size(); ++i) {
                if (y >= itemY && y <= itemY + 36) {
                    g_hoverListIdx = (int)i;
                    break;
                }
                itemY += 40;
            }
        }

        if (oldHover != g_hoverControlId || oldHoverList != g_hoverListIdx) {
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);

        // Window Traffic Light Controls (Close/Minimize/Center)
        if (x >= 20 && x <= 32 && y >= 20 && y <= 32) {
            ShowWindow(hWnd, SW_HIDE);
            return 0;
        }
        else if (x >= 38 && x <= 50 && y >= 20 && y <= 32) {
            ShowWindow(hWnd, SW_HIDE);
            return 0;
        }
        else if (x >= 56 && x <= 68 && y >= 20 && y <= 32) {
            int screenW = GetSystemMetrics(SM_CXSCREEN);
            int screenH = GetSystemMetrics(SM_CYSCREEN);
            SetWindowPos(hWnd, NULL, (screenW - 780) / 2, (screenH - 540) / 2, 780, 540, SWP_NOZORDER);
            return 0;
        }

        g_pressedControlId = g_hoverControlId;

        // List Item Selection
        if (g_hoverListIdx != -1) {
            g_selectedListIdx = g_hoverListIdx;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_LBUTTONUP: {
        int clickedId = g_hoverControlId;
        g_pressedControlId = 0;

        if (clickedId == ID_CANVAS_BTN_ABOUT) {
            PromptShowAboutModal(hWnd);
        }
        else if (clickedId == ID_CANVAS_TOGGLE_ENABLE) {
            auto& settings = ConfigManager::GetInstance().GetSettings();
            settings.protectionEnabled = !settings.protectionEnabled;
            ConfigManager::GetInstance().SaveSettings();
            UpdateTrayIconMetrics(hWnd);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (clickedId == ID_CANVAS_TOGGLE_WINAUTH) {
            auto& settings = ConfigManager::GetInstance().GetSettings();
            settings.useWindowsAuth = !settings.useWindowsAuth;
            ConfigManager::GetInstance().SaveSettings();
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (clickedId == ID_CANVAS_TOGGLE_CUSTOMPIN) {
            auto& settings = ConfigManager::GetInstance().GetSettings();
            settings.useCustomPin = !settings.useCustomPin;
            ConfigManager::GetInstance().SaveSettings();
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (clickedId == ID_CANVAS_TOGGLE_AUTOSTART) {
            auto& settings = ConfigManager::GetInstance().GetSettings();
            bool newAutoStart = !settings.autoStartWithWindows;
            ConfigManager::GetInstance().SetAutoStart(newAutoStart);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (clickedId == ID_CANVAS_BTN_SETPIN) {
            PromptSetCustomPin(hWnd);
        }
        else if (clickedId == ID_CANVAS_BTN_ADD) {
            wchar_t szFile[MAX_PATH] = { 0 };
            OPENFILENAMEW ofn = { sizeof(ofn) };
            ofn.hwndOwner = hWnd;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
            ofn.lpstrFilter = L"Executable Files (*.exe)\0*.exe\0All Files (*.*)\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

            if (GetOpenFileNameW(&ofn)) {
                ConfigManager::GetInstance().AddLockedApp(szFile);
                UpdateTrayIconMetrics(hWnd);
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
        else if (clickedId == ID_CANVAS_BTN_REMOVE) {
            const auto& lockedApps = ConfigManager::GetInstance().GetSettings().lockedApps;
            if (g_selectedListIdx >= 0 && g_selectedListIdx < (int)lockedApps.size()) {
                std::wstring appToRemove = lockedApps[g_selectedListIdx];
                ConfigManager::GetInstance().RemoveLockedApp(appToRemove);
                g_selectedListIdx = -1;
                UpdateTrayIconMetrics(hWnd);
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
        else if (clickedId == ID_CANVAS_PRESET_NOTEPAD) {
            ConfigManager::GetInstance().AddLockedApp(L"notepad.exe");
            UpdateTrayIconMetrics(hWnd);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (clickedId == ID_CANVAS_PRESET_CHROME) {
            ConfigManager::GetInstance().AddLockedApp(L"chrome.exe");
            UpdateTrayIconMetrics(hWnd);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (clickedId == ID_CANVAS_PRESET_CMD) {
            ConfigManager::GetInstance().AddLockedApp(L"cmd.exe");
            UpdateTrayIconMetrics(hWnd);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (clickedId == ID_CANVAS_PRESET_CALC) {
            ConfigManager::GetInstance().AddLockedApp(L"calc.exe");
            UpdateTrayIconMetrics(hWnd);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_NCHITTEST: {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(hWnd, &pt);

        if (pt.x >= 15 && pt.x <= 75 && pt.y >= 15 && pt.y <= 35) {
            return HTCLIENT;
        }
        if (pt.y <= 50) {
            return HTCAPTION;
        }
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }

    case WM_TIMER: {
        if (wParam == g_nTimerID) {
            AppLockEngine::GetInstance().PeriodicCheck();
        }
        return 0;
    }

    case WM_TRAYICON: {
        if (lParam == WM_RBUTTONUP) {
            TrayIcon::GetInstance().ShowContextMenu(hWnd);
        } else if (lParam == WM_LBUTTONDBLCLK) {
            ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);
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

        if (winW <= 0 || winH <= 0) {
            EndPaint(hWnd, &ps);
            return 0;
        }

        // Native Hardware-Accelerated GDI Double-Buffer Memory DC
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP hMemBmp = CreateCompatibleBitmap(hdc, winW, winH);
        HBITMAP hOldBmp = (HBITMAP)SelectObject(memDC, hMemBmp);

        // Pre-fill memory DC with macOS Dark Slate (#141417)
        HBRUSH hBg = CreateSolidBrush(RGB(20, 20, 23));
        FillRect(memDC, &rect, hBg);
        DeleteObject(hBg);

        {
            Graphics graphics(memDC);
            graphics.SetSmoothingMode(SmoothingModeAntiAlias);

            // 1. Draw Traffic Light Dots (Red #FF5F56, Yellow #FFBD2E, Green #27C93F)
            UIComponents::DrawTrafficLights(graphics, 20, 20);

            // Draw Green 'S' Padlock Icon in Header
            UIComponents::DrawAppLogo(graphics, 85, 14, 22);

            // 2. Draw Window Header Title
            FontFamily fontFamily(L"Segoe UI");
            Font titleFont(&fontFamily, 13.5f, FontStyleBold, UnitPoint);
            SolidBrush whiteBrush(Color(255, 248, 250, 252));

            StringFormat formatLeft;
            formatLeft.SetAlignment(StringAlignmentNear);
            formatLeft.SetLineAlignment(StringAlignmentCenter);

            RectF titleRect(115.0f, 12.0f, 320.0f, 28.0f);
            graphics.DrawString(L"Suraksha \x2014 Privacy \x0026 Security", -1, &titleFont, titleRect, &formatLeft, &whiteBrush);

            // Draw About Button next to Status Badge
            bool hovAbout = (g_hoverControlId == ID_CANVAS_BTN_ABOUT);
            bool prsAbout = (g_pressedControlId == ID_CANVAS_BTN_ABOUT);
            UIComponents::DrawCanvasButton(graphics, 520, 14, 75, 28, L"About", ButtonVariant::Secondary, hovAbout, prsAbout);

            // 3. Draw Status Indicator Pill
            const auto& settings = ConfigManager::GetInstance().GetSettings();
            bool protectionActive = settings.protectionEnabled;
            UIComponents::DrawStatusBadge(graphics, 610, 14, 130, 28, protectionActive ? L"Protected" : L"Paused", protectionActive);

            // 4. Draw Left macOS Sidebar Card Container (#1A1A1E)
            Font headerFont(&fontFamily, 11.0f, FontStyleBold, UnitPoint);
            RectF leftHeadRect(30.0f, 70.0f, 330.0f, 24.0f);
            graphics.DrawString(L"Protected Applications", -1, &headerFont, leftHeadRect, &formatLeft, &whiteBrush);

            UIComponents::DrawCanvasCard(graphics, 30, 100, 330, 310, Color(255, 26, 26, 30), Color(255, 255, 255, 15), 12);

            // Draw List Items inside Left Card Container
            const auto& lockedApps = settings.lockedApps;
            if (lockedApps.empty()) {
                UIComponents::DrawEmptyState(graphics, 30, 100, 330, 310, L"No Protected Applications", L"Click '+ Add Application...' to protect your first app");
            } else {
                int itemY = 106;
                for (size_t i = 0; i < lockedApps.size(); ++i) {
                    if (itemY + 36 > 400) break;
                    bool isSel = (g_selectedListIdx == (int)i);
                    bool isHov = (g_hoverListIdx == (int)i);
                    UIComponents::DrawCanvasListItem(graphics, 36, itemY, 318, 36, lockedApps[i], isSel, isHov);
                    itemY += 40;
                }
            }

            // Draw Action Capsule Buttons
            bool hovAdd = (g_hoverControlId == ID_CANVAS_BTN_ADD);
            bool prsAdd = (g_pressedControlId == ID_CANVAS_BTN_ADD);
            UIComponents::DrawCanvasButton(graphics, 30, 420, 160, 38, L"+ Add Application...", ButtonVariant::Primary, hovAdd, prsAdd);

            bool hovRem = (g_hoverControlId == ID_CANVAS_BTN_REMOVE);
            bool prsRem = (g_pressedControlId == ID_CANVAS_BTN_REMOVE);
            UIComponents::DrawCanvasButton(graphics, 200, 420, 160, 38, L"- Remove Selected", ButtonVariant::Danger, hovRem, prsRem);

            // Draw Quick Add Presets (Drawn width-aligned so "+ Calculator" fits on 1 line!)
            bool hovQ1 = (g_hoverControlId == ID_CANVAS_PRESET_NOTEPAD);
            bool prsQ1 = (g_pressedControlId == ID_CANVAS_PRESET_NOTEPAD);
            UIComponents::DrawCanvasButton(graphics, 30, 468, 78, 28, L"+ Notepad", ButtonVariant::Secondary, hovQ1, prsQ1);

            bool hovQ2 = (g_hoverControlId == ID_CANVAS_PRESET_CHROME);
            bool prsQ2 = (g_pressedControlId == ID_CANVAS_PRESET_CHROME);
            UIComponents::DrawCanvasButton(graphics, 114, 468, 76, 28, L"+ Chrome", ButtonVariant::Secondary, hovQ2, prsQ2);

            bool hovQ3 = (g_hoverControlId == ID_CANVAS_PRESET_CMD);
            bool prsQ3 = (g_pressedControlId == ID_CANVAS_PRESET_CMD);
            UIComponents::DrawCanvasButton(graphics, 196, 468, 78, 28, L"+ Terminal", ButtonVariant::Secondary, hovQ3, prsQ3);

            bool hovQ4 = (g_hoverControlId == ID_CANVAS_PRESET_CALC);
            bool prsQ4 = (g_pressedControlId == ID_CANVAS_PRESET_CALC);
            UIComponents::DrawCanvasButton(graphics, 280, 468, 80, 28, L"+ Calculator", ButtonVariant::Secondary, hovQ4, prsQ4);

            // 5. Draw Right macOS Settings Group Card Container (#202024)
            RectF rightHeadRect(395.0f, 70.0f, 355.0f, 24.0f);
            graphics.DrawString(L"Passcode \x0026 Security Options", -1, &headerFont, rightHeadRect, &formatLeft, &whiteBrush);

            UIComponents::DrawCanvasCard(graphics, 395, 100, 355, 396, Color(255, 32, 32, 36), Color(255, 255, 255, 18), 12);

            // Draw 4 macOS Sliding Pill Toggle Switches
            bool hovT1 = (g_hoverControlId == ID_CANVAS_TOGGLE_ENABLE);
            UIComponents::DrawCanvasToggle(graphics, 415, 120, 320, 34, L"Enable App Protection Engine", settings.protectionEnabled, hovT1);

            std::wstring winUserLabel = L"Allow Windows Password (" + SecurityManager::GetInstance().GetCurrentWindowsUsername() + L")";
            bool hovT2 = (g_hoverControlId == ID_CANVAS_TOGGLE_WINAUTH);
            UIComponents::DrawCanvasToggle(graphics, 415, 175, 320, 34, winUserLabel.c_str(), settings.useWindowsAuth, hovT2);

            bool hovT3 = (g_hoverControlId == ID_CANVAS_TOGGLE_CUSTOMPIN);
            UIComponents::DrawCanvasToggle(graphics, 415, 230, 320, 34, L"Allow Custom Master Passcode", settings.useCustomPin, hovT3);

            bool hovSetPin = (g_hoverControlId == ID_CANVAS_BTN_SETPIN);
            bool prsSetPin = (g_pressedControlId == ID_CANVAS_BTN_SETPIN);
            UIComponents::DrawCanvasButton(graphics, 415, 280, 310, 38, L"Set / Change Master Passcode...", ButtonVariant::Secondary, hovSetPin, prsSetPin);

            bool hovT4 = (g_hoverControlId == ID_CANVAS_TOGGLE_AUTOSTART);
            UIComponents::DrawCanvasToggle(graphics, 415, 335, 320, 34, L"Launch automatically at system login", settings.autoStartWithWindows, hovT4);
        }

        // Direct Blit to screen
        BitBlt(hdc, 0, 0, winW, winH, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, hOldBmp);
        DeleteObject(hMemBmp);
        DeleteDC(memDC);

        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_SYSCOMMAND: {
        if ((wParam & 0xFFF0) == SC_MINIMIZE) {
            ShowWindow(hWnd, SW_HIDE);
            return 0;
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    case WM_CLOSE: {
        ShowWindow(hWnd, SW_HIDE);
        return 0;
    }

    case WM_DESTROY: {
        UnregisterHotKey(hWnd, HOTKEY_LOCKALL);
        UnregisterHotKey(hWnd, HOTKEY_TOGGLEWIN);
        KillTimer(hWnd, g_nTimerID);
        AppLockEngine::GetInstance().StopMonitoring();
        TrayIcon::GetInstance().Remove();

        AuditLogger::GetInstance().LogEvent(L"SYSTEM", L"Suraksha Application Shutdown.");

        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProcW(hWnd, message, wParam, lParam);
}
