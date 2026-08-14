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

// Software Update Controls
#define ID_CANVAS_BTN_CHECKUPDATE  3030
#define ID_CANVAS_BTN_DOWNLOAD     3031
#define ID_CANVAS_BTN_INSTALL      3032
#define ID_CANVAS_RADIO_STABLE     3033
#define ID_CANVAS_RADIO_BETA       3034

#define ID_CANVAS_BTN_OPENLOG      3040
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
ULONG_PTR g_gdiplusToken = 0;

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
            L"Before protecting applications, please choose your authorization method:\n\n"
            L"• Click YES to authorize via Windows Hello / PIN / Biometrics (Recommended)\n"
            L"• Click NO to set a Custom Master Passcode\n"
            L"• Click CANCEL to abort",
            L"Suraksha - Security Setup Required",
            MB_YESNOCANCEL | MB_ICONQUESTION);

        if (choice == IDYES) {
            settings.useWindowsAuth = true;
            ConfigManager::GetInstance().SaveSettings();
            AuditLogger::GetInstance().LogEvent(L"SECURITY_SETUP", L"Windows Hello / PIN configured as primary unlock method.");
            return true;
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

    // Page 1: Security Controls
    const RECT rcToggleEnable    = { 255, 120, 785, 158 };
    const RECT rcToggleWinAuth   = { 255, 180, 785, 218 };
    const RECT rcToggleCustomPin = { 255, 240, 785, 278 };
    const RECT rcBtnSetPin       = { 255, 296, 600, 338 };
    const RECT rcToggleAutoStart = { 255, 356, 785, 394 };

    // Page 2: Software Update Controls (macOS Style)
    const RECT rcBtnCheckUpdate  = { 255, 220, 435, 262 };
    const RECT rcBtnDownload     = { 255, 220, 445, 262 };
    const RECT rcBtnInstall      = { 255, 220, 465, 262 };
    const RECT rcRadioStable     = { 255, 335, 515, 410 };
    const RECT rcRadioBeta       = { 525, 335, 785, 410 };

    // Page 3: Audit Logs Controls
    const RECT rcBtnOpenLog      = { 255, 480, 520, 522 };

    // Page 4: About Controls
    const RECT rcBtnYABP         = { 255, 465, 420, 508 };
    const RECT rcBtnDev          = { 435, 465, 595, 508 };
    const RECT rcBtnGithub       = { 610, 465, 785, 508 };

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
        // Page 4 Hover (About Links)
        else if (g_activeTab == TAB_ABOUT) {
            if (PtInRectStruct(rcBtnYABP, x, y)) g_hoverControlId = ID_CANVAS_BTN_YABP;
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
                UpdateManager::GetInstance().CheckForUpdatesAsync(true);
                return 0;
            }
            if (PtInRectStruct(rcRadioStable, x, y)) {
                UpdateManager::GetInstance().SetChannel(L"stable");
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }
            if (PtInRectStruct(rcRadioBeta, x, y)) {
                UpdateManager::GetInstance().SetChannel(L"beta");
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
        // Page 4 Actions (About Links)
        else if (g_activeTab == TAB_ABOUT) {
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

            // Premium Modern Typography Stack with Segoe UI Variable fallback
            FontFamily fontFamDisplay(L"Segoe UI Variable Display");
            FontFamily fontFamText(L"Segoe UI Variable Text");
            FontFamily fontFamFallback(L"Segoe UI");

            const FontFamily* pDisplayFam = fontFamDisplay.IsAvailable() ? &fontFamDisplay : &fontFamFallback;
            const FontFamily* pTextFam = fontFamText.IsAvailable() ? &fontFamText : &fontFamFallback;

            Font brandFont(pDisplayFam, 12.5f, FontStyleBold, UnitPoint);
            Font pageHeadFont(pDisplayFam, 14.5f, FontStyleBold, UnitPoint);
            Font sectionFont(pDisplayFam, 11.0f, FontStyleBold, UnitPoint);
            Font bodyFont(pTextFam, 9.5f, FontStyleRegular, UnitPoint);

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
                L"App Locker", (g_activeTab == TAB_APPLOCKER), hovTab0, VectorIcon::Lock);

            bool hovTab1 = (g_hoverControlId == ID_CANVAS_TAB_SECURITY);
            UIComponents::DrawCanvasListItem(graphics, rcTab1.left, rcTab1.top, rcTab1.right - rcTab1.left, rcTab1.bottom - rcTab1.top,
                L"Security & Auth", (g_activeTab == TAB_SECURITY), hovTab1, VectorIcon::Shield);

            bool hovTab2 = (g_hoverControlId == ID_CANVAS_TAB_UPDATES);
            UIComponents::DrawCanvasListItem(graphics, rcTab2.left, rcTab2.top, rcTab2.right - rcTab2.left, rcTab2.bottom - rcTab2.top,
                L"Software Update", (g_activeTab == TAB_UPDATES), hovTab2, VectorIcon::Update);

            bool hovTab3 = (g_hoverControlId == ID_CANVAS_TAB_LOGS);
            UIComponents::DrawCanvasListItem(graphics, rcTab3.left, rcTab3.top, rcTab3.right - rcTab3.left, rcTab3.bottom - rcTab3.top,
                L"Audit Logs", (g_activeTab == TAB_LOGS), hovTab3, VectorIcon::Logs);

            bool hovTab4 = (g_hoverControlId == ID_CANVAS_TAB_ABOUT);
            UIComponents::DrawCanvasListItem(graphics, rcTab4.left, rcTab4.top, rcTab4.right - rcTab4.left, rcTab4.bottom - rcTab4.top,
                L"About Suraksha", (g_activeTab == TAB_ABOUT), hovTab4, VectorIcon::Info);

            // Top Status Badge
            const auto& settings = ConfigManager::GetInstance().GetSettings();
            bool protectionActive = settings.protectionEnabled;
            UIComponents::DrawStatusBadge(graphics, 675, 16, 135, 28, protectionActive ? L"Protected" : L"Paused", protectionActive);

            // ================= PAGE 0: APP LOCKER =================
            if (g_activeTab == TAB_APPLOCKER) {
                RectF headRect(235.0f, 16.0f, 400.0f, 28.0f);
                graphics.DrawString(L"Protected Applications", -1, &pageHeadFont, headRect, &formatLeft, &whiteBrush);

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
                    L"Add Application", ButtonVariant::Primary, hovAdd, prsAdd, VectorIcon::Plus);

                bool hovRem = (g_hoverControlId == ID_CANVAS_BTN_REMOVE);
                bool prsRem = (g_pressedControlId == ID_CANVAS_BTN_REMOVE);
                UIComponents::DrawCanvasButton(graphics, rcBtnRemove.left, rcBtnRemove.top, rcBtnRemove.right - rcBtnRemove.left, rcBtnRemove.bottom - rcBtnRemove.top,
                    L"Remove Application", ButtonVariant::Danger, hovRem, prsRem, VectorIcon::Trash);

                // Quick Presets
                bool hovP1 = (g_hoverControlId == ID_CANVAS_PRESET_NOTEPAD);
                bool prsP1 = (g_pressedControlId == ID_CANVAS_PRESET_NOTEPAD);
                UIComponents::DrawCanvasButton(graphics, rcPreset1.left, rcPreset1.top, rcPreset1.right - rcPreset1.left, rcPreset1.bottom - rcPreset1.top,
                    L"Notepad", ButtonVariant::Secondary, hovP1, prsP1, VectorIcon::Document);

                bool hovP2 = (g_hoverControlId == ID_CANVAS_PRESET_CHROME);
                bool prsP2 = (g_pressedControlId == ID_CANVAS_PRESET_CHROME);
                UIComponents::DrawCanvasButton(graphics, rcPreset2.left, rcPreset2.top, rcPreset2.right - rcPreset2.left, rcPreset2.bottom - rcPreset2.top,
                    L"Google Chrome", ButtonVariant::Secondary, hovP2, prsP2, VectorIcon::Globe);

                bool hovP3 = (g_hoverControlId == ID_CANVAS_PRESET_CMD);
                bool prsP3 = (g_pressedControlId == ID_CANVAS_PRESET_CMD);
                UIComponents::DrawCanvasButton(graphics, rcPreset3.left, rcPreset3.top, rcPreset3.right - rcPreset3.left, rcPreset3.bottom - rcPreset3.top,
                    L"Terminal", ButtonVariant::Secondary, hovP3, prsP3, VectorIcon::Terminal);

                bool hovP4 = (g_hoverControlId == ID_CANVAS_PRESET_CALC);
                bool prsP4 = (g_pressedControlId == ID_CANVAS_PRESET_CALC);
                UIComponents::DrawCanvasButton(graphics, rcPreset4.left, rcPreset4.top, rcPreset4.right - rcPreset4.left, rcPreset4.bottom - rcPreset4.top,
                    L"Calculator", ButtonVariant::Secondary, hovP4, prsP4, VectorIcon::Calculator);
            }
            // ================= PAGE 1: SECURITY & AUTH =================
            else if (g_activeTab == TAB_SECURITY) {
                RectF headRect(235.0f, 16.0f, 400.0f, 28.0f);
                graphics.DrawString(L"Security & Authentication Options", -1, &pageHeadFont, headRect, &formatLeft, &whiteBrush);

                UIComponents::DrawCanvasCard(graphics, 235, 60, 575, 460, Color(255, 26, 26, 30), Color(18, 255, 255, 255), 14);

                bool hovT1 = (g_hoverControlId == ID_CANVAS_TOGGLE_ENABLE);
                UIComponents::DrawCanvasToggle(graphics, rcToggleEnable.left, rcToggleEnable.top, rcToggleEnable.right - rcToggleEnable.left, rcToggleEnable.bottom - rcToggleEnable.top,
                    L"Enable Application Protection Engine", settings.protectionEnabled, hovT1);

                std::wstring winUserLabel = L"Allow Windows Password / PIN (" + SecurityManager::GetInstance().GetCurrentWindowsUsername() + L")";
                bool hovT2 = (g_hoverControlId == ID_CANVAS_TOGGLE_WINAUTH);
                UIComponents::DrawCanvasToggle(graphics, rcToggleWinAuth.left, rcToggleWinAuth.top, rcToggleWinAuth.right - rcToggleWinAuth.left, rcToggleWinAuth.bottom - rcToggleWinAuth.top,
                    winUserLabel.c_str(), settings.useWindowsAuth, hovT2);

                bool hovT3 = (g_hoverControlId == ID_CANVAS_TOGGLE_CUSTOMPIN);
                UIComponents::DrawCanvasToggle(graphics, rcToggleCustomPin.left, rcToggleCustomPin.top, rcToggleCustomPin.right - rcToggleCustomPin.left, rcToggleCustomPin.bottom - rcToggleCustomPin.top,
                    L"Allow Custom Master Passcode", settings.useCustomPin, hovT3);

                bool hovSetPin = (g_hoverControlId == ID_CANVAS_BTN_SETPIN);
                bool prsSetPin = (g_pressedControlId == ID_CANVAS_BTN_SETPIN);
                UIComponents::DrawCanvasButton(graphics, rcBtnSetPin.left, rcBtnSetPin.top, rcBtnSetPin.right - rcBtnSetPin.left, rcBtnSetPin.bottom - rcBtnSetPin.top,
                    L"Set Master Passcode...", ButtonVariant::Secondary, hovSetPin, prsSetPin, VectorIcon::Key);

                bool hovT4 = (g_hoverControlId == ID_CANVAS_TOGGLE_AUTOSTART);
                UIComponents::DrawCanvasToggle(graphics, rcToggleAutoStart.left, rcToggleAutoStart.top, rcToggleAutoStart.right - rcToggleAutoStart.left, rcToggleAutoStart.bottom - rcToggleAutoStart.top,
                    L"Launch automatically when Windows starts", settings.autoStartWithWindows, hovT4);
            }
            // ================= PAGE 2: SOFTWARE UPDATE (macOS Style) =================
            else if (g_activeTab == TAB_UPDATES) {
                RectF headRect(235.0f, 16.0f, 400.0f, 28.0f);
                graphics.DrawString(L"Software Update", -1, &pageHeadFont, headRect, &formatLeft, &whiteBrush);

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

                UIComponents::DrawCanvasCard(graphics, emblemX, emblemY, 52, 52, emblemBg, Color(30, 255, 255, 255), 14);
                if (emblemIcon == VectorIcon::Check) UIComponents::DrawIconCheck(graphics, emblemX + 16, emblemY + 16, 20, iconColor);
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
                std::wstring verLine = L"Installed: Version " + std::wstring(SURAKSHA_VERSION_STRING) + L" • Build " + std::wstring(SURAKSHA_BUILD_TAG);
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
                graphics.DrawString(L"Security Audit Trails", -1, &pageHeadFont, headRect, &formatLeft, &whiteBrush);

                UIComponents::DrawCanvasCard(graphics, 235, 60, 575, 400, Color(255, 26, 26, 30), Color(18, 255, 255, 255), 14);

                auto logLines = GetRecentAuditLogs(9);
                if (logLines.empty()) {
                    UIComponents::DrawEmptyState(graphics, 235, 60, 575, 400, L"No Audit Events Logged", L"All security operations will be tracked here in real-time");
                } else {
                    Font fontMono(pTextFam, 8.5f, FontStyleRegular, UnitPoint);
                    int logY = 75;
                    for (const auto& logEntry : logLines) {
                        RectF logRect(250.0f, (REAL)logY, 545.0f, 32.0f);
                        graphics.DrawString(logEntry.c_str(), -1, &fontMono, logRect, &formatLeft, &mutedBrush);
                        logY += 36;
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
                graphics.DrawString(L"About Suraksha", -1, &pageHeadFont, headRect, &formatLeft, &whiteBrush);

                UIComponents::DrawCanvasCard(graphics, 235, 60, 575, 460, Color(255, 26, 26, 30), Color(18, 255, 255, 255), 14);

                // App Branding Logo
                UIComponents::DrawAppLogo(graphics, 495, 80, 54);

                StringFormat formatCenter;
                formatCenter.SetAlignment(StringAlignmentCenter);
                formatCenter.SetLineAlignment(StringAlignmentCenter);

                RectF titleRc(250.0f, 142.0f, 545.0f, 26.0f);
                std::wstring aboutTitle = L"Suraksha - v" + std::wstring(SURAKSHA_DISPLAY_VERSION);
                graphics.DrawString(aboutTitle.c_str(), -1, &pageHeadFont, titleRc, &formatCenter, &whiteBrush);

                RectF yabpRc(250.0f, 172.0f, 545.0f, 22.0f);
                graphics.DrawString(L"A YABP Initiative (Yet Another Boring Project)", -1, &sectionFont, yabpRc, &formatCenter, &blueBrush);

                RectF devRc(250.0f, 200.0f, 545.0f, 20.0f);
                graphics.DrawString(L"Developed by Dheeraz", -1, &bodyFont, devRc, &formatCenter, &whiteBrush);

                // GPLv3 License Box
                UIComponents::DrawCanvasCard(graphics, 255, 235, 535, 210, Color(255, 34, 34, 38), Color(18, 255, 255, 255), 10);

                RectF licHeadRc(275.0f, 248.0f, 495.0f, 22.0f);
                graphics.DrawString(L"GNU General Public License v3.0 (GPLv3)", -1, &sectionFont, licHeadRc, &formatLeft, &whiteBrush);

                std::wstring fullLicDesc = L"This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3 of the License.\n\nThis program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.";
                RectF licBodyRc(275.0f, 278.0f, 495.0f, 140.0f);
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
