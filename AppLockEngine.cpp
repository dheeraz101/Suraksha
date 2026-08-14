#include "AppLockEngine.h"
#include "ConfigManager.h"
#include "SecurityManager.h"
#include "UnlockDialog.h"
#include "AuditLogger.h"

#include <tlhelp32.h>
#include <psapi.h>
#include <algorithm>

#pragma comment(lib, "psapi.lib")

struct CSLock {
    CRITICAL_SECTION& cs;
    CSLock(CRITICAL_SECTION& c) : cs(c) { EnterCriticalSection(&cs); }
    ~CSLock() { LeaveCriticalSection(&cs); }
};

AppLockEngine::AppLockEngine() {
    InitializeCriticalSection(&m_cs);
    HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
    if (hNtDll) {
        m_pfnNtSuspendProcess = (pfnNtSuspendProcess)GetProcAddress(hNtDll, "NtSuspendProcess");
        m_pfnNtResumeProcess = (pfnNtResumeProcess)GetProcAddress(hNtDll, "NtResumeProcess");
    }
}

AppLockEngine::~AppLockEngine() {
    StopMonitoring();
    DeleteCriticalSection(&m_cs);
}

AppLockEngine& AppLockEngine::GetInstance() {
    static AppLockEngine instance;
    return instance;
}

bool AppLockEngine::IsCriticalSystemProcess(const std::wstring& exeName) {
    std::wstring lower = exeName;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
    static const std::set<std::wstring> criticalList = {
        L"system", L"registry", L"smss.exe", L"csrss.exe",
        L"wininit.exe", L"services.exe", L"lsass.exe",
        L"svchost.exe", L"fontdrvhost.exe", L"winlogon.exe",
        L"dwmp.exe", L"dwm.exe", L"sihost.exe",
        L"taskhostw.exe", L"ctfmon.exe", L"explorer.exe",
        L"suraksha.exe"
    };
    return criticalList.find(lower) != criticalList.end();
}

bool AppLockEngine::StartMonitoring(HWND hMainWnd) {
    m_hMainWnd = hMainWnd;

    if (!m_hEventHook) {
        m_hEventHook = SetWinEventHook(
            EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND,
            NULL, WinEventProc, 0, 0,
            WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
    }

    if (!m_hCreateHook) {
        m_hCreateHook = SetWinEventHook(
            EVENT_OBJECT_SHOW, EVENT_OBJECT_SHOW,
            NULL, WinEventProc, 0, 0,
            WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
    }

    if (!m_hScannerThread) {
        m_bScanning = true;
        m_hScannerThread = CreateThread(NULL, 0, FastScannerThreadProc, this, 0, NULL);
    }

    return (m_hEventHook != NULL || m_hScannerThread != NULL);
}

void AppLockEngine::StopMonitoring() {
    m_bScanning = false;
    if (m_hScannerThread) {
        WaitForSingleObject(m_hScannerThread, 2000);
        CloseHandle(m_hScannerThread);
        m_hScannerThread = NULL;
    }
    if (m_hEventHook) { UnhookWinEvent(m_hEventHook); m_hEventHook = NULL; }
    if (m_hCreateHook) { UnhookWinEvent(m_hCreateHook); m_hCreateHook = NULL; }
}

DWORD WINAPI AppLockEngine::FastScannerThreadProc(LPVOID lpParam) {
    AppLockEngine* pThis = (AppLockEngine*)lpParam;
    while (pThis->m_bScanning.load()) {
        pThis->ScanForLockedProcesses();
        Sleep(1000);
    }
    return 0;
}

VOID CALLBACK AppLockEngine::WinEventProc(
    HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd,
    LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    if (hwnd != NULL && (idObject == OBJID_WINDOW || idObject == 0)) {
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid != 0 && pid != GetCurrentProcessId()) {
            GetInstance().InterceptProcess(pid, hwnd);
        }
    }
}

void AppLockEngine::PeriodicCheck() {
    const auto& settings = ConfigManager::GetInstance().GetSettings();
    if (!settings.protectionEnabled) return;
    if (m_isPromptShowing.load()) return;

    HWND fg = GetForegroundWindow();
    if (!fg) return;
    DWORD pid = 0;
    GetWindowThreadProcessId(fg, &pid);
    if (pid != 0 && pid != GetCurrentProcessId() && pid > 4) {
        InterceptProcess(pid, fg);
    }
}

void AppLockEngine::ScanForLockedProcesses() {
    const auto& settings = ConfigManager::GetInstance().GetSettings();
    if (!settings.protectionEnabled) return;
    if (m_isPromptShowing.load()) return;

    if (settings.scheduleEnabled) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        if (st.wHour < settings.scheduleStartHour || st.wHour >= settings.scheduleEndHour) return;
    }

    // Prune exited PIDs
    {
        CSLock lock(m_cs);
        for (auto it = m_unlockedPIDs.begin(); it != m_unlockedPIDs.end(); ) {
            HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, *it);
            if (!hProc) { it = m_unlockedPIDs.erase(it); continue; }
            DWORD exitCode = 0;
            if (GetExitCodeProcess(hProc, &exitCode) && exitCode != STILL_ACTIVE)
                it = m_unlockedPIDs.erase(it);
            else
                ++it;
            CloseHandle(hProc);
        }
    }

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32W pe = { sizeof(pe) };
    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            if (pe.th32ProcessID != GetCurrentProcessId() && pe.th32ProcessID > 4) {
                if (!IsCriticalSystemProcess(pe.szExeFile)) {
                    if (settings.isAppLocked(pe.szExeFile)) {
                        if (!IsProcessUnlocked(pe.th32ProcessID)) {
                            CloseHandle(hSnapshot);
                            InterceptProcess(pe.th32ProcessID, NULL);
                            return;
                        }
                    }
                }
            }
        } while (Process32NextW(hSnapshot, &pe));
    }
    CloseHandle(hSnapshot);
}

