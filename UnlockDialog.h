#pragma once

#include <windows.h>
#include <string>

class UnlockDialog {
public:
    static bool Show(HWND hParent, const std::wstring& appName, const std::wstring& appPath);

private:
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};
