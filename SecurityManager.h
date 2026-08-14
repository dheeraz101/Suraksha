#pragma once

#include "targetver.h"
#include <windows.h>
#include <string>
#include <chrono>


class SecurityManager {
public:
    static SecurityManager& GetInstance() {
        static SecurityManager instance;
        return instance;
    }

    bool VerifyWindowsCredentials(HWND hWndOwner, std::wstring& outError);
    bool VerifyCustomPin(const std::wstring& inputPin);
    void SetCustomPin(const std::wstring& pin);
    bool HasCustomPin();

    bool IsLockedOut(int& remainingSeconds);
    void RecordFailedAttempt();
    void ResetFailedAttempts();

    std::wstring GetCurrentWindowsUsername();

    static std::wstring HashSHA256(const std::wstring& input, const std::wstring& salt);
    static std::wstring PBKDF2Hash(const std::wstring& password, const std::wstring& salt, int iterations = 100000);
    static std::wstring GenerateSalt();
    static bool EnableDebugPrivilege();
    static bool ProtectProcessDACL();
    static bool CheckAntiDebugging();


private:
    SecurityManager();
    ~SecurityManager() = default;

    int m_failedAttempts;
    std::chrono::steady_clock::time_point m_lockoutEndTime;
};