struct HideContext { DWORD targetPid; };

// Non-blocking hide: SetWindowPos with SWP_NOSENDCHANGING never waits for the
// target thread to process messages. It modifies the window position directly
// in the kernel, avoiding the cross-process SendMessage that causes hangs.
static BOOL CALLBACK NonBlockingHideProc(HWND hwnd, LPARAM lParam) {
    HideContext* ctx = (HideContext*)lParam;
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == ctx->targetPid) {
        SetWindowPos(hwnd, HWND_BOTTOM, -32000, -32000, 0, 0,
            SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING | SWP_ASYNCWINDOWPOS);
    }
    return TRUE;
}

static BOOL CALLBACK RestoreEnumWindowsProc(HWND hwnd, LPARAM lParam) {
    HideContext* ctx = (HideContext*)lParam;
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == ctx->targetPid && IsWindow(hwnd)) {
        SetWindowPos(hwnd, HWND_TOP, 100, 100, 0, 0,
            SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOSENDCHANGING);
        ShowWindowAsync(hwnd, SW_RESTORE);
        SetForegroundWindow(hwnd);
    }
    return TRUE;
}

void AppLockEngine::HideAllProcessWindows(DWORD pid) {
    HideContext ctx = { pid };
    EnumWindows(NonBlockingHideProc, (LPARAM)&ctx);
}

void AppLockEngine::RestoreAllProcessWindows(DWORD pid) {
    HideContext ctx = { pid };
    EnumWindows(RestoreEnumWindowsProc, (LPARAM)&ctx);
}

struct PromptWorkerData {
    DWORD pid;
    HWND hMainWnd;
    std::wstring appName;
    std::wstring appPath;
};

static DWORD WINAPI PromptWorkerThread(LPVOID lpParam) {
    PromptWorkerData* pData = (PromptWorkerData*)lpParam;
    DWORD pid = pData->pid;
    std::wstring appName = pData->appName;
    std::wstring appPath = pData->appPath;
    delete pData;

    // ALL heavy work happens here on the worker thread, never on the UI thread.

    // Step 1: Move all windows off-screen instantly (non-blocking)
    HideContext ctx = { pid };
    EnumWindows(NonBlockingHideProc, (LPARAM)&ctx);

    // Step 2: Suspend the process
    AppLockEngine::SuspendProcess(pid);

    // Step 3: Move again (belt & suspenders for windows created between step 1-2)
    EnumWindows(NonBlockingHideProc, (LPARAM)&ctx);

    // Step 4: Show unlock dialog
    bool authSuccess = UnlockDialog::Show(NULL, appName, appPath);

    if (authSuccess) {
        AppLockEngine::GetInstance().MarkProcessUnlocked(pid);
        AppLockEngine::ResumeProcess(pid);
        Sleep(80);
        AppLockEngine::RestoreAllProcessWindows(pid);
        AuditLogger::GetInstance().LogEvent(L"APP_UNLOCKED", L"User unlocked " + appName);
    } else {
        AppLockEngine::TerminateProcessByID(pid);
        AuditLogger::GetInstance().LogEvent(L"APP_TERMINATED", L"Auth failed for " + appName);
    }

    AppLockEngine::GetInstance().m_promptingPID.store(0);
    AppLockEngine::GetInstance().m_isPromptShowing.store(false);
    return 0;
}

