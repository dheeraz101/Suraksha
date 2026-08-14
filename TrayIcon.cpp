#include "TrayIcon.h"

TrayIcon::TrayIcon() : m_isCreated(false) {
    ZeroMemory(&m_nid, sizeof(m_nid));
}

TrayIcon::~TrayIcon() {
    Remove();
}

bool TrayIcon::Create(HWND hWnd, UINT uID, HICON hIcon, const std::wstring& tooltip) {
    if (m_isCreated) return true;

    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = hWnd;
    m_nid.uID = uID;
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = WM_TRAYICON;
    m_nid.hIcon = hIcon;

    wcscpy_s(m_nid.szTip, tooltip.c_str());

    m_isCreated = Shell_NotifyIconW(NIM_ADD, &m_nid);
    return m_isCreated;
}

void TrayIcon::UpdateTooltip(HWND hWnd, const std::wstring& tooltip) {
    if (!m_isCreated) return;
    wcscpy_s(m_nid.szTip, tooltip.c_str());
    Shell_NotifyIconW(NIM_MODIFY, &m_nid);
}

bool TrayIcon::Remove() {
    if (!m_isCreated) return false;
    m_isCreated = false;
    return Shell_NotifyIconW(NIM_DELETE, &m_nid);
}

void TrayIcon::ShowContextMenu(HWND hWnd) {
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    AppendMenuW(hMenu, MF_STRING, ID_TRAY_OPEN, L"Open Suraksha Control Panel");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_TOGGLE, L"Pause / Resume Protection Engine");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_LOCKALL, L"Lock All Protected Sessions Now");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit Suraksha");

    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hWnd);

    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
    DestroyMenu(hMenu);
}
