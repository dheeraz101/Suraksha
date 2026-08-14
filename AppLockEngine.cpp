#include "framework.h"
#include "AppLockEngine.h"
#include "ConfigManager.h"
#include "UnlockDialog.h"
#include "SecurityManager.h"
#include "AuditLogger.h"

#include <tlhelp32.h>
#include <psapi.h>
#include <algorithm>

#pragma comment(lib, "psapi.lib")

struct PromptGuard {
    std::atomic<bool>& flag;
    std::atomic<DWORD>& pidVal;
    PromptGuard(std::atomic<bool>& f, std::atomic<DWORD>& p, DWORD id) : flag(f), pidVal(p) {
        flag.store(true);
        pidVal.store(id);
    }
    ~PromptGuard() {
        pidVal.store(0);
        flag.store(false);
    }
};

AppLockEngine& AppLockEngine::GetInstance() {
    static AppLockEngine instance;
    return instance;
}

#include "SecurityManager.h"

AppLockEngine::AppLockEngine() {
    InitializeCriticalSectionAndSpinCount(&m_cs, 400);
    SecurityManager::EnableDebugPrivilege();
    SecurityManager::ProtectProcessDACL();
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (hNtdll) {
        m_pfnNtSuspendProcess = (pfnNtSuspendProcess)GetProcAddress(hNtdll, "NtSuspendProcess");
        m_pfnNtResumeProcess = (pfnNtResumeProcess)GetProcAddress(hNtdll, "NtResumeProcess");
    }
}

AppLockEngine::~AppLockEngine() {
    DeleteCriticalSection(&m_cs);
}



static BOOL CALLBACK EnumHideProc(HWND hwnd, LPARAM lParam) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == (DWORD)lParam && IsWindowVisible(hwnd)) {
        ShowWindow(hwnd, SW_HIDE);
        SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_HIDEWINDOW);
    }
    return TRUE;
}

void AppLockEngine::HideAllProcessWindows(DWORD pid) {
    EnumWindows(EnumHideProc, (LPARAM)pid);
}

bool AppLockEngine::IsCriticalSystemProcess(const std::wstring& exeName) {
    if (exeName.empty()) return true;

    std::wstring lower = exeName;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    size_t slash = lower.find_last_of(L"\\/");
    if (slash != std::wstring::npos) {
        lower = lower.substr(slash + 1);
    }

    static const std::set<std::wstring> criticalList = {
        L"suraksha.exe", L"explorer.exe", L"dwm.exe", L"csrss.exe", L"lsass.exe",
        L"services.exe", L"svchost.exe", L"winlogon.exe", L"smss.exe", L"system",
        L"taskhostw.exe", L"sihost.exe", L"ctfmon.exe", L"searchhost.exe",
        L"startmenuexperiencehost.exe", L"shellexperiencehost.exe",
        L"applicationframehost.exe", L"conhost.exe", L"fontdrvhost.exe",
        L"wininit.exe", L"logonui.exe", L"audiodg.exe", L"spoolsv.exe"
    };

    return criticalList.find(lower) != criticalList.end();
}

bool AppLockEngine::StartMonitoring(HWND hMainWnd) {
    m_hMainWnd = hMainWnd;
    if (!m_hEventHook) {
        m_hEventHook = SetWinEventHook(
            EVENT_SYSTEM_FOREGROUND,
            EVENT_SYSTEM_FOREGROUND,
            NULL,
            WinEventProc,
            0,
            0,
            WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
        );
    }
    return (m_hEventHook != NULL);
}

void AppLockEngine::StopMonitoring() {
    if (m_hEventHook) {
        UnhookWinEvent(m_hEventHook);
        m_hEventHook = NULL;
    }
}

VOID CALLBACK AppLockEngine::WinEventProc(
    HWINEVENTHOOK hWinEventHook,
    DWORD event,
    HWND hwnd,
    LONG idObject,
    LONG idChild,
    DWORD dwEventThread,
    DWORD dwmsEventTime)
{
    if (event == EVENT_SYSTEM_FOREGROUND && hwnd != NULL) {
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid != 0) {
            GetInstance().InterceptProcess(pid, hwnd);
        }
    }
}