void AppLockEngine::InterceptProcess(DWORD pid, HWND hwndTarget) {
    // This function is called from WinEventProc and WM_TIMER on the UI thread.
    // It MUST return instantly. Zero cross-process calls allowed here.

    if (pid == GetCurrentProcessId() || pid <= 4) return;
    if (!ConfigManager::GetInstance().GetSettings().protectionEnabled) return;
    if (IsProcessUnlocked(pid)) return;
    if (m_isPromptShowing.load()) return;

    std::wstring exeName = GetProcessNameFromPID(pid);
    if (exeName.empty() || IsCriticalSystemProcess(exeName)) return;
    if (!ConfigManager::GetInstance().GetSettings().isAppLocked(exeName)) return;

    // Atomically claim the prompt slot
    bool expected = false;
    if (!m_isPromptShowing.compare_exchange_strong(expected, true)) return;
    m_promptingPID.store(pid);

    // Launch worker thread IMMEDIATELY. All hiding/suspending/prompting
    // happens there. This function returns in microseconds.
    std::wstring appPath = GetProcessPathFromPID(pid);
    PromptWorkerData* pData = new PromptWorkerData{ pid, m_hMainWnd, exeName, appPath };
    HANDLE hThread = CreateThread(NULL, 0, PromptWorkerThread, pData, 0, NULL);
    if (hThread) {
        CloseHandle(hThread);
    } else {
        delete pData;
        m_promptingPID.store(0);
        m_isPromptShowing.store(false);
    }
}

bool AppLockEngine::IsProcessUnlocked(DWORD pid) {
    CSLock lock(m_cs);
    return (m_unlockedPIDs.find(pid) != m_unlockedPIDs.end());
}

void AppLockEngine::MarkProcessUnlocked(DWORD pid) {
    CSLock lock(m_cs);
    m_unlockedPIDs.insert(pid);
}

void AppLockEngine::LockAllProcesses() {
    std::vector<DWORD> pidsToLock;
    {
        CSLock lock(m_cs);
        for (DWORD pid : m_unlockedPIDs) pidsToLock.push_back(pid);
        m_unlockedPIDs.clear();
    }
    for (DWORD pid : pidsToLock) {
        HideAllProcessWindows(pid);
        SuspendProcess(pid);
    }
}

bool AppLockEngine::SuspendProcess(DWORD pid) {
    if (pid <= 4 || pid == GetCurrentProcessId()) return false;
    if (!GetInstance().m_pfnNtSuspendProcess) return false;
    HANDLE hProc = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, pid);
    if (!hProc) return false;
    LONG status = GetInstance().m_pfnNtSuspendProcess(hProc);
    CloseHandle(hProc);
    return (status >= 0);
}

bool AppLockEngine::ResumeProcess(DWORD pid) {
    if (pid <= 4 || pid == GetCurrentProcessId()) return false;
    if (!GetInstance().m_pfnNtResumeProcess) return false;
    HANDLE hProc = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, pid);
    if (!hProc) return false;
    LONG status = GetInstance().m_pfnNtResumeProcess(hProc);
    CloseHandle(hProc);
    return (status >= 0);
}

bool AppLockEngine::TerminateProcessByID(DWORD pid) {
    if (pid <= 4 || pid == GetCurrentProcessId()) return false;
    HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!hProc) return false;
    BOOL ok = TerminateProcess(hProc, 1);
    CloseHandle(hProc);
    return (ok == TRUE);
}

std::wstring AppLockEngine::GetProcessNameFromPID(DWORD pid) {
    if (pid <= 4) return L"";
    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProc) return L"";
    wchar_t szPath[MAX_PATH] = { 0 };
    DWORD dwSize = MAX_PATH;
    std::wstring result;
    if (QueryFullProcessImageNameW(hProc, 0, szPath, &dwSize)) {
        wchar_t* fn = wcsrchr(szPath, L'\\');
        result = fn ? (fn + 1) : szPath;
    }
    CloseHandle(hProc);
    return result;
}

std::wstring AppLockEngine::GetProcessPathFromPID(DWORD pid) {
    if (pid <= 4) return L"";
    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProc) return L"";
    wchar_t szPath[MAX_PATH] = { 0 };
    DWORD dwSize = MAX_PATH;
    std::wstring result;
    if (QueryFullProcessImageNameW(hProc, 0, szPath, &dwSize)) result = szPath;
    CloseHandle(hProc);
    return result;
}

DWORD AppLockEngine::GetParentPID(DWORD pid) {
    if (pid <= 4) return 0;
    DWORD parentPID = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe = { sizeof(pe) };
        if (Process32FirstW(hSnapshot, &pe)) {
            do {
                if (pe.th32ProcessID == pid) { parentPID = pe.th32ParentProcessID; break; }
            } while (Process32NextW(hSnapshot, &pe));
        }
        CloseHandle(hSnapshot);
    }
    return parentPID;
}
