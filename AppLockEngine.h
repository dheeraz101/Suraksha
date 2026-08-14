#pragma once

#include <windows.h>
#include <string>
#include <set>
#include <vector>
#include <mutex>
#include <atomic>

typedef LONG(NTAPI* pfnNtSuspendProcess)(HANDLE ProcessHandle);
typedef LONG(NTAPI* pfnNtResumeProcess)(HANDLE ProcessHandle);

class AppLockEngine {
public:
    static AppLockEngine& GetInstance();

    bool StartMonitoring(HWND hMainWnd);
    void StopMonitoring();
    void PeriodicCheck();

    void InterceptProcess(DWORD pid, HWND hwndTarget = NULL);
    bool IsProcessUnlocked(DWORD pid);
    void MarkProcessUnlocked(DWORD pid);
    void LockAllProcesses();

    static void HideAllProcessWindows(DWORD pid);
    static bool IsCriticalSystemProcess(const std::wstring& exeName);
    static std::wstring GetProcessNameFromPID(DWORD pid);
    static std::wstring GetProcessPathFromPID(DWORD pid);
    static DWORD GetParentPID(DWORD pid);
    static bool SuspendProcess(DWORD pid);

    static bool ResumeProcess(DWORD pid);
    static bool TerminateProcessByID(DWORD pid);

private:
    AppLockEngine();
    ~AppLockEngine();

    static VOID CALLBACK WinEventProc(
        HWINEVENTHOOK hWinEventHook,
        DWORD event,
        HWND hwnd,
        LONG idObject,
        LONG idChild,
        DWORD dwEventThread,
        DWORD dwmsEventTime
    );

    HWND m_hMainWnd = NULL;
    HWINEVENTHOOK m_hEventHook = NULL;
    std::set<DWORD> m_unlockedPIDs;
    CRITICAL_SECTION m_cs;

    std::atomic<bool> m_isPromptShowing{ false };

    std::atomic<DWORD> m_promptingPID{ 0 };

    pfnNtSuspendProcess m_pfnNtSuspendProcess = nullptr;
    pfnNtResumeProcess m_pfnNtResumeProcess = nullptr;
};
