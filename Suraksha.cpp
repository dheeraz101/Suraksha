// Suraksha.cpp : Entry point for Suraksha Windows App Locker (v2.0 5-Tab Edition)
#include "framework.h"
#include "Suraksha.h"
#include "Version.h"
#include "Resource.h"
#include "ConfigManager.h"
#include "SecurityManager.h"
#include "AppLockEngine.h"
#include "TrayIcon.h"
#include "UIComponents.h"
#include "AuditLogger.h"
#include "UpdateManager.h"
#include "LanguageManager.h"
#include "FontManager.h"

#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <shlobj.h>
#include <objbase.h>
#include <gdiplus.h>
#include <vector>
#include <string>
#include <fstream>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shell32.lib")

using namespace Gdiplus;

#define MAX_LOADSTRING 100

// Navigation Tab Constants (5 Tabs)
#define TAB_APPLOCKER  0
#define TAB_SECURITY   1
#define TAB_UPDATES    2
#define TAB_LOGS       3
#define TAB_ABOUT      4

// Control IDs for Direct Canvas Hit Testing
#define ID_CANVAS_TAB_APPLOCKER    3001
#define ID_CANVAS_TAB_SECURITY     3002
#define ID_CANVAS_TAB_UPDATES      3003
#define ID_CANVAS_TAB_LOGS         3004
#define ID_CANVAS_TAB_ABOUT        3005

#define ID_CANVAS_BTN_ADD          3010
#define ID_CANVAS_BTN_REMOVE       3011
#define ID_CANVAS_PRESET_NOTEPAD   3012
#define ID_CANVAS_PRESET_CHROME    3013
#define ID_CANVAS_PRESET_CMD       3014
#define ID_CANVAS_PRESET_CALC      3015

#define ID_CANVAS_TOGGLE_ENABLE    3020
#define ID_CANVAS_TOGGLE_WINAUTH   3021
#define ID_CANVAS_TOGGLE_CUSTOMPIN 3022
#define ID_CANVAS_BTN_SETPIN       3023
#define ID_CANVAS_TOGGLE_AUTOSTART 3024

// Scheduled Protection Controls
#define ID_CANVAS_TOGGLE_SCHEDULE  3060
#define ID_CANVAS_BTN_SCHED_S_DEC  3061
#define ID_CANVAS_BTN_SCHED_S_INC  3062
#define ID_CANVAS_BTN_SCHED_E_DEC  3063
#define ID_CANVAS_BTN_SCHED_E_INC  3064

// Enterprise Policy Controls
#define ID_CANVAS_BTN_EXPORT_POL   3070
#define ID_CANVAS_BTN_IMPORT_POL   3071

// Software Update Controls
#define ID_CANVAS_BTN_CHECKUPDATE  3030
#define ID_CANVAS_BTN_DOWNLOAD     3031
#define ID_CANVAS_BTN_INSTALL      3032
#define ID_CANVAS_RADIO_STABLE     3033
#define ID_CANVAS_RADIO_BETA       3034

#define ID_CANVAS_BTN_OPENLOG      3040

// Language Selector Controls (Tab 4)
#define ID_CANVAS_LANG_EN          3080
#define ID_CANVAS_LANG_HI          3081
#define ID_CANVAS_LANG_ES          3082
#define ID_CANVAS_LANG_DE          3083
#define ID_CANVAS_LANG_FR          3084
#define ID_CANVAS_LANG_JA          3085

#define ID_CANVAS_BTN_YABP         3050
#define ID_CANVAS_BTN_DEV          3051
#define ID_CANVAS_BTN_GITHUB       3052

// Global System Hotkey IDs
#define HOTKEY_LOCKALL   101 // Ctrl + Alt + L
#define HOTKEY_TOGGLEWIN 102 // Ctrl + Alt + S

// Global Variables:
HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];
HWND g_hWndMain = NULL;
UINT_PTR g_nTimerID = 1001;
UINT_PTR g_nSpinnerTimerID = 1002;
ULONG_PTR g_gdiplusToken = 0;
static float g_spinAngle = 0.0f;

// Interactive Canvas State
int g_activeTab = TAB_APPLOCKER;
int g_hoverControlId = 0;
int g_pressedControlId = 0;
int g_selectedListIdx = -1;
int g_hoverListIdx = -1;
bool g_closeHover = false;

// Forward declarations
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void ApplyDarkThemeToWindow(HWND hWnd);
void UpdateTrayIconMetrics(HWND hWnd);
void PromptSetCustomPin(HWND hWndParent);

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
    wcex.hbrBackground  = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

void ApplyDarkThemeToWindow(HWND hWnd) {
    BOOL useDarkMode = TRUE;
    DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));

    COLORREF darkBorder = RGB(36, 36, 40);
    DwmSetWindowAttribute(hWnd, DWMWA_BORDER_COLOR, &darkBorder, sizeof(darkBorder));

    MARGINS margins = { 1, 1, 1, 1 };
    DwmExtendFrameIntoClientArea(hWnd, &margins);
}

void UpdateTrayIconMetrics(HWND hWnd) {
    const auto& settings = ConfigManager::GetInstance().GetSettings();
    int count = (int)settings.lockedApps.size();
    std::wstring statusStr = settings.protectionEnabled ? L"Active" : L"Paused";
    std::wstring tip = L"Suraksha: " + std::to_wstring(count) + L" Apps Protected | Status: " + statusStr;
    TrayIcon::GetInstance().UpdateTooltip(hWnd, tip);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
    hInst = hInstance;

    int winWidth = 840;
    int winHeight = 560;

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenW - winWidth) / 2;
    int posY = (screenH - winHeight) / 2;

    HWND hWnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        szWindowClass,
        L"Suraksha - Privacy & Security",
        WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX,
        posX, posY, winWidth, winHeight,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hWnd) return FALSE;

    g_hWndMain = hWnd;
    ApplyDarkThemeToWindow(hWnd);
    UIComponents::ApplyRoundedRegion(hWnd, 22);

    // Register Global System Hotkeys
    RegisterHotKey(hWnd, HOTKEY_LOCKALL, MOD_CONTROL | MOD_ALT, 'L');  // Ctrl + Alt + L
    RegisterHotKey(hWnd, HOTKEY_TOGGLEWIN, MOD_CONTROL | MOD_ALT, 'S'); // Ctrl + Alt + S

    // Create System Tray Icon
    HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SURAKSHA));
    if (!hIcon) hIcon = LoadIcon(NULL, IDI_SHIELD);
    TrayIcon::GetInstance().Create(hWnd, 1, hIcon, L"Suraksha - Privacy & Security");
    UpdateTrayIconMetrics(hWnd);

    // Initialize UI Localization
    int currentLang = ConfigManager::GetInstance().GetSettings().language;
    if (currentLang >= 0 && currentLang <= 5) {
        LanguageManager::GetInstance().SetLanguage((Language)currentLang);
    }

    // Start App Monitoring Engine
    AppLockEngine::GetInstance().StartMonitoring(hWnd);

    // Initialize Software Update Manager and check for updates silently
    UpdateManager::GetInstance().Initialize(hWnd);
    UpdateManager::GetInstance().CheckForUpdatesAsync(false);

    // 1000ms periodic timer for UI sync
    SetTimer(hWnd, g_nTimerID, 1000, NULL);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

// ---------------- 100% Direct Canvas Master Passcode Modal ----------------
static std::wstring s_pin1 = L"";
static std::wstring s_pin2 = L"";
static int s_activePinField = 0; // 0 for Field 1, 1 for Field 2
static std::wstring s_pinError = L"";
static bool s_pinCloseHover = false;
static int s_pinBtnHover = 0;
static int s_pinBtnPressed = 0;

