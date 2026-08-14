#pragma once

#include <windows.h>
#include <shellapi.h>
#include <string>

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_OPEN 4001
#define ID_TRAY_TOGGLE 4002
#define ID_TRAY_LOCKALL 4003
#define ID_TRAY_EXIT 4004

class TrayIcon {
public:
    static TrayIcon& GetInstance() {
        static TrayIcon instance;
        return instance;
    }

    bool Create(HWND hWnd, UINT uID, HICON hIcon, const std::wstring& tooltip);
    bool Remove();
    void ShowContextMenu(HWND hWnd);
    void UpdateTooltip(HWND hWnd, const std::wstring& tooltip);

private:
    TrayIcon();
    ~TrayIcon();

    NOTIFYICONDATAW m_nid;
    bool m_isCreated;
};