void AppLockEngine::PeriodicCheck() {
    if (SecurityManager::CheckAntiDebugging()) {
        static bool s_alerted = false;
        if (!s_alerted) {
            s_alerted = true;
            AuditLogger::GetInstance().LogEvent(L"SECURITY_ALERT", L"Debugger or Memory Tampering attempt detected on Suraksha process! Lockdown enforced.");
            LockAllProcesses();
        }
    }

    const auto& settings = ConfigManager::GetInstance().GetSettings();
    if (!settings.protectionEnabled) return;

    if (settings.scheduleEnabled) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        if (st.wHour < settings.scheduleStartHour || st.wHour >= settings.scheduleEndHour) {
            return; // Paused during off-schedule hours
        }
    }

    if (m_isPromptShowing.load()) return;

    // Prune dead/exited PIDs from unlocked set
    {
        CSLock lock(m_cs);
        for (auto it = m_unlockedPIDs.begin(); it != m_unlockedPIDs.end(); ) {
            HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, *it);
            if (!hProc) {
                it = m_unlockedPIDs.erase(it);
            } else {
                DWORD exitCode = 0;
                if (GetExitCodeProcess(hProc, &exitCode) && exitCode != STILL_ACTIVE) {
                    it = m_unlockedPIDs.erase(it);
                } else {
                    ++it;
                }
                CloseHandle(hProc);
            }
        }
    }

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32W pe = { sizeof(pe) };
    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            if (pe.th32ProcessID != GetCurrentProcessId() && pe.th32ProcessID > 4) {
                if (!IsCriticalSystemProcess(pe.szExeFile)) {
                    bool shouldLock = settings.isAppLocked(pe.szExeFile);
                    if (!shouldLock && settings.lineageProtectionEnabled) {
                        DWORD ppid = GetParentPID(pe.th32ProcessID);
                        if (ppid > 4 && ppid != GetCurrentProcessId()) {
                            std::wstring parentName = GetProcessNameFromPID(ppid);
                            if (settings.isAppLocked(parentName)) {
                                shouldLock = true;
                            }
                        }
                    }
                    if (shouldLock) {
                        InterceptProcess(pe.th32ProcessID, NULL);
                    }
                }
            }
        } while (Process32NextW(hSnapshot, &pe));
    }
    CloseHandle(hSnapshot);
}

void AppLockEngine::InterceptProcess(DWORD pid, HWND hwndTarget) {
    if (pid == GetCurrentProcessId() || pid <= 4) return;

    const auto& settings = ConfigManager::GetInstance().GetSettings();
    if (!settings.protectionEnabled) return;

    if (settings.scheduleEnabled) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        if (st.wHour < settings.scheduleStartHour || st.wHour >= settings.scheduleEndHour) {
            return;
        }
    }

    if (m_isPromptShowing.load()) return;

    {
        CSLock lock(m_cs);
        if (m_unlockedPIDs.find(pid) != m_unlockedPIDs.end()) {
            return; // Already authorized
        }
    }

    std::wstring exeName = GetProcessNameFromPID(pid);
    std::wstring exePath = GetProcessPathFromPID(pid);

    if (IsCriticalSystemProcess(exeName) || IsCriticalSystemProcess(exePath)) return;

    bool shouldLock = settings.isAppLocked(exeName) || settings.isAppLocked(exePath);

    if (!shouldLock && settings.lineageProtectionEnabled) {
        DWORD ppid = GetParentPID(pid);
        if (ppid > 4 && ppid != GetCurrentProcessId()) {
            std::wstring parentName = GetProcessNameFromPID(ppid);
            std::wstring parentPath = GetProcessPathFromPID(ppid);
            if (settings.isAppLocked(parentName) || settings.isAppLocked(parentPath)) {
                shouldLock = true;
                AuditLogger::GetInstance().LogEvent(L"LINEAGE_PROTECTION", L"Child process (" + exeName + L") locked via Parent Process Lineage (" + parentName + L").");
            }
        }
    }

    if (shouldLock) {
        PromptGuard guard(m_isPromptShowing, m_promptingPID, pid);

        // 1. Suspend target process IMMEDIATELY!
        SuspendProcess(pid);

        // 2. Hide ALL windows of target process IMMEDIATELY!
        HideAllProcessWindows(pid);
        if (hwndTarget != NULL && IsWindow(hwndTarget)) {
            ShowWindow(hwndTarget, SW_HIDE);
        }

        std::wstring displayName = exeName.empty() ? L"Locked Application" : exeName;

        // 3. Display unlock prompt (topmost modal)
        bool authenticated = UnlockDialog::Show(m_hMainWnd, displayName, exePath);

        if (authenticated) {
            MarkProcessUnlocked(pid);
            ResumeProcess(pid);
            if (hwndTarget != NULL && IsWindow(hwndTarget)) {
                ShowWindow(hwndTarget, SW_SHOW);
                SetForegroundWindow(hwndTarget);
            }
        } else {
            TerminateProcessByID(pid);
        }
    }
}