static LRESULT CALLBACK SetPinDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    const RECT rcField1 = { 30, 70, 310, 110 };
    const RECT rcField2 = { 30, 120, 310, 160 };
    const RECT rcBtnSave = { 30, 175, 310, 218 };

    switch (uMsg) {
    case WM_CREATE: {
        s_pin1 = L"";
        s_pin2 = L"";
        s_activePinField = 0;
        s_pinError = L"";
        s_pinCloseHover = false;
        s_pinBtnHover = 0;
        s_pinBtnPressed = 0;
        SetFocus(hWnd);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;

    case WM_SETCURSOR:
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return TRUE;

    case WM_CHAR: {
        wchar_t ch = (wchar_t)wParam;
        std::wstring& targetStr = (s_activePinField == 0) ? s_pin1 : s_pin2;

        if (ch == VK_BACK) {
            if (!targetStr.empty()) {
                targetStr.pop_back();
                s_pinError = L"";
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
        else if (ch == VK_TAB) {
            s_activePinField = (s_activePinField == 0) ? 1 : 0;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (ch == VK_RETURN) {
            if (s_pin1.empty()) {
                s_pinError = L"Passcode cannot be empty.";
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }
            if (s_pin1 != s_pin2) {
                s_pinError = L"Passcodes do not match.";
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }
            SecurityManager::GetInstance().SetCustomPin(s_pin1);
            MessageBoxW(hWnd, L"Master Passcode successfully updated!", L"Suraksha", MB_OK | MB_ICONINFORMATION);
            DestroyWindow(hWnd);
        }
        else if (ch == VK_ESCAPE) {
            DestroyWindow(hWnd);
        }
        else if (ch >= 32 && ch < 127) {
            if (targetStr.length() < 64) {
                targetStr.push_back(ch);
                s_pinError = L"";
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);

        int oldBtn = s_pinBtnHover;
        bool oldClose = s_pinCloseHover;

        s_pinCloseHover = (x >= 18 && x <= 34 && y >= 16 && y <= 32);
        s_pinBtnHover = (x >= rcBtnSave.left && x <= rcBtnSave.right && y >= rcBtnSave.top && y <= rcBtnSave.bottom) ? 1 : 0;

        if (oldBtn != s_pinBtnHover || oldClose != s_pinCloseHover) {
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);

        if (x >= 18 && x <= 34 && y >= 16 && y <= 32) {
            DestroyWindow(hWnd);
            return 0;
        }

        if (x >= rcField1.left && x <= rcField1.right && y >= rcField1.top && y <= rcField1.bottom) {
            s_activePinField = 0;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (x >= rcField2.left && x <= rcField2.right && y >= rcField2.top && y <= rcField2.bottom) {
            s_activePinField = 1;
            InvalidateRect(hWnd, NULL, FALSE);
        }

        s_pinBtnPressed = s_pinBtnHover;
        SetFocus(hWnd);
        return 0;
    }

    case WM_LBUTTONUP: {
        if (s_pinBtnHover == 1) {
            if (s_pin1.empty()) {
                s_pinError = L"Passcode cannot be empty.";
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }
            if (s_pin1 != s_pin2) {
                s_pinError = L"Passcodes do not match.";
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }
            SecurityManager::GetInstance().SetCustomPin(s_pin1);
            MessageBoxW(hWnd, L"Master Passcode successfully updated!", L"Suraksha", MB_OK | MB_ICONINFORMATION);
            DestroyWindow(hWnd);
        }
        s_pinBtnPressed = 0;
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

        HBRUSH hBg = CreateSolidBrush(RGB(20, 20, 23));
        FillRect(memDC, &rect, hBg);
        DeleteObject(hBg);

        {
            Graphics graphics(memDC);
            graphics.SetSmoothingMode(SmoothingModeAntiAlias);
            graphics.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

            Pen borderPen(Color(20, 255, 255, 255), 1.0f);
            graphics.DrawRectangle(&borderPen, 0, 0, winW - 1, winH - 1);

            UIComponents::DrawCloseButton(graphics, 20, 18, 13, s_pinCloseHover);

            FontFamily fontFamDisplay(L"Segoe UI Variable Display");
            FontFamily fontFamText(L"Segoe UI Variable Text");
            FontFamily fontFamFallback(L"Segoe UI");

            const FontFamily* pDisplayFam = fontFamDisplay.IsAvailable() ? &fontFamDisplay : &fontFamFallback;
            const FontFamily* pTextFam = fontFamText.IsAvailable() ? &fontFamText : &fontFamFallback;

            Font fontTitle(pDisplayFam, 12.0f, FontStyleBold, UnitPoint);
            Font fontSub(pTextFam, 9.5f, FontStyleRegular, UnitPoint);
            Font fontBullet(pDisplayFam, 11.0f, FontStyleBold, UnitPoint);
            Font fontErr(pTextFam, 9.0f, FontStyleBold, UnitPoint);

            SolidBrush whiteBrush(Color(255, 248, 250, 252));
            SolidBrush mutedBrush(Color(255, 148, 163, 184));
            SolidBrush errBrush(Color(255, 255, 69, 58));

            StringFormat formatLeft;
            formatLeft.SetAlignment(StringAlignmentNear);
            formatLeft.SetLineAlignment(StringAlignmentCenter);

            RectF titleRect(45.0f, 12.0f, 270.0f, 26.0f);
            graphics.DrawString(L"Set Master Passcode", -1, &fontTitle, titleRect, &formatLeft, &whiteBrush);

            // Field 1: New Passcode
            Color b1 = (s_activePinField == 0) ? Color(255, 10, 132, 255) : Color(20, 255, 255, 255);
            UIComponents::DrawCanvasCard(graphics, rcField1.left, rcField1.top, rcField1.right - rcField1.left, rcField1.bottom - rcField1.top, Color(255, 34, 34, 38), b1, 8);
            UIComponents::DrawIconKey(graphics, rcField1.left + 10, rcField1.top + 12, 16, Color(255, 148, 163, 184));

            if (s_pin1.empty()) {
                RectF rc1((REAL)(rcField1.left + 34), (REAL)rcField1.top, (REAL)(rcField1.right - rcField1.left - 40), (REAL)(rcField1.bottom - rcField1.top));
                graphics.DrawString(L"Enter New Passcode...", -1, &fontSub, rc1, &formatLeft, &mutedBrush);
            } else {
                std::wstring mask1 = L"";
                for (size_t i = 0; i < s_pin1.length(); ++i) mask1 += L"\x25CF  ";
                RectF rc1((REAL)(rcField1.left + 34), (REAL)rcField1.top, (REAL)(rcField1.right - rcField1.left - 40), (REAL)(rcField1.bottom - rcField1.top));
                graphics.DrawString(mask1.c_str(), -1, &fontBullet, rc1, &formatLeft, &whiteBrush);
            }

            // Field 2: Confirm Passcode
            Color b2 = (s_activePinField == 1) ? Color(255, 10, 132, 255) : Color(20, 255, 255, 255);
            UIComponents::DrawCanvasCard(graphics, rcField2.left, rcField2.top, rcField2.right - rcField2.left, rcField2.bottom - rcField2.top, Color(255, 34, 34, 38), b2, 8);
            UIComponents::DrawIconKey(graphics, rcField2.left + 10, rcField2.top + 12, 16, Color(255, 148, 163, 184));

            if (s_pin2.empty()) {
                RectF rc2((REAL)(rcField2.left + 34), (REAL)rcField2.top, (REAL)(rcField2.right - rcField2.left - 40), (REAL)(rcField2.bottom - rcField2.top));
                graphics.DrawString(L"Confirm New Passcode...", -1, &fontSub, rc2, &formatLeft, &mutedBrush);
            } else {
                std::wstring mask2 = L"";
                for (size_t i = 0; i < s_pin2.length(); ++i) mask2 += L"\x25CF  ";
                RectF rc2((REAL)(rcField2.left + 34), (REAL)rcField2.top, (REAL)(rcField2.right - rcField2.left - 40), (REAL)(rcField2.bottom - rcField2.top));
                graphics.DrawString(mask2.c_str(), -1, &fontBullet, rc2, &formatLeft, &whiteBrush);
            }

            // Save Button
            UIComponents::DrawCanvasButton(graphics, rcBtnSave.left, rcBtnSave.top, rcBtnSave.right - rcBtnSave.left, rcBtnSave.bottom - rcBtnSave.top,
                L"Save Master Passcode", ButtonVariant::Primary, (s_pinBtnHover == 1), (s_pinBtnPressed == 1), VectorIcon::Key);

            // Error Text
            if (!s_pinError.empty()) {
                StringFormat formatCenter;
                formatCenter.SetAlignment(StringAlignmentCenter);
                formatCenter.SetLineAlignment(StringAlignmentCenter);
                RectF rcErr(20.0f, (REAL)(winH - 24), (REAL)(winW - 40), 18.0f);
                graphics.DrawString(s_pinError.c_str(), -1, &fontErr, rcErr, &formatCenter, &errBrush);
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
        DestroyWindow(hWnd);
        return 0;
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

void PromptSetCustomPin(HWND hWndParent) {
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = SetPinDialogProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"SurakshaDirectCanvasSetPinClass";
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassExW(&wc);

    HWND hwndDlg = CreateWindowExW(
        WS_EX_TOPMOST,
        wc.lpszClassName, L"Set Master Passcode",
        WS_POPUP,
        (GetSystemMetrics(SM_CXSCREEN) - 340) / 2,
        (GetSystemMetrics(SM_CYSCREEN) - 250) / 2,
        340, 250, hWndParent, NULL, GetModuleHandle(NULL), NULL
    );

    if (hwndDlg) {
        UIComponents::ApplyRoundedRegion(hwndDlg, 20);
        ShowWindow(hwndDlg, SW_SHOW);
        UpdateWindow(hwndDlg);
        SetForegroundWindow(hwndDlg);
        MSG msg;
        while (IsWindow(hwndDlg) && GetMessageW(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

// Helper: Format hour integer into 12-hour AM/PM string
static std::wstring FormatHour(int hour) {
    int h = hour % 24;
    bool pm = (h >= 12);
    int dispH = h % 12;
    if (dispH == 0) dispH = 12;
    wchar_t buf[32];
    swprintf_s(buf, 32, L"%02d:00 %s", dispH, pm ? L"PM" : L"AM");
    return buf;
}

// Enterprise Policy Passkey Dialog
static std::wstring s_policyPasskey = L"";
static bool s_policyPassAccepted = false;
static std::wstring s_policyPromptTitle = L"Policy Encryption Key";

static LRESULT CALLBACK PolicyPassDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static std::wstring enteredPass = L"";
    switch (uMsg) {
    case WM_CREATE:
        enteredPass = L"";
        s_policyPassAccepted = false;
        SetFocus(hWnd);
        return 0;
    case WM_CHAR: {
        wchar_t ch = (wchar_t)wParam;
        if (ch == VK_BACK) {
            if (!enteredPass.empty()) { enteredPass.pop_back(); InvalidateRect(hWnd, NULL, FALSE); }
        } else if (ch == VK_RETURN) {
            if (!enteredPass.empty()) {
                s_policyPasskey = enteredPass;
                s_policyPassAccepted = true;
                DestroyWindow(hWnd);
            }
        } else if (ch == VK_ESCAPE) {
            s_policyPassAccepted = false;
            DestroyWindow(hWnd);
        } else if (ch >= 32 && ch < 127) {
            if (enteredPass.length() < 64) {
                enteredPass.push_back(ch);
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rect; GetClientRect(hWnd, &rect);
        int w = rect.right - rect.left, h = rect.bottom - rect.top;
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP hMemBmp = CreateCompatibleBitmap(hdc, w, h);
        HBITMAP hOldBmp = (HBITMAP)SelectObject(memDC, hMemBmp);

        HBRUSH hBg = CreateSolidBrush(RGB(20, 20, 23));
        FillRect(memDC, &rect, hBg);
        DeleteObject(hBg);

        {
            Graphics graphics(memDC);
            graphics.SetSmoothingMode(SmoothingModeAntiAlias);
            graphics.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

            Pen borderPen(Color(30, 255, 255, 255), 1.0f);
            graphics.DrawRectangle(&borderPen, 0, 0, w - 1, h - 1);

            FontFamily fontFamDisplay(L"Segoe UI Variable Display");
            FontFamily fontFamText(L"Segoe UI Variable Text");
            FontFamily fontFamFallback(L"Segoe UI");
            const FontFamily* pDisplayFam = fontFamDisplay.IsAvailable() ? &fontFamDisplay : &fontFamFallback;
            const FontFamily* pTextFam = fontFamText.IsAvailable() ? &fontFamText : &fontFamFallback;

            Font fontTitle(pDisplayFam, 12.0f, FontStyleBold, UnitPoint);
            Font fontSub(pTextFam, 9.0f, FontStyleRegular, UnitPoint);
            SolidBrush whiteBrush(Color(255, 248, 250, 252));
            SolidBrush mutedBrush(Color(255, 148, 163, 184));

            StringFormat formatCenter;
            formatCenter.SetAlignment(StringAlignmentCenter);
            formatCenter.SetLineAlignment(StringAlignmentCenter);

            RectF rcTitle(20.0f, 20.0f, (REAL)(w - 40), 22.0f);
            graphics.DrawString(s_policyPromptTitle.c_str(), -1, &fontTitle, rcTitle, &formatCenter, &whiteBrush);

            RectF rcSub(20.0f, 44.0f, (REAL)(w - 40), 20.0f);
            graphics.DrawString(L"Enter AES-256 Encryption Passphrase:", -1, &fontSub, rcSub, &formatCenter, &mutedBrush);

            // Input card
            UIComponents::DrawCanvasCard(graphics, 30, 75, w - 60, 42, Color(255, 36, 36, 40), Color(255, 10, 132, 255), 10);
            UIComponents::DrawIconKey(graphics, 42, 88, 16, Color(255, 148, 163, 184));

            std::wstring masked = L"";
            for (size_t i = 0; i < enteredPass.length(); ++i) masked += L"*  ";

            if (masked.empty()) {
                RectF rcCue(66.0f, 75.0f, (REAL)(w - 100), 42.0f);
                StringFormat formatLeft;
                formatLeft.SetAlignment(StringAlignmentNear);
                formatLeft.SetLineAlignment(StringAlignmentCenter);
                graphics.DrawString(L"Type passphrase & press Enter...", -1, &fontSub, rcCue, &formatLeft, &mutedBrush);
            } else {
                Font fontBullet(pDisplayFam, 12.0f, FontStyleBold, UnitPoint);
                RectF rcMasked(66.0f, 75.0f, (REAL)(w - 100), 42.0f);
                StringFormat formatLeft;
                formatLeft.SetAlignment(StringAlignmentNear);
                formatLeft.SetLineAlignment(StringAlignmentCenter);
                graphics.DrawString(masked.c_str(), -1, &fontBullet, rcMasked, &formatLeft, &whiteBrush);
            }

            UIComponents::DrawCanvasButton(graphics, 30, 130, w - 60, 38, L"Confirm Passphrase (Enter)", ButtonVariant::Primary, false, false, VectorIcon::Check);
            UIComponents::DrawCanvasButton(graphics, 30, 176, w - 60, 34, L"Cancel (Esc)", ButtonVariant::Secondary, false, false);
        }

        BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);
        SelectObject(memDC, hOldBmp);
        DeleteObject(hMemBmp);
        DeleteDC(memDC);
        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
        RECT rect; GetClientRect(hWnd, &rect);
        int w = rect.right - rect.left;
        if (x >= 30 && x <= w - 30 && y >= 130 && y <= 168) {
            if (!enteredPass.empty()) {
                s_policyPasskey = enteredPass;
                s_policyPassAccepted = true;
                DestroyWindow(hWnd);
            }
        } else if (x >= 30 && x <= w - 30 && y >= 176 && y <= 210) {
            s_policyPassAccepted = false;
            DestroyWindow(hWnd);
        }
        return 0;
    }
    case WM_CLOSE:
        s_policyPassAccepted = false;
        DestroyWindow(hWnd);
        return 0;
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

static bool PromptPolicyPasskey(HWND hWndParent, bool isExport, std::wstring& outPasskey) {
    s_policyPromptTitle = isExport ? L"Set Policy Export Passkey" : L"Enter Policy Import Passkey";
    s_policyPasskey = L"";
    s_policyPassAccepted = false;

    static bool s_classReg = false;
    if (!s_classReg) {
        WNDCLASSEXW wc = { sizeof(wc) };
        wc.lpfnWndProc = PolicyPassDialogProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = L"SurakshaPolicyPassDialogClass";
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        RegisterClassExW(&wc);
        s_classReg = true;
    }

    HWND hwndDlg = CreateWindowExW(
        WS_EX_TOPMOST,
        L"SurakshaPolicyPassDialogClass", s_policyPromptTitle.c_str(),
        WS_POPUP,
        (GetSystemMetrics(SM_CXSCREEN) - 360) / 2,
        (GetSystemMetrics(SM_CYSCREEN) - 235) / 2,
        360, 235, hWndParent, NULL, GetModuleHandle(NULL), NULL
    );

    if (hwndDlg) {
        UIComponents::ApplyRoundedRegion(hwndDlg, 20);
        ShowWindow(hwndDlg, SW_SHOW);
        UpdateWindow(hwndDlg);
        SetForegroundWindow(hwndDlg);
        MSG msg;
        while (IsWindow(hwndDlg) && GetMessageW(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    if (s_policyPassAccepted && !s_policyPasskey.empty()) {
        outPasskey = s_policyPasskey;
        return true;
    }
    return false;
}

// Helper: Check if point is inside RECT
static bool PtInRectStruct(const RECT& r, int x, int y) {
    return (x >= r.left && x <= r.right && y >= r.top && y <= r.bottom);
}

// Helper: Read last N lines from Audit Log
static std::vector<std::wstring> GetRecentAuditLogs(int maxLines = 8) {
    std::vector<std::wstring> lines;
    wchar_t appData[MAX_PATH] = { 0 };
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appData))) {
        std::wstring logPath = std::wstring(appData) + L"\\Suraksha\\logs\\audit.log";
        std::wifstream file(logPath);
        if (file.is_open()) {
            std::wstring line;
            while (std::getline(file, line)) {
                if (!line.empty()) lines.push_back(line);
            }
            file.close();
        }
    }
    if ((int)lines.size() > maxLines) {
        lines.erase(lines.begin(), lines.begin() + (lines.size() - maxLines));
    }
    return lines;
}

static bool EnsureSecurityConfigured(HWND hWnd) {
    auto& settings = ConfigManager::GetInstance().GetSettings();
    if (!settings.useWindowsAuth && !SecurityManager::GetInstance().HasCustomPin()) {
        int choice = MessageBoxW(hWnd,
            L"Before protecting applications, you must configure an authentication method:\n\n"
            L" \x2022 Click YES to use Windows Hello Face, Fingerprint, or PIN (Recommended)\n"
            L" \x2022 Click NO to create a Custom Master Passcode inside Suraksha\n"
            L" \x2022 Click CANCEL to abort",
            L"Suraksha - Security Setup Required",
            MB_YESNOCANCEL | MB_ICONQUESTION);

        if (choice == IDYES) {
            std::wstring authErr = L"";
            bool testOk = SecurityManager::GetInstance().VerifyWindowsCredentials(hWnd, authErr);
            if (testOk) {
                settings.useWindowsAuth = true;
                ConfigManager::GetInstance().SaveSettings();
                AuditLogger::GetInstance().LogEvent(L"SECURITY_SETUP", L"Windows Hello / PIN configured as primary unlock method.");
                return true;
            } else {
                int noPinChoice = MessageBoxW(hWnd,
                    L"Windows authorization could not be completed or no Windows PIN/Password is set up on this device.\n\n"
                    L" \x2022 Click RETRY to open Windows Sign-in Settings and set up a PIN/Password now.\n"
                    L" \x2022 Click CANCEL to set a Custom Master Passcode inside Suraksha instead.",
                    L"Suraksha - Setup Windows Sign-in",
                    MB_RETRYCANCEL | MB_ICONEXCLAMATION);

                if (noPinChoice == IDRETRY) {
                    ShellExecuteW(NULL, L"open", L"ms-settings:signinoptions", NULL, NULL, SW_SHOWNORMAL);
                    return false;
                } else {
                    PromptSetCustomPin(hWnd);
                    return SecurityManager::GetInstance().HasCustomPin();
                }
            }
        } else if (choice == IDNO) {
            PromptSetCustomPin(hWnd);
            return SecurityManager::GetInstance().HasCustomPin();
        }
        return false;
    }
    return true;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    // 5 Sidebar Tabs Coordinates (x: 12 to 198)
    const RECT rcTab0 = { 12, 90, 198, 126 };  // App Locker
    const RECT rcTab1 = { 12, 132, 198, 168 }; // Security & Auth
    const RECT rcTab2 = { 12, 174, 198, 210 }; // Software Update
    const RECT rcTab3 = { 12, 216, 198, 252 }; // Audit Logs
    const RECT rcTab4 = { 12, 258, 198, 294 }; // About Suraksha

    // Page 0: App Locker Controls
    const RECT rcBtnAdd          = { 235, 436, 430, 478 };
    const RECT rcBtnRemove       = { 445, 436, 640, 478 };
    const RECT rcPreset1         = { 235, 490, 345, 526 }; // Notepad
    const RECT rcPreset2         = { 355, 490, 495, 526 }; // Google Chrome
    const RECT rcPreset3         = { 505, 490, 615, 526 }; // Terminal
    const RECT rcPreset4         = { 625, 490, 755, 526 }; // Calculator

    // Page 1: Security Controls (3 Cards)
    const RECT rcToggleEnable    = { 255, 68, 785, 102 };
    const RECT rcToggleWinAuth   = { 255, 106, 515, 140 };
    const RECT rcToggleCustomPin = { 525, 106, 785, 140 };
    const RECT rcBtnSetPin       = { 255, 148, 515, 186 };
    const RECT rcToggleAutoStart = { 525, 148, 785, 186 };

    // Card 2: Scheduled Protection (Active Hours)
    const RECT rcToggleSchedule  = { 255, 238, 785, 272 };
    const RECT rcSchedStartDec   = { 365, 280, 395, 312 };
    const RECT rcSchedStartInc   = { 480, 280, 510, 312 };
    const RECT rcSchedEndDec     = { 640, 280, 670, 312 };
    const RECT rcSchedEndInc     = { 755, 280, 785, 312 };

    // Card 3: Enterprise Policy Management
    const RECT rcBtnExportPolicy = { 255, 415, 515, 455 };
    const RECT rcBtnImportPolicy = { 525, 415, 785, 455 };

    // Page 2: Software Update Controls (macOS Style)
    const RECT rcBtnCheckUpdate  = { 255, 220, 435, 262 };
    const RECT rcBtnDownload     = { 255, 220, 445, 262 };
    const RECT rcBtnInstall      = { 255, 220, 465, 262 };
    const RECT rcRadioStable     = { 255, 335, 515, 410 };
    const RECT rcRadioBeta       = { 525, 335, 785, 410 };

    // Page 3: Audit Logs Controls
    const RECT rcBtnOpenLog      = { 255, 480, 520, 522 };

    // Page 4: About Controls & Language Selector
    const RECT rcLangEN          = { 265, 180, 425, 218 };
    const RECT rcLangHI          = { 435, 180, 595, 218 };
    const RECT rcLangES          = { 605, 180, 775, 218 };
    const RECT rcLangDE          = { 265, 224, 425, 262 };
    const RECT rcLangFR          = { 435, 224, 595, 262 };
    const RECT rcLangJA          = { 605, 224, 775, 262 };

    const RECT rcBtnYABP         = { 250, 428, 415, 470 };
    const RECT rcBtnDev          = { 430, 428, 595, 470 };
    const RECT rcBtnGithub       = { 610, 428, 775, 470 };

    switch (message) {
    case WM_CREATE:
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_SETCURSOR:
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return TRUE;

    case WM_HOTKEY: {
        if (wParam == HOTKEY_LOCKALL) { // Ctrl + Alt + L
            AppLockEngine::GetInstance().LockAllProcesses();
            AuditLogger::GetInstance().LogEvent(L"HOTKEY_TRIGGERED", L"Global Hotkey Ctrl+Alt+L triggered: All protected sessions locked.");
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

        TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hWnd, 0 };
        TrackMouseEvent(&tme);

        int oldHover = g_hoverControlId;
        int oldHoverList = g_hoverListIdx;
        bool oldClose = g_closeHover;

        g_hoverControlId = 0;
        g_hoverListIdx = -1;
        g_closeHover = (x >= 18 && x <= 34 && y >= 16 && y <= 32);

        // Sidebar Tabs Hover
        if (PtInRectStruct(rcTab0, x, y)) g_hoverControlId = ID_CANVAS_TAB_APPLOCKER;
        else if (PtInRectStruct(rcTab1, x, y)) g_hoverControlId = ID_CANVAS_TAB_SECURITY;
        else if (PtInRectStruct(rcTab2, x, y)) g_hoverControlId = ID_CANVAS_TAB_UPDATES;
        else if (PtInRectStruct(rcTab3, x, y)) g_hoverControlId = ID_CANVAS_TAB_LOGS;
        else if (PtInRectStruct(rcTab4, x, y)) g_hoverControlId = ID_CANVAS_TAB_ABOUT;

        // Page 0 Hover
        else if (g_activeTab == TAB_APPLOCKER) {
            if (PtInRectStruct(rcBtnAdd, x, y)) g_hoverControlId = ID_CANVAS_BTN_ADD;
            else if (PtInRectStruct(rcBtnRemove, x, y)) g_hoverControlId = ID_CANVAS_BTN_REMOVE;
            else if (PtInRectStruct(rcPreset1, x, y)) g_hoverControlId = ID_CANVAS_PRESET_NOTEPAD;
            else if (PtInRectStruct(rcPreset2, x, y)) g_hoverControlId = ID_CANVAS_PRESET_CHROME;
            else if (PtInRectStruct(rcPreset3, x, y)) g_hoverControlId = ID_CANVAS_PRESET_CMD;
            else if (PtInRectStruct(rcPreset4, x, y)) g_hoverControlId = ID_CANVAS_PRESET_CALC;

            // List Item Hover (matching render origin Y = 70)
            if (x >= 235 && x <= 810 && y >= 60 && y <= 420) {
                const auto& lockedApps = ConfigManager::GetInstance().GetSettings().lockedApps;
                int itemY = 70;
                for (size_t i = 0; i < lockedApps.size(); ++i) {
                    if (y >= itemY && y <= itemY + 36) {
                        g_hoverListIdx = (int)i;
                        break;
                    }
                    itemY += 40;
                }
            }
        }
        // Page 1 Hover
        else if (g_activeTab == TAB_SECURITY) {
            if (PtInRectStruct(rcToggleEnable, x, y)) g_hoverControlId = ID_CANVAS_TOGGLE_ENABLE;
            else if (PtInRectStruct(rcToggleWinAuth, x, y)) g_hoverControlId = ID_CANVAS_TOGGLE_WINAUTH;
            else if (PtInRectStruct(rcToggleCustomPin, x, y)) g_hoverControlId = ID_CANVAS_TOGGLE_CUSTOMPIN;
            else if (PtInRectStruct(rcBtnSetPin, x, y)) g_hoverControlId = ID_CANVAS_BTN_SETPIN;
            else if (PtInRectStruct(rcToggleAutoStart, x, y)) g_hoverControlId = ID_CANVAS_TOGGLE_AUTOSTART;
            else if (PtInRectStruct(rcToggleSchedule, x, y)) g_hoverControlId = ID_CANVAS_TOGGLE_SCHEDULE;
            else if (PtInRectStruct(rcSchedStartDec, x, y)) g_hoverControlId = ID_CANVAS_BTN_SCHED_S_DEC;
            else if (PtInRectStruct(rcSchedStartInc, x, y)) g_hoverControlId = ID_CANVAS_BTN_SCHED_S_INC;
            else if (PtInRectStruct(rcSchedEndDec, x, y)) g_hoverControlId = ID_CANVAS_BTN_SCHED_E_DEC;
            else if (PtInRectStruct(rcSchedEndInc, x, y)) g_hoverControlId = ID_CANVAS_BTN_SCHED_E_INC;
            else if (PtInRectStruct(rcBtnExportPolicy, x, y)) g_hoverControlId = ID_CANVAS_BTN_EXPORT_POL;
            else if (PtInRectStruct(rcBtnImportPolicy, x, y)) g_hoverControlId = ID_CANVAS_BTN_IMPORT_POL;
        }
        // Page 2 Hover (Software Update)
        else if (g_activeTab == TAB_UPDATES) {
            UpdateStatus st = UpdateManager::GetInstance().GetStatus();
            if (st == UpdateStatus::UpdateAvailable && PtInRectStruct(rcBtnDownload, x, y)) g_hoverControlId = ID_CANVAS_BTN_DOWNLOAD;
            else if (st == UpdateStatus::ReadyToInstall && PtInRectStruct(rcBtnInstall, x, y)) g_hoverControlId = ID_CANVAS_BTN_INSTALL;
            else if (PtInRectStruct(rcBtnCheckUpdate, x, y)) g_hoverControlId = ID_CANVAS_BTN_CHECKUPDATE;
            else if (PtInRectStruct(rcRadioStable, x, y)) g_hoverControlId = ID_CANVAS_RADIO_STABLE;
            else if (PtInRectStruct(rcRadioBeta, x, y)) g_hoverControlId = ID_CANVAS_RADIO_BETA;
        }
        // Page 3 Hover (Audit Logs)
        else if (g_activeTab == TAB_LOGS) {
            if (PtInRectStruct(rcBtnOpenLog, x, y)) g_hoverControlId = ID_CANVAS_BTN_OPENLOG;
        }
        // Page 4 Hover (About Links & Language Selector)
        else if (g_activeTab == TAB_ABOUT) {
            if (PtInRectStruct(rcLangEN, x, y)) g_hoverControlId = ID_CANVAS_LANG_EN;
            else if (PtInRectStruct(rcLangHI, x, y)) g_hoverControlId = ID_CANVAS_LANG_HI;
            else if (PtInRectStruct(rcLangES, x, y)) g_hoverControlId = ID_CANVAS_LANG_ES;
            else if (PtInRectStruct(rcLangDE, x, y)) g_hoverControlId = ID_CANVAS_LANG_DE;
            else if (PtInRectStruct(rcLangFR, x, y)) g_hoverControlId = ID_CANVAS_LANG_FR;
            else if (PtInRectStruct(rcLangJA, x, y)) g_hoverControlId = ID_CANVAS_LANG_JA;
            else if (PtInRectStruct(rcBtnYABP, x, y)) g_hoverControlId = ID_CANVAS_BTN_YABP;
            else if (PtInRectStruct(rcBtnDev, x, y)) g_hoverControlId = ID_CANVAS_BTN_DEV;
            else if (PtInRectStruct(rcBtnGithub, x, y)) g_hoverControlId = ID_CANVAS_BTN_GITHUB;
        }

        if (oldHover != g_hoverControlId || oldHoverList != g_hoverListIdx || oldClose != g_closeHover) {
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_MOUSELEAVE: {
        g_hoverControlId = 0;
        g_hoverListIdx = -1;
        g_closeHover = false;
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);

        // Single Red Close Button -> Closes to tray
        if (x >= 18 && x <= 34 && y >= 16 && y <= 32) {
            ShowWindow(hWnd, SW_HIDE);
            return 0;
        }

        // Direct Tab Switching on Click
        if (PtInRectStruct(rcTab0, x, y)) {
            g_activeTab = TAB_APPLOCKER;
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }
        if (PtInRectStruct(rcTab1, x, y)) {
            g_activeTab = TAB_SECURITY;
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }
        if (PtInRectStruct(rcTab2, x, y)) {
            g_activeTab = TAB_UPDATES;
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }
        if (PtInRectStruct(rcTab3, x, y)) {
            g_activeTab = TAB_LOGS;
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }
        if (PtInRectStruct(rcTab4, x, y)) {
            g_activeTab = TAB_ABOUT;
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }

        g_pressedControlId = g_hoverControlId;

        // List Item Selection (matching render origin Y = 70)
        if (g_activeTab == TAB_APPLOCKER && x >= 235 && x <= 810 && y >= 60 && y <= 420) {
            const auto& lockedApps = ConfigManager::GetInstance().GetSettings().lockedApps;
            int itemY = 70;
            for (size_t i = 0; i < lockedApps.size(); ++i) {
                if (y >= itemY && y <= itemY + 36) {
                    g_selectedListIdx = (int)i;
                    InvalidateRect(hWnd, NULL, FALSE);
                    break;
                }
                itemY += 40;
            }
        }
        return 0;
    }

    case WM_LBUTTONUP: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        g_pressedControlId = 0;

        // Page 0 Actions
        if (g_activeTab == TAB_APPLOCKER) {
            if (PtInRectStruct(rcBtnAdd, x, y)) {
                if (!EnsureSecurityConfigured(hWnd)) return 0;

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
                return 0;
            }
            if (PtInRectStruct(rcBtnRemove, x, y)) {
                const auto& lockedApps = ConfigManager::GetInstance().GetSettings().lockedApps;
                if (g_selectedListIdx >= 0 && g_selectedListIdx < (int)lockedApps.size()) {
                    std::wstring appToRemove = lockedApps[g_selectedListIdx];
                    ConfigManager::GetInstance().RemoveLockedApp(appToRemove);
                    g_selectedListIdx = -1;
                    UpdateTrayIconMetrics(hWnd);
                    InvalidateRect(hWnd, NULL, FALSE);
                }
                return 0;
            }
            if (PtInRectStruct(rcPreset1, x, y)) {
                if (!EnsureSecurityConfigured(hWnd)) return 0;
                ConfigManager::GetInstance().AddLockedApp(L"notepad.exe");
                UpdateTrayIconMetrics(hWnd);
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }
            if (PtInRectStruct(rcPreset2, x, y)) {
                if (!EnsureSecurityConfigured(hWnd)) return 0;
                ConfigManager::GetInstance().AddLockedApp(L"chrome.exe");
                UpdateTrayIconMetrics(hWnd);
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }
            if (PtInRectStruct(rcPreset3, x, y)) {
                if (!EnsureSecurityConfigured(hWnd)) return 0;
                ConfigManager::GetInstance().AddLockedApp(L"cmd.exe");
                UpdateTrayIconMetrics(hWnd);
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }
            if (PtInRectStruct(rcPreset4, x, y)) {
                if (!EnsureSecurityConfigured(hWnd)) return 0;
                ConfigManager::GetInstance().AddLockedApp(L"calc.exe");
                UpdateTrayIconMetrics(hWnd);
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }
        }
        // Page 1 Actions
        else if (g_activeTab == TAB_SECURITY) {
            if (PtInRectStruct(rcToggleEnable, x, y)) {
                auto& settings = ConfigManager::GetInstance().GetSettings();
                settings.protectionEnabled = !settings.protectionEnabled;
                ConfigManager::GetInstance().SaveSettings();
                UpdateTrayIconMetrics(hWnd);
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }
            if (PtInRectStruct(rcToggleWinAuth, x, y)) {
                auto& settings = ConfigManager::GetInstance().GetSettings();
                settings.useWindowsAuth = !settings.useWindowsAuth;
                ConfigManager::GetInstance().SaveSettings();
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }
            if (PtInRectStruct(rcToggleCustomPin, x, y)) {
                auto& settings = ConfigManager::GetInstance().GetSettings();
                settings.useCustomPin = !settings.useCustomPin;
                ConfigManager::GetInstance().SaveSettings();
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }
            if (PtInRectStruct(rcToggleAutoStart, x, y)) {
                auto& settings = ConfigManager::GetInstance().GetSettings();
                bool newAutoStart = !settings.autoStartWithWindows;
                ConfigManager::GetInstance().SetAutoStart(newAutoStart);
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }
            if (PtInRectStruct(rcBtnSetPin, x, y)) {
                PromptSetCustomPin(hWnd);
                return 0;
            }
            // Scheduled Protection Actions
            if (PtInRectStruct(rcToggleSchedule, x, y)) {
                auto& settings = ConfigManager::GetInstance().GetSettings();
                settings.scheduleEnabled = !settings.scheduleEnabled;
                ConfigManager::GetInstance().SaveSettings();
                AuditLogger::GetInstance().LogEvent(L"SCHEDULE_TOGGLE", settings.scheduleEnabled ? L"Schedule-based active hours protection enabled." : L"Schedule-based active hours protection disabled.");
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }
            if (PtInRectStruct(rcSchedStartDec, x, y)) {
                auto& settings = ConfigManager::GetInstance().GetSettings();
                if (settings.scheduleStartHour > 0) settings.scheduleStartHour--;
                else settings.scheduleStartHour = 23;
                ConfigManager::GetInstance().SaveSettings();
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }
            if (PtInRectStruct(rcSchedStartInc, x, y)) {
                auto& settings = ConfigManager::GetInstance().GetSettings();
                settings.scheduleStartHour = (settings.scheduleStartHour + 1) % 24;
                ConfigManager::GetInstance().SaveSettings();
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }
            if (PtInRectStruct(rcSchedEndDec, x, y)) {
                auto& settings = ConfigManager::GetInstance().GetSettings();
                if (settings.scheduleEndHour > 0) settings.scheduleEndHour--;
                else settings.scheduleEndHour = 23;
                ConfigManager::GetInstance().SaveSettings();
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }
            if (PtInRectStruct(rcSchedEndInc, x, y)) {
                auto& settings = ConfigManager::GetInstance().GetSettings();
                settings.scheduleEndHour = (settings.scheduleEndHour + 1) % 24;
                ConfigManager::GetInstance().SaveSettings();
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }
            // Enterprise Policy Export / Import
            if (PtInRectStruct(rcBtnExportPolicy, x, y)) {
                wchar_t szFile[MAX_PATH] = L"SurakshaPolicy.suraksha";
                OPENFILENAMEW ofn = { sizeof(ofn) };
                ofn.hwndOwner = hWnd;
                ofn.lpstrFile = szFile;
                ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
                ofn.lpstrFilter = L"Suraksha Policy (*.suraksha)\0*.suraksha\0All Files (*.*)\0*.*\0";
                ofn.lpstrDefExt = L"suraksha";
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

                if (GetSaveFileNameW(&ofn)) {
                    std::wstring passkey = L"";
                    if (PromptPolicyPasskey(hWnd, true, passkey)) {
                        if (ConfigManager::GetInstance().ExportEncryptedPolicy(szFile, passkey)) {
                            AuditLogger::GetInstance().LogEvent(L"POLICY_EXPORT", L"AES-256 policy exported to " + std::wstring(szFile));
                            MessageBoxW(hWnd, L"Enterprise Security Policy exported successfully with AES-256 encryption!", L"Suraksha Enterprise", MB_OK | MB_ICONINFORMATION);
                        } else {
                            MessageBoxW(hWnd, L"Failed to export policy file. Please check file permissions.", L"Export Failed", MB_OK | MB_ICONERROR);
                        }
                    }
                }
                return 0;
            }
            if (PtInRectStruct(rcBtnImportPolicy, x, y)) {
                wchar_t szFile[MAX_PATH] = { 0 };
                OPENFILENAMEW ofn = { sizeof(ofn) };
                ofn.hwndOwner = hWnd;
                ofn.lpstrFile = szFile;
                ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
                ofn.lpstrFilter = L"Suraksha Policy (*.suraksha;*.dat)\0*.suraksha;*.dat\0All Files (*.*)\0*.*\0";
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

                if (GetOpenFileNameW(&ofn)) {
                    std::wstring passkey = L"";
                    if (PromptPolicyPasskey(hWnd, false, passkey)) {
                        if (ConfigManager::GetInstance().ImportEncryptedPolicy(szFile, passkey)) {
                            AuditLogger::GetInstance().LogEvent(L"POLICY_IMPORT", L"AES-256 policy imported from " + std::wstring(szFile));
                            UpdateTrayIconMetrics(hWnd);
                            InvalidateRect(hWnd, NULL, FALSE);
                            MessageBoxW(hWnd, L"Enterprise Security Policy imported and applied successfully!", L"Suraksha Enterprise", MB_OK | MB_ICONINFORMATION);
                        } else {
                            MessageBoxW(hWnd, L"Failed to decrypt policy file. Please verify the decryption passphrase and file integrity.", L"Import Failed", MB_OK | MB_ICONERROR);
                        }
                    }
                }
                return 0;
            }
        }
        // Page 2 Actions (Software Updates)
        else if (g_activeTab == TAB_UPDATES) {
            UpdateStatus st = UpdateManager::GetInstance().GetStatus();
            if (st == UpdateStatus::UpdateAvailable && PtInRectStruct(rcBtnDownload, x, y)) {
                UpdateManager::GetInstance().StartDownloadAsync();
                return 0;
            }
            if (st == UpdateStatus::ReadyToInstall && PtInRectStruct(rcBtnInstall, x, y)) {
                UpdateManager::GetInstance().InstallAndRelaunch();
                return 0;
            }
            if (PtInRectStruct(rcBtnCheckUpdate, x, y)) {
                g_spinAngle = 0.0f;
                SetTimer(hWnd, g_nSpinnerTimerID, 33, NULL);
                UpdateManager::GetInstance().CheckForUpdatesAsync(true);
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }
            if (PtInRectStruct(rcRadioStable, x, y)) {
                UpdateManager::GetInstance().SetChannel(L"stable");
                g_spinAngle = 0.0f;
                SetTimer(hWnd, g_nSpinnerTimerID, 33, NULL);
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }
            if (PtInRectStruct(rcRadioBeta, x, y)) {
                UpdateManager::GetInstance().SetChannel(L"beta");
                g_spinAngle = 0.0f;
                SetTimer(hWnd, g_nSpinnerTimerID, 33, NULL);
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }
        }
        // Page 3 Actions (Audit Logs)
        else if (g_activeTab == TAB_LOGS) {
            if (PtInRectStruct(rcBtnOpenLog, x, y)) {
                wchar_t appData[MAX_PATH] = { 0 };
                if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appData))) {
                    std::wstring logPath = std::wstring(appData) + L"\\Suraksha\\logs\\audit.log";
                    ShellExecuteW(NULL, L"open", logPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
                }
                return 0;
            }
        }
        // Page 4 Actions (About Links & Language Selector)
        else if (g_activeTab == TAB_ABOUT) {
            auto SetAppLang = [&](int langIdx) {
                LanguageManager::GetInstance().SetLanguage((Language)langIdx);
                auto& s = ConfigManager::GetInstance().GetSettings();
                s.language = langIdx;
                ConfigManager::GetInstance().SaveSettings();
                InvalidateRect(hWnd, NULL, FALSE);
            };

            if (PtInRectStruct(rcLangEN, x, y)) { SetAppLang(0); return 0; }
            if (PtInRectStruct(rcLangHI, x, y)) { SetAppLang(1); return 0; }
            if (PtInRectStruct(rcLangES, x, y)) { SetAppLang(2); return 0; }
            if (PtInRectStruct(rcLangDE, x, y)) { SetAppLang(3); return 0; }
            if (PtInRectStruct(rcLangFR, x, y)) { SetAppLang(4); return 0; }
            if (PtInRectStruct(rcLangJA, x, y)) { SetAppLang(5); return 0; }

            if (PtInRectStruct(rcBtnYABP, x, y)) {
                ShellExecuteW(NULL, L"open", L"https://yabp.netlify.app/", NULL, NULL, SW_SHOWNORMAL);
                return 0;
            }
            if (PtInRectStruct(rcBtnDev, x, y)) {
                ShellExecuteW(NULL, L"open", L"https://dheeraz.dpdns.org/", NULL, NULL, SW_SHOWNORMAL);
                return 0;
            }
            if (PtInRectStruct(rcBtnGithub, x, y)) {
                ShellExecuteW(NULL, L"open", L"https://github.com/dheeraz101/Suraksha", NULL, NULL, SW_SHOWNORMAL);
                return 0;
            }
        }

        return 0;
    }

    case WM_NCHITTEST: {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(hWnd, &pt);

        if (pt.x >= 15 && pt.x <= 40 && pt.y >= 15 && pt.y <= 35) {
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
        } else if (wParam == g_nSpinnerTimerID) {
            if (UpdateManager::GetInstance().GetStatus() == UpdateStatus::Checking) {
                g_spinAngle += 15.0f;
                if (g_spinAngle >= 360.0f) g_spinAngle -= 360.0f;
                InvalidateRect(hWnd, NULL, FALSE);
            } else {
                KillTimer(hWnd, g_nSpinnerTimerID);
                g_spinAngle = 0.0f;
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
        return 0;
    }

    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case ID_TRAY_OPEN:
            ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);
            break;
        case ID_TRAY_TOGGLE: {
            auto& settings = ConfigManager::GetInstance().GetSettings();
            settings.protectionEnabled = !settings.protectionEnabled;
            ConfigManager::GetInstance().SaveSettings();
            UpdateTrayIconMetrics(hWnd);
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        }
        case ID_TRAY_LOCKALL:
            AppLockEngine::GetInstance().LockAllProcesses();
            AuditLogger::GetInstance().LogEvent(L"HOTKEY_TRIGGERED", L"Tray command: All protected sessions locked.");
            break;
        case ID_TRAY_EXIT:
            TrayIcon::GetInstance().Remove();
            DestroyWindow(hWnd);
            break;
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

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP hMemBmp = CreateCompatibleBitmap(hdc, winW, winH);
        HBITMAP hOldBmp = (HBITMAP)SelectObject(memDC, hMemBmp);

        // Pre-fill memory DC with Dark Content Background (#141417)
        HBRUSH hBg = CreateSolidBrush(RGB(20, 20, 23));
        FillRect(memDC, &rect, hBg);
        DeleteObject(hBg);

        {
            Graphics graphics(memDC);
            graphics.SetSmoothingMode(SmoothingModeAntiAlias);
            graphics.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

            // 1. Draw Left Sidebar Background (#17171A)
            SolidBrush sidebarBrush(Color(255, 23, 23, 26));
            graphics.FillRectangle(&sidebarBrush, 0, 0, 210, winH);

            Pen sidebarDivider(Color(18, 255, 255, 255), 1.0f);
            graphics.DrawLine(&sidebarDivider, 210, 0, 210, winH);

            // Single Red Close Button
            UIComponents::DrawCloseButton(graphics, 20, 18, 13, g_closeHover);

            // Sidebar Logo + Brand Title
            UIComponents::DrawAppLogo(graphics, 20, 52, 22);

            // Premium Modern Typography Stack with Inter / Segoe UI Variable fallback
            FontManager::GetInstance().Initialize();
            const FontFamily* pDisplayFam = FontManager::GetInstance().GetDisplayFamily();
            const FontFamily* pTextFam = FontManager::GetInstance().GetTextFamily();

            Font brandFont(pDisplayFam, 13.5f, FontStyleBold, UnitPoint);
            Font pageHeadFont(pDisplayFam, 16.0f, FontStyleBold, UnitPoint);
            Font sectionFont(pDisplayFam, 12.0f, FontStyleBold, UnitPoint);
            Font bodyFont(pTextFam, 10.5f, FontStyleRegular, UnitPoint);
            Font bodyBoldFont(pTextFam, 10.5f, FontStyleBold, UnitPoint);

            SolidBrush whiteBrush(Color(255, 248, 250, 252));
            SolidBrush mutedBrush(Color(255, 148, 163, 184));
            SolidBrush blueBrush(Color(255, 10, 132, 255));

            StringFormat formatLeft;
            formatLeft.SetAlignment(StringAlignmentNear);
            formatLeft.SetLineAlignment(StringAlignmentCenter);

            RectF brandRect(48.0f, 50.0f, 150.0f, 26.0f);
            graphics.DrawString(L"Suraksha", -1, &brandFont, brandRect, &formatLeft, &whiteBrush);

            // 2. Draw 5 Sidebar Navigation Tabs with Native MDL2 / Fluent Icons
            bool hovTab0 = (g_hoverControlId == ID_CANVAS_TAB_APPLOCKER);
            UIComponents::DrawCanvasListItem(graphics, rcTab0.left, rcTab0.top, rcTab0.right - rcTab0.left, rcTab0.bottom - rcTab0.top,
                LanguageManager::GetInstance().GetString(L"TAB_APPLOCKER"), (g_activeTab == TAB_APPLOCKER), hovTab0, VectorIcon::Lock);

            bool hovTab1 = (g_hoverControlId == ID_CANVAS_TAB_SECURITY);
            UIComponents::DrawCanvasListItem(graphics, rcTab1.left, rcTab1.top, rcTab1.right - rcTab1.left, rcTab1.bottom - rcTab1.top,
                LanguageManager::GetInstance().GetString(L"TAB_SECURITY"), (g_activeTab == TAB_SECURITY), hovTab1, VectorIcon::Shield);

            bool hovTab2 = (g_hoverControlId == ID_CANVAS_TAB_UPDATES);
            UIComponents::DrawCanvasListItem(graphics, rcTab2.left, rcTab2.top, rcTab2.right - rcTab2.left, rcTab2.bottom - rcTab2.top,
                LanguageManager::GetInstance().GetString(L"TAB_UPDATES"), (g_activeTab == TAB_UPDATES), hovTab2, VectorIcon::Update);

            bool hovTab3 = (g_hoverControlId == ID_CANVAS_TAB_LOGS);
            UIComponents::DrawCanvasListItem(graphics, rcTab3.left, rcTab3.top, rcTab3.right - rcTab3.left, rcTab3.bottom - rcTab3.top,
                LanguageManager::GetInstance().GetString(L"TAB_LOGS"), (g_activeTab == TAB_LOGS), hovTab3, VectorIcon::Logs);

            bool hovTab4 = (g_hoverControlId == ID_CANVAS_TAB_ABOUT);
            UIComponents::DrawCanvasListItem(graphics, rcTab4.left, rcTab4.top, rcTab4.right - rcTab4.left, rcTab4.bottom - rcTab4.top,
                LanguageManager::GetInstance().GetString(L"TAB_ABOUT"), (g_activeTab == TAB_ABOUT), hovTab4, VectorIcon::Info);

            // Top Status Badge
            const auto& settings = ConfigManager::GetInstance().GetSettings();
            bool protectionActive = settings.protectionEnabled;
            std::wstring statusBadgeText = protectionActive ? 
                LanguageManager::GetInstance().GetString(L"STATUS_PROTECTED") : 
                LanguageManager::GetInstance().GetString(L"STATUS_PAUSED");
            UIComponents::DrawStatusBadge(graphics, 675, 16, 135, 28, statusBadgeText, protectionActive);

            // ================= PAGE 0: APP LOCKER =================
            if (g_activeTab == TAB_APPLOCKER) {
                RectF headRect(235.0f, 16.0f, 400.0f, 28.0f);
                graphics.DrawString(LanguageManager::GetInstance().GetString(L"PROTECTED_APPS").c_str(), -1, &pageHeadFont, headRect, &formatLeft, &whiteBrush);

                UIComponents::DrawCanvasCard(graphics, 235, 60, 575, 360, Color(255, 26, 26, 30), Color(18, 255, 255, 255), 14);

                const auto& lockedApps = settings.lockedApps;
                if (lockedApps.empty()) {
                    UIComponents::DrawEmptyState(graphics, 235, 60, 575, 360, L"No Protected Applications", L"Click 'Add Application' or choose a quick preset below");
                } else {
                    int itemY = 70;
                    for (size_t i = 0; i < lockedApps.size(); ++i) {
                        if (itemY + 36 > 410) break;
                        bool isSel = (g_selectedListIdx == (int)i);
                        bool isHov = (g_hoverListIdx == (int)i);
                        UIComponents::DrawCanvasListItem(graphics, 245, itemY, 555, 36, lockedApps[i], isSel, isHov, VectorIcon::Lock);
                        itemY += 40;
                    }
                }

                // Action Buttons
                bool hovAdd = (g_hoverControlId == ID_CANVAS_BTN_ADD);
                bool prsAdd = (g_pressedControlId == ID_CANVAS_BTN_ADD);
                UIComponents::DrawCanvasButton(graphics, rcBtnAdd.left, rcBtnAdd.top, rcBtnAdd.right - rcBtnAdd.left, rcBtnAdd.bottom - rcBtnAdd.top,
                    LanguageManager::GetInstance().GetString(L"ADD_APP"), ButtonVariant::Primary, hovAdd, prsAdd, VectorIcon::Plus);

                bool hovRem = (g_hoverControlId == ID_CANVAS_BTN_REMOVE);
                bool prsRem = (g_pressedControlId == ID_CANVAS_BTN_REMOVE);
                UIComponents::DrawCanvasButton(graphics, rcBtnRemove.left, rcBtnRemove.top, rcBtnRemove.right - rcBtnRemove.left, rcBtnRemove.bottom - rcBtnRemove.top,
                    LanguageManager::GetInstance().GetString(L"REMOVE_APP"), ButtonVariant::Danger, hovRem, prsRem, VectorIcon::Trash);

                // Quick Presets (Clean text-only pill buttons)
                bool hovP1 = (g_hoverControlId == ID_CANVAS_PRESET_NOTEPAD);
                bool prsP1 = (g_pressedControlId == ID_CANVAS_PRESET_NOTEPAD);
                UIComponents::DrawCanvasButton(graphics, rcPreset1.left, rcPreset1.top, rcPreset1.right - rcPreset1.left, rcPreset1.bottom - rcPreset1.top,
                    L"Notepad", ButtonVariant::Secondary, hovP1, prsP1, VectorIcon::None);

                bool hovP2 = (g_hoverControlId == ID_CANVAS_PRESET_CHROME);
                bool prsP2 = (g_pressedControlId == ID_CANVAS_PRESET_CHROME);
                UIComponents::DrawCanvasButton(graphics, rcPreset2.left, rcPreset2.top, rcPreset2.right - rcPreset2.left, rcPreset2.bottom - rcPreset2.top,
                    L"Google Chrome", ButtonVariant::Secondary, hovP2, prsP2, VectorIcon::None);

                bool hovP3 = (g_hoverControlId == ID_CANVAS_PRESET_CMD);
                bool prsP3 = (g_pressedControlId == ID_CANVAS_PRESET_CMD);
                UIComponents::DrawCanvasButton(graphics, rcPreset3.left, rcPreset3.top, rcPreset3.right - rcPreset3.left, rcPreset3.bottom - rcPreset3.top,
                    L"Terminal", ButtonVariant::Secondary, hovP3, prsP3, VectorIcon::None);

                bool hovP4 = (g_hoverControlId == ID_CANVAS_PRESET_CALC);
                bool prsP4 = (g_pressedControlId == ID_CANVAS_PRESET_CALC);
                UIComponents::DrawCanvasButton(graphics, rcPreset4.left, rcPreset4.top, rcPreset4.right - rcPreset4.left, rcPreset4.bottom - rcPreset4.top,
                    L"Calculator", ButtonVariant::Secondary, hovP4, prsP4, VectorIcon::None);
            }
            // ================= PAGE 1: SECURITY & AUTH =================
            else if (g_activeTab == TAB_SECURITY) {
                RectF headRect(235.0f, 16.0f, 400.0f, 28.0f);
                graphics.DrawString(LanguageManager::GetInstance().GetString(L"SECURITY_DEFENSE").c_str(), -1, &pageHeadFont, headRect, &formatLeft, &whiteBrush);

                // --- Card 1: Primary Authentication Options ---
                UIComponents::DrawCanvasCard(graphics, 235, 52, 575, 142, Color(255, 26, 26, 30), Color(18, 255, 255, 255), 12);

                RectF authTitleRc(255.0f, 66.0f, 500.0f, 20.0f);
                graphics.DrawString(L"Primary Authentication Methods", -1, &sectionFont, authTitleRc, &formatLeft, &whiteBrush);

                bool hovWin = (g_hoverControlId == ID_CANVAS_TOGGLE_WINAUTH);
                UIComponents::DrawCanvasToggle(graphics, rcToggleWinAuth.left, rcToggleWinAuth.top, rcToggleWinAuth.right - rcToggleWinAuth.left, rcToggleWinAuth.bottom - rcToggleWinAuth.top,
                    L"Windows Hello (Biometrics & PIN)", settings.useWindowsAuth, hovWin);

                bool hovCust = (g_hoverControlId == ID_CANVAS_TOGGLE_CUSTOMPIN);
                UIComponents::DrawCanvasToggle(graphics, rcToggleCustomPin.left, rcToggleCustomPin.top, rcToggleCustomPin.right - rcToggleCustomPin.left, rcToggleCustomPin.bottom - rcToggleCustomPin.top,
                    L"Custom Master Passcode", settings.useCustomPin, hovCust);

                bool hovSetPin = (g_hoverControlId == ID_CANVAS_BTN_SETPIN);
                bool prsSetPin = (g_pressedControlId == ID_CANVAS_BTN_SETPIN);
                UIComponents::DrawCanvasButton(graphics, rcBtnSetPin.left, rcBtnSetPin.top, rcBtnSetPin.right - rcBtnSetPin.left, rcBtnSetPin.bottom - rcBtnSetPin.top,
                    SecurityManager::GetInstance().HasCustomPin() ? L"Change Passcode" : L"Set Master Passcode",
                    ButtonVariant::Secondary, hovSetPin, prsSetPin, VectorIcon::Key);

                // --- Card 2: Scheduled Protection (Active Hours) ---
                UIComponents::DrawCanvasCard(graphics, 235, 204, 575, 138, Color(255, 26, 26, 30), Color(18, 255, 255, 255), 12);

                RectF schedTitleRc(255.0f, 216.0f, 500.0f, 20.0f);
                graphics.DrawString(LanguageManager::GetInstance().GetString(L"SCHEDULE_PROTECTION").c_str(), -1, &sectionFont, schedTitleRc, &formatLeft, &whiteBrush);

                bool hovSched = (g_hoverControlId == ID_CANVAS_TOGGLE_SCHEDULE);
                UIComponents::DrawCanvasToggle(graphics, rcToggleSchedule.left, rcToggleSchedule.top, rcToggleSchedule.right - rcToggleSchedule.left, rcToggleSchedule.bottom - rcToggleSchedule.top,
                    LanguageManager::GetInstance().GetString(L"SCHEDULE_ENABLE"), settings.scheduleEnabled, hovSched);

                // Stepper Controls
                RectF startLblRc((REAL)rcSchedStartDec.left, (REAL)(rcSchedStartDec.top - 18), 120.0f, 16.0f);
                graphics.DrawString(LanguageManager::GetInstance().GetString(L"SCHEDULE_START").c_str(), -1, &bodyFont, startLblRc, &formatLeft, &mutedBrush);

                bool hovS1 = (g_hoverControlId == ID_CANVAS_BTN_SCHED_S_DEC);
                bool prsS1 = (g_pressedControlId == ID_CANVAS_BTN_SCHED_S_DEC);
                UIComponents::DrawCanvasButton(graphics, rcSchedStartDec.left, rcSchedStartDec.top, 36, 34, L"-", ButtonVariant::Secondary, hovS1, prsS1);

                wchar_t startBuf[32];
                swprintf_s(startBuf, 32, L"%02d:00", settings.scheduleStartHour);
                RectF startValRc((REAL)(rcSchedStartDec.left + 36), (REAL)rcSchedStartDec.top, 56.0f, 34.0f);
                StringFormat fmtVal;
                fmtVal.SetAlignment(StringAlignmentCenter);
                fmtVal.SetLineAlignment(StringAlignmentCenter);
                graphics.DrawString(startBuf, -1, &sectionFont, startValRc, &fmtVal, &whiteBrush);

                bool hovS2 = (g_hoverControlId == ID_CANVAS_BTN_SCHED_S_INC);
                bool prsS2 = (g_pressedControlId == ID_CANVAS_BTN_SCHED_S_INC);
                UIComponents::DrawCanvasButton(graphics, rcSchedStartInc.left, rcSchedStartInc.top, 36, 34, L"+", ButtonVariant::Secondary, hovS2, prsS2);

                RectF endLblRc((REAL)rcSchedEndDec.left, (REAL)(rcSchedEndDec.top - 18), 120.0f, 16.0f);
                graphics.DrawString(LanguageManager::GetInstance().GetString(L"SCHEDULE_END").c_str(), -1, &bodyFont, endLblRc, &formatLeft, &mutedBrush);

                bool hovE1 = (g_hoverControlId == ID_CANVAS_BTN_SCHED_E_DEC);
                bool prsE1 = (g_pressedControlId == ID_CANVAS_BTN_SCHED_E_DEC);
                UIComponents::DrawCanvasButton(graphics, rcSchedEndDec.left, rcSchedEndDec.top, 36, 34, L"-", ButtonVariant::Secondary, hovE1, prsE1);

                wchar_t endBuf[32];
                swprintf_s(endBuf, 32, L"%02d:00", settings.scheduleEndHour);
                RectF endValRc((REAL)(rcSchedEndDec.left + 36), (REAL)rcSchedEndDec.top, 56.0f, 34.0f);
                graphics.DrawString(endBuf, -1, &sectionFont, endValRc, &fmtVal, &whiteBrush);

                bool hovE2 = (g_hoverControlId == ID_CANVAS_BTN_SCHED_E_INC);
                bool prsE2 = (g_pressedControlId == ID_CANVAS_BTN_SCHED_E_INC);
                UIComponents::DrawCanvasButton(graphics, rcSchedEndInc.left, rcSchedEndInc.top, 36, 34, L"+", ButtonVariant::Secondary, hovE2, prsE2);

                // --- Card 3: Enterprise Policy Export / Import (AES-256) ---
                UIComponents::DrawCanvasCard(graphics, 235, 352, 575, 118, Color(255, 26, 26, 30), Color(18, 255, 255, 255), 12);

                RectF polTitleRc(255.0f, 364.0f, 500.0f, 20.0f);
                graphics.DrawString(LanguageManager::GetInstance().GetString(L"POLICY_ENTERPRISE").c_str(), -1, &sectionFont, polTitleRc, &formatLeft, &whiteBrush);

                RectF polDescRc(255.0f, 386.0f, 535.0f, 18.0f);
                graphics.DrawString(LanguageManager::GetInstance().GetString(L"POLICY_DESC").c_str(), -1, &bodyFont, polDescRc, &formatLeft, &mutedBrush);

                bool hovExp = (g_hoverControlId == ID_CANVAS_BTN_EXPORT_POL);
                bool prsExp = (g_pressedControlId == ID_CANVAS_BTN_EXPORT_POL);
                UIComponents::DrawCanvasButton(graphics, rcBtnExportPolicy.left, rcBtnExportPolicy.top, rcBtnExportPolicy.right - rcBtnExportPolicy.left, rcBtnExportPolicy.bottom - rcBtnExportPolicy.top,
                    LanguageManager::GetInstance().GetString(L"POLICY_EXPORT"), ButtonVariant::Secondary, hovExp, prsExp, VectorIcon::Export);

                bool hovImp = (g_hoverControlId == ID_CANVAS_BTN_IMPORT_POL);
                bool prsImp = (g_pressedControlId == ID_CANVAS_BTN_IMPORT_POL);
                UIComponents::DrawCanvasButton(graphics, rcBtnImportPolicy.left, rcBtnImportPolicy.top, rcBtnImportPolicy.right - rcBtnImportPolicy.left, rcBtnImportPolicy.bottom - rcBtnImportPolicy.top,
                    LanguageManager::GetInstance().GetString(L"POLICY_IMPORT"), ButtonVariant::Secondary, hovImp, prsImp, VectorIcon::Import);

                // Auto-start switch
                bool hovAuto = (g_hoverControlId == ID_CANVAS_TOGGLE_AUTOSTART);
                UIComponents::DrawCanvasToggle(graphics, rcToggleAutoStart.left, rcToggleAutoStart.top, rcToggleAutoStart.right - rcToggleAutoStart.left, rcToggleAutoStart.bottom - rcToggleAutoStart.top,
                    L"Start Suraksha automatically with Windows", settings.autoStartWithWindows, hovAuto);
            }
            // ================= PAGE 2: SOFTWARE UPDATE (macOS Style) =================
            else if (g_activeTab == TAB_UPDATES) {
                RectF headRect(235.0f, 16.0f, 400.0f, 28.0f);
                graphics.DrawString(LanguageManager::GetInstance().GetString(L"TAB_UPDATES").c_str(), -1, &pageHeadFont, headRect, &formatLeft, &whiteBrush);

                // --- Upper Hero Card: Status & Download ---
                UIComponents::DrawCanvasCard(graphics, 235, 60, 575, 215, Color(255, 26, 26, 30), Color(18, 255, 255, 255), 14);

                UpdateStatus status = UpdateManager::GetInstance().GetStatus();
                const auto& relInfo = UpdateManager::GetInstance().GetAvailableRelease();
                std::wstring channel = UpdateManager::GetInstance().GetChannel();
                bool isBeta = (channel == L"beta");

                // Large Status Emblem (52x52)
                int emblemX = 255;
                int emblemY = 80;
                Color emblemBg = Color(255, 36, 36, 42);
                VectorIcon emblemIcon = VectorIcon::Check;
                Color iconColor = Color(255, 52, 199, 89); // Green

                if (status == UpdateStatus::Checking) {
                    emblemBg = Color(255, 20, 35, 60);
                    emblemIcon = VectorIcon::Update;
                    iconColor = Color(255, 10, 132, 255);
                } else if (status == UpdateStatus::UpdateAvailable || status == UpdateStatus::Downloading) {
                    emblemBg = Color(255, 10, 50, 95);
                    emblemIcon = (status == UpdateStatus::Downloading) ? VectorIcon::Download : VectorIcon::Update;
                    iconColor = Color(255, 10, 132, 255);
                } else if (status == UpdateStatus::ReadyToInstall) {
                    emblemBg = Color(255, 20, 55, 35);
                    emblemIcon = VectorIcon::Check;
                    iconColor = Color(255, 52, 199, 89);
                } else if (status == UpdateStatus::Error) {
                    emblemBg = Color(255, 60, 30, 20);
                    emblemIcon = VectorIcon::Warning;
                    iconColor = Color(255, 255, 69, 58);
                }

                if (status == UpdateStatus::Checking) {
                    SetTimer(hWnd, g_nSpinnerTimerID, 33, NULL);
                }

                UIComponents::DrawCanvasCard(graphics, emblemX, emblemY, 52, 52, emblemBg, Color(30, 255, 255, 255), 14);
                if (status == UpdateStatus::Checking) {
                    GraphicsState gState = graphics.Save();
                    graphics.TranslateTransform((REAL)(emblemX + 26), (REAL)(emblemY + 26));
                    graphics.RotateTransform(g_spinAngle);
                    graphics.TranslateTransform(-(REAL)(emblemX + 26), -(REAL)(emblemY + 26));
                    UIComponents::DrawIconUpdate(graphics, emblemX + 15, emblemY + 15, 22, iconColor);
                    graphics.Restore(gState);
                } else if (emblemIcon == VectorIcon::Check) UIComponents::DrawIconCheck(graphics, emblemX + 16, emblemY + 16, 20, iconColor);
                else if (emblemIcon == VectorIcon::Download) UIComponents::DrawIconDownload(graphics, emblemX + 16, emblemY + 16, 20, iconColor);
                else if (emblemIcon == VectorIcon::Warning) UIComponents::DrawIconWarning(graphics, emblemX + 16, emblemY + 16, 20, iconColor);
                else UIComponents::DrawIconUpdate(graphics, emblemX + 16, emblemY + 16, 20, iconColor);

                // Headline text beside Emblem
                std::wstring mainTitle = L"Suraksha is up to date";
                if (status == UpdateStatus::Checking) mainTitle = L"Checking for updates...";
                else if (status == UpdateStatus::UpdateAvailable) mainTitle = L"Suraksha v" + relInfo.version + L" is available";
                else if (status == UpdateStatus::Downloading) mainTitle = L"Downloading Suraksha Update...";
                else if (status == UpdateStatus::ReadyToInstall) mainTitle = L"Update Ready to Install";
                else if (status == UpdateStatus::Error) mainTitle = L"Update Check Failed";

                RectF titleRc(322.0f, 80.0f, 470.0f, 24.0f);
                graphics.DrawString(mainTitle.c_str(), -1, &sectionFont, titleRc, &formatLeft, &whiteBrush);

                // Subtitle: Version info
                std::wstring verLine = L"Installed: Version " + std::wstring(SURAKSHA_VERSION_STRING) + L" \x2022 Build " + std::wstring(SURAKSHA_BUILD_TAG);
                RectF verRc(322.0f, 106.0f, 470.0f, 20.0f);
                graphics.DrawString(verLine.c_str(), -1, &bodyFont, verRc, &formatLeft, &mutedBrush);

                // Subtitle: Last checked
                std::wstring lastCheckLine = L"Last checked: " + UpdateManager::GetInstance().GetLastCheckedString();
                RectF checkRc(322.0f, 126.0f, 470.0f, 20.0f);
                graphics.DrawString(lastCheckLine.c_str(), -1, &bodyFont, checkRc, &formatLeft, &mutedBrush);

                // State-Specific Action Controls
                if (status == UpdateStatus::Downloading) {
                    int pct = UpdateManager::GetInstance().GetDownloadProgress();
                    UIComponents::DrawProgressBar(graphics, 255, 170, 535, 8, pct);

                    wchar_t pctBuf[128];
                    size_t downBytes = UpdateManager::GetInstance().GetDownloadedBytes();
                    size_t totBytes = UpdateManager::GetInstance().GetTotalBytes();
                    if (totBytes > 0) {
                        swprintf_s(pctBuf, 128, L"%d%% completed (%.1f MB / %.1f MB)", pct, downBytes / 1048576.0f, totBytes / 1048576.0f);
                    } else {
                        swprintf_s(pctBuf, 128, L"Downloading... %.1f MB", downBytes / 1048576.0f);
                    }
                    RectF pctRc(255.0f, 185.0f, 535.0f, 20.0f);
                    graphics.DrawString(pctBuf, -1, &bodyFont, pctRc, &formatLeft, &mutedBrush);
                } else if (status == UpdateStatus::UpdateAvailable) {
                    bool hovDown = (g_hoverControlId == ID_CANVAS_BTN_DOWNLOAD);
                    bool prsDown = (g_pressedControlId == ID_CANVAS_BTN_DOWNLOAD);
                    std::wstring downBtnText = L"Download & Update (v" + relInfo.version + L")";
                    UIComponents::DrawCanvasButton(graphics, rcBtnDownload.left, rcBtnDownload.top, rcBtnDownload.right - rcBtnDownload.left, rcBtnDownload.bottom - rcBtnDownload.top,
                        downBtnText.c_str(), ButtonVariant::Primary, hovDown, prsDown, VectorIcon::Download);

                    bool hovChk = (g_hoverControlId == ID_CANVAS_BTN_CHECKUPDATE);
                    bool prsChk = (g_pressedControlId == ID_CANVAS_BTN_CHECKUPDATE);
                    UIComponents::DrawCanvasButton(graphics, 460, 220, 160, 42,
                        L"Check Again", ButtonVariant::Secondary, hovChk, prsChk, VectorIcon::Update);
                } else if (status == UpdateStatus::ReadyToInstall) {
                    bool hovInst = (g_hoverControlId == ID_CANVAS_BTN_INSTALL);
                    bool prsInst = (g_pressedControlId == ID_CANVAS_BTN_INSTALL);
                    UIComponents::DrawCanvasButton(graphics, rcBtnInstall.left, rcBtnInstall.top, rcBtnInstall.right - rcBtnInstall.left, rcBtnInstall.bottom - rcBtnInstall.top,
                        L"Restart & Install Update Now", ButtonVariant::Primary, hovInst, prsInst, VectorIcon::Check);
                } else {
                    bool hovChk = (g_hoverControlId == ID_CANVAS_BTN_CHECKUPDATE);
                    bool prsChk = (g_pressedControlId == ID_CANVAS_BTN_CHECKUPDATE);
                    UIComponents::DrawCanvasButton(graphics, rcBtnCheckUpdate.left, rcBtnCheckUpdate.top, rcBtnCheckUpdate.right - rcBtnCheckUpdate.left, rcBtnCheckUpdate.bottom - rcBtnCheckUpdate.top,
                        (status == UpdateStatus::Checking ? L"Checking..." : L"Check for Updates"), ButtonVariant::Secondary, hovChk, prsChk, VectorIcon::Update);
                }

                // --- Lower Section: Release Channel Preference ---
                UIComponents::DrawCanvasCard(graphics, 235, 290, 575, 230, Color(255, 26, 26, 30), Color(18, 255, 255, 255), 14);

                RectF chanHeadRc(255.0f, 305.0f, 535.0f, 22.0f);
                graphics.DrawString(L"Update Channels", -1, &sectionFont, chanHeadRc, &formatLeft, &whiteBrush);

                // Stable Channel Card
                bool isStableActive = !isBeta;
                bool hovStab = (g_hoverControlId == ID_CANVAS_RADIO_STABLE);
                Color bgStab = isStableActive ? Color(255, 20, 36, 26) : (hovStab ? Color(255, 38, 38, 44) : Color(255, 30, 30, 34));
                Color brdStab = isStableActive ? Color(255, 52, 199, 89) : Color(25, 255, 255, 255);
                UIComponents::DrawCanvasCard(graphics, rcRadioStable.left, rcRadioStable.top, rcRadioStable.right - rcRadioStable.left, rcRadioStable.bottom - rcRadioStable.top, bgStab, brdStab, 10);

                UIComponents::DrawVectorRadio(graphics, rcRadioStable.left + 14, rcRadioStable.top + 14, 16, isStableActive, Color(255, 52, 199, 89));

                RectF stabTitleRc((REAL)(rcRadioStable.left + 38), (REAL)(rcRadioStable.top + 12), 210.0f, 20.0f);
                graphics.DrawString(L"Stable (Recommended)", -1, &sectionFont, stabTitleRc, &formatLeft, isStableActive ? &whiteBrush : &mutedBrush);

                RectF stabDescRc((REAL)(rcRadioStable.left + 14), (REAL)(rcRadioStable.top + 34), 230.0f, 32.0f);
                graphics.DrawString(L"Official thoroughly tested releases with guaranteed stability.", -1, &bodyFont, stabDescRc, &formatLeft, &mutedBrush);

                // Beta Channel Card
                bool isBetaActive = isBeta;
                bool hovBeta = (g_hoverControlId == ID_CANVAS_RADIO_BETA);
                Color bgBeta = isBetaActive ? Color(255, 42, 32, 16) : (hovBeta ? Color(255, 38, 38, 44) : Color(255, 30, 30, 34));
                Color brdBeta = isBetaActive ? Color(255, 255, 159, 10) : Color(25, 255, 255, 255);
                UIComponents::DrawCanvasCard(graphics, rcRadioBeta.left, rcRadioBeta.top, rcRadioBeta.right - rcRadioBeta.left, rcRadioBeta.bottom - rcRadioBeta.top, bgBeta, brdBeta, 10);

                UIComponents::DrawVectorRadio(graphics, rcRadioBeta.left + 14, rcRadioBeta.top + 14, 16, isBetaActive, Color(255, 255, 159, 10));

                RectF betaTitleRc((REAL)(rcRadioBeta.left + 38), (REAL)(rcRadioBeta.top + 12), 210.0f, 20.0f);
                graphics.DrawString(L"Beta Channel", -1, &sectionFont, betaTitleRc, &formatLeft, isBetaActive ? &whiteBrush : &mutedBrush);

                RectF betaDescRc((REAL)(rcRadioBeta.left + 14), (REAL)(rcRadioBeta.top + 34), 230.0f, 32.0f);
                graphics.DrawString(L"Preview early builds with newest features and rapid updates.", -1, &bodyFont, betaDescRc, &formatLeft, &mutedBrush);

                // Beta Warning Notice Banner
                if (isBetaActive) {
                    UIComponents::DrawCanvasCard(graphics, 255, 420, 535, 85, Color(255, 42, 30, 16), Color(255, 255, 159, 10), 10);
                    UIComponents::DrawIconWarning(graphics, 270, 436, 20, Color(255, 255, 159, 10));

                    RectF warnHeadRc(300.0f, 430.0f, 475.0f, 20.0f);
                    SolidBrush amberBrush(Color(255, 255, 159, 10));
                    graphics.DrawString(L"Beta Channel Active - Experimental Builds", -1, &sectionFont, warnHeadRc, &formatLeft, &amberBrush);

                    RectF warnBodyRc(300.0f, 452.0f, 475.0f, 45.0f);
                    graphics.DrawString(L"Beta releases receive frequent updates and experimental features, but may contain bugs or stability issues. Recommended for testing only.", -1, &bodyFont, warnBodyRc, &formatLeft, &mutedBrush);
                } else {
                    UIComponents::DrawCanvasCard(graphics, 255, 420, 535, 85, Color(255, 20, 32, 24), Color(255, 52, 199, 89), 10);
                    UIComponents::DrawIconCheck(graphics, 270, 436, 20, Color(255, 52, 199, 89));

                    RectF stabHeadRc(300.0f, 430.0f, 475.0f, 20.0f);
                    SolidBrush greenBrush(Color(255, 52, 199, 89));
                    graphics.DrawString(L"Stable Channel Active - Production Release", -1, &sectionFont, stabHeadRc, &formatLeft, &greenBrush);

                    RectF stabBodyRc(300.0f, 452.0f, 475.0f, 45.0f);
                    graphics.DrawString(L"You will only receive official, thoroughly validated updates. Recommended for all daily protection workflows.", -1, &bodyFont, stabBodyRc, &formatLeft, &mutedBrush);
                }
            }
            // ================= PAGE 3: AUDIT LOGS =================
            else if (g_activeTab == TAB_LOGS) {
                RectF headRect(235.0f, 16.0f, 400.0f, 28.0f);
                graphics.DrawString(LanguageManager::GetInstance().GetString(L"TAB_AUDIT").c_str(), -1, &pageHeadFont, headRect, &formatLeft, &whiteBrush);

                UIComponents::DrawCanvasCard(graphics, 235, 60, 575, 400, Color(255, 26, 26, 30), Color(18, 255, 255, 255), 14);

                auto logLines = GetRecentAuditLogs(6);
                if (logLines.empty()) {
                    UIComponents::DrawEmptyState(graphics, 235, 60, 575, 400, L"No Audit Events Logged", L"All security operations will be tracked here in real-time");
                } else {
                    Font fontMono(pTextFam, 8.5f, FontStyleRegular, UnitPoint);
                    int logY = 72;
                    for (const auto& logEntry : logLines) {
                        UIComponents::DrawCanvasCard(graphics, 248, logY, 549, 48, Color(255, 32, 32, 36), Color(15, 255, 255, 255), 6);
                        RectF logRect(258.0f, (REAL)(logY + 4), 530.0f, 40.0f);
                        graphics.DrawString(logEntry.c_str(), -1, &fontMono, logRect, &formatLeft, &mutedBrush);
                        logY += 54;
                    }
                }

                bool hovOpenLog = (g_hoverControlId == ID_CANVAS_BTN_OPENLOG);
                bool prsOpenLog = (g_pressedControlId == ID_CANVAS_BTN_OPENLOG);
                UIComponents::DrawCanvasButton(graphics, rcBtnOpenLog.left, rcBtnOpenLog.top, rcBtnOpenLog.right - rcBtnOpenLog.left, rcBtnOpenLog.bottom - rcBtnOpenLog.top,
                    L"Open Complete Audit Log File", ButtonVariant::Secondary, hovOpenLog, prsOpenLog, VectorIcon::Logs);
            }
            // ================= PAGE 4: ABOUT SURAKSHA =================
            else if (g_activeTab == TAB_ABOUT) {
                RectF headRect(235.0f, 16.0f, 400.0f, 28.0f);
                graphics.DrawString(LanguageManager::GetInstance().GetString(L"TAB_ABOUT").c_str(), -1, &pageHeadFont, headRect, &formatLeft, &whiteBrush);

                UIComponents::DrawCanvasCard(graphics, 235, 52, 575, 472, Color(255, 26, 26, 30), Color(18, 255, 255, 255), 14);

                // App Branding Logo & Header
                UIComponents::DrawAppLogo(graphics, 495, 60, 40);

                StringFormat formatCenter;
                formatCenter.SetAlignment(StringAlignmentCenter);
                formatCenter.SetLineAlignment(StringAlignmentCenter);

                RectF titleRc(250.0f, 102.0f, 545.0f, 22.0f);
                std::wstring aboutTitle = L"Suraksha - v" + std::wstring(SURAKSHA_DISPLAY_VERSION);
                graphics.DrawString(aboutTitle.c_str(), -1, &sectionFont, titleRc, &formatCenter, &whiteBrush);

                RectF yabpRc(250.0f, 124.0f, 545.0f, 18.0f);
                graphics.DrawString(L"An YABP Initiative (Yet Another Boring Project) | Developed by Dheeraz", -1, &bodyFont, yabpRc, &formatCenter, &mutedBrush);

                // Display Language Selector Grid
                UIComponents::DrawCanvasCard(graphics, 250, 146, 545, 126, Color(255, 34, 34, 38), Color(20, 255, 255, 255), 10);
                RectF langHeadRc(265.0f, 154.0f, 515.0f, 20.0f);
                graphics.DrawString(LanguageManager::GetInstance().GetString(L"LANGUAGE_SECTION").c_str(), -1, &sectionFont, langHeadRc, &formatLeft, &whiteBrush);

                int curLang = settings.language;
                auto DrawLangBtn = [&](const RECT& r, const wchar_t* name, int langId, int cId, const FontFamily* pFam) {
                    bool isSel = (curLang == langId);
                    bool isHov = (g_hoverControlId == cId);
                    Color bColor = isSel ? Color(255, 10, 132, 255) : (isHov ? Color(255, 55, 55, 62) : Color(255, 42, 42, 46));
                    Color brdColor = isSel ? Color(255, 10, 132, 255) : Color(30, 255, 255, 255);
                    UIComponents::DrawCanvasCard(graphics, r.left, r.top, r.right - r.left, r.bottom - r.top, bColor, brdColor, 8);
                    Font btnFont(pFam ? pFam : pTextFam, 10.0f, FontStyleRegular, UnitPoint);
                    RectF rF((REAL)r.left, (REAL)r.top, (REAL)(r.right - r.left), (REAL)(r.bottom - r.top));
                    graphics.DrawString(name, -1, &btnFont, rF, &formatCenter, isSel ? &whiteBrush : &mutedBrush);
                };

                const FontFamily* pHindiFam = FontManager::GetInstance().GetHindiFamily();
                const FontFamily* pJapaneseFam = FontManager::GetInstance().GetJapaneseFamily();

                DrawLangBtn(rcLangEN, L"English", 0, ID_CANVAS_LANG_EN, pTextFam);
                DrawLangBtn(rcLangHI, L"\x0939\x093F\x0928\x094D\x0926\x094D\x0940 (Hindi)", 1, ID_CANVAS_LANG_HI, pHindiFam);
                DrawLangBtn(rcLangES, L"Espa\x00F1ol (Spanish)", 2, ID_CANVAS_LANG_ES, pTextFam);
                DrawLangBtn(rcLangDE, L"Deutsch (German)", 3, ID_CANVAS_LANG_DE, pTextFam);
                DrawLangBtn(rcLangFR, L"Fran\x00E7ais (French)", 4, ID_CANVAS_LANG_FR, pTextFam);
                DrawLangBtn(rcLangJA, L"\x65E5\x672C\x8A9E (Japanese)", 5, ID_CANVAS_LANG_JA, pJapaneseFam);

                // GPLv3 License Box
                UIComponents::DrawCanvasCard(graphics, 250, 280, 545, 138, Color(255, 34, 34, 38), Color(18, 255, 255, 255), 10);

                RectF licHeadRc(265.0f, 290.0f, 515.0f, 20.0f);
                graphics.DrawString(L"GNU General Public License v3.0 (GPLv3)", -1, &sectionFont, licHeadRc, &formatLeft, &whiteBrush);

                std::wstring fullLicDesc = L"This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3 of the License.\n\nThis program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.";
                RectF licBodyRc(265.0f, 312.0f, 515.0f, 96.0f);
                graphics.DrawString(fullLicDesc.c_str(), -1, &bodyFont, licBodyRc, &formatLeft, &mutedBrush);

                // Action Buttons
                bool hovY = (g_hoverControlId == ID_CANVAS_BTN_YABP);
                bool prsY = (g_pressedControlId == ID_CANVAS_BTN_YABP);
                UIComponents::DrawCanvasButton(graphics, rcBtnYABP.left, rcBtnYABP.top, rcBtnYABP.right - rcBtnYABP.left, rcBtnYABP.bottom - rcBtnYABP.top,
                    L"Visit YABP Site", ButtonVariant::Primary, hovY, prsY, VectorIcon::ExternalLink);

                bool hovD = (g_hoverControlId == ID_CANVAS_BTN_DEV);
                bool prsD = (g_pressedControlId == ID_CANVAS_BTN_DEV);
                UIComponents::DrawCanvasButton(graphics, rcBtnDev.left, rcBtnDev.top, rcBtnDev.right - rcBtnDev.left, rcBtnDev.bottom - rcBtnDev.top,
                    L"Developer Site", ButtonVariant::Secondary, hovD, prsD, VectorIcon::ExternalLink);

                bool hovG = (g_hoverControlId == ID_CANVAS_BTN_GITHUB);
                bool prsG = (g_pressedControlId == ID_CANVAS_BTN_GITHUB);
                UIComponents::DrawCanvasButton(graphics, rcBtnGithub.left, rcBtnGithub.top, rcBtnGithub.right - rcBtnGithub.left, rcBtnGithub.bottom - rcBtnGithub.top,
                    L"View Source Code", ButtonVariant::Secondary, hovG, prsG, VectorIcon::ExternalLink);
            }
        }

        // Direct Blit onto monitor
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