bool AppLockEngine::IsProcessUnlocked(DWORD pid) {
    CSLock lock(m_cs);
    return m_unlockedPIDs.find(pid) != m_unlockedPIDs.end();
}

void AppLockEngine::MarkProcessUnlocked(DWORD pid) {
    CSLock lock(m_cs);
    m_unlockedPIDs.insert(pid);
}

void AppLockEngine::LockAllProcesses() {
    CSLock lock(m_cs);
    m_unlockedPIDs.clear();
}


std::wstring AppLockEngine::GetProcessNameFromPID(DWORD pid) {
    std::wstring name = L"";
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe = { sizeof(pe) };
        if (Process32FirstW(hSnapshot, &pe)) {
            do {
                if (pe.th32ProcessID == pid) {
                    name = pe.szExeFile;
                    break;
                }
            } while (Process32NextW(hSnapshot, &pe));
        }
        CloseHandle(hSnapshot);
    }
    return name;
}

std::wstring AppLockEngine::GetProcessPathFromPID(DWORD pid) {
    std::wstring path = L"";
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProcess) {
        hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    }
    if (hProcess) {
        wchar_t szBuffer[MAX_PATH] = { 0 };
        DWORD dwSize = MAX_PATH;
        if (QueryFullProcessImageNameW(hProcess, 0, szBuffer, &dwSize)) {
            path = szBuffer;
        } else {
            if (GetModuleFileNameExW(hProcess, NULL, szBuffer, MAX_PATH)) {
                path = szBuffer;
            } else if (GetProcessImageFileNameW(hProcess, szBuffer, MAX_PATH)) {
                path = szBuffer;
            }
        }
        CloseHandle(hProcess);
    }
    if (path.empty()) {
        path = GetProcessNameFromPID(pid);
    }
    return path;
}

DWORD AppLockEngine::GetParentPID(DWORD pid) {
    DWORD parentPid = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe = { sizeof(pe) };
        if (Process32FirstW(hSnap, &pe)) {
            do {
                if (pe.th32ProcessID == pid) {
                    parentPid = pe.th32ParentProcessID;
                    break;
                }
            } while (Process32NextW(hSnap, &pe));
        }
        CloseHandle(hSnap);
    }
    return parentPid;
}


bool AppLockEngine::SuspendProcess(DWORD pid) {
    auto pfn = GetInstance().m_pfnNtSuspendProcess;
    if (!pfn) return false;

    HANDLE hProcess = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, pid);
    if (hProcess) {
        pfn(hProcess);
        CloseHandle(hProcess);
        return true;
    }
    return false;
}

bool AppLockEngine::ResumeProcess(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, pid);
    if (hProcess) {
        if (GetInstance().m_pfnNtResumeProcess) {
            for (int i = 0; i < 5; ++i) {
                LONG status = GetInstance().m_pfnNtResumeProcess(hProcess);
                if (status != 0) break;
            }
        }
        CloseHandle(hProcess);
    }

    if (!GetInstance().m_pfnNtResumeProcess) {
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (hSnap != INVALID_HANDLE_VALUE) {
            THREADENTRY32 te = { sizeof(te) };
            if (Thread32First(hSnap, &te)) {
                do {
                    if (te.th32OwnerProcessID == pid) {
                        HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
                        if (hThread) {
                            for (int k = 0; k < 5; ++k) {
                                DWORD prevCount = ResumeThread(hThread);
                                if (prevCount <= 1 || prevCount == (DWORD)-1) break;
                            }
                            CloseHandle(hThread);
                        }
                    }
                } while (Thread32Next(hSnap, &te));
            }
            CloseHandle(hSnap);
        }
    }
    return true;
}

bool AppLockEngine::TerminateProcessByID(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess) {
        BOOL res = TerminateProcess(hProcess, 0);
        CloseHandle(hProcess);
        return (res != FALSE);
    }
    return false;
}

