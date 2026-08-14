#include "SecurityManager.h"
#include "ConfigManager.h"
#include "AuditLogger.h"
#include <wincred.h>
#include <wincrypt.h>
#include <lmcons.h>
#include <cstdio>

#pragma comment(lib, "credui.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "advapi32.lib")

SecurityManager::SecurityManager() : m_failedAttempts(0) {
}

bool SecurityManager::IsLockedOut(int& remainingSeconds) {
    if (m_failedAttempts >= 3) {
        auto now = std::chrono::steady_clock::now();
        if (now < m_lockoutEndTime) {
            remainingSeconds = (int)std::chrono::duration_cast<std::chrono::seconds>(m_lockoutEndTime - now).count() + 1;
            return true;
        } else {
            // Lockout expired
            m_failedAttempts = 0;
        }
    }
    remainingSeconds = 0;
    return false;
}

void SecurityManager::RecordFailedAttempt() {
    m_failedAttempts++;
    AuditLogger::GetInstance().LogEvent(L"SECURITY_WARNING", L"Failed passcode unlock attempt recorded. Total consecutive failures: " + std::to_wstring(m_failedAttempts));
    if (m_failedAttempts >= 3) {
        m_lockoutEndTime = std::chrono::steady_clock::now() + std::chrono::seconds(30);
        AuditLogger::GetInstance().LogEvent(L"SECURITY_LOCKOUT", L"3 consecutive failed attempts. System locked out for 30 seconds.");
    }
}

void SecurityManager::ResetFailedAttempts() {
    m_failedAttempts = 0;
}

std::wstring SecurityManager::GetCurrentWindowsUsername() {
    wchar_t username[UNLEN + 1] = { 0 };
    DWORD size = UNLEN + 1;
    if (GetUserNameW(username, &size)) {
        return std::wstring(username);
    }
    return L"User";
}

std::wstring SecurityManager::HashSHA256(const std::wstring& input, const std::wstring& salt) {
    std::wstring combined = salt + L":" + input;
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    std::wstring result = L"";

    if (CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        if (CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
            DWORD bytesLen = (DWORD)(combined.length() * sizeof(wchar_t));
            if (CryptHashData(hHash, (BYTE*)combined.c_str(), bytesLen, 0)) {
                DWORD hashLen = 32;
                BYTE hashBuf[32] = { 0 };
                if (CryptGetHashParam(hHash, HP_HASHVAL, hashBuf, &hashLen, 0)) {
                    wchar_t hexStr[65] = { 0 };
                    for (DWORD i = 0; i < hashLen; ++i) {
                        swprintf_s(&hexStr[i * 2], 3, L"%02x", hashBuf[i]);
                    }
                    result = hexStr;
                }
            }
            CryptDestroyHash(hHash);
        }
        CryptReleaseContext(hProv, 0);
    }
    return result;
}

std::wstring SecurityManager::GenerateSalt() {
    HCRYPTPROV hProv = 0;
    BYTE saltBytes[16] = { 0 };
    std::wstring result = L"1234567890abcdef";
    if (CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        if (CryptGenRandom(hProv, sizeof(saltBytes), saltBytes)) {
            wchar_t hexStr[33] = { 0 };
            for (int i = 0; i < 16; ++i) {
                swprintf_s(&hexStr[i * 2], 3, L"%02x", saltBytes[i]);
            }
            result = hexStr;
        }
        CryptReleaseContext(hProv, 0);
    }
    return result;
}

bool SecurityManager::EnableDebugPrivilege() {
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        LUID luid;
        if (LookupPrivilegeValueW(NULL, SE_DEBUG_NAME, &luid)) {
            TOKEN_PRIVILEGES tp;
            tp.PrivilegeCount = 1;
            tp.Privileges[0].Luid = luid;
            tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
        }
        CloseHandle(hToken);
        return true;
    }
    return false;
}

typedef BOOL (WINAPI *pfnCredUnpackAuthenticationBufferW)(
    DWORD dwFlags,
    PVOID pAuthBuffer,
    DWORD cbAuthBuffer,
    LPWSTR pszUserName,
    DWORD *pcchMaxUserName,
    LPWSTR pszDomainName,
    DWORD *pcchMaxDomainName,
    LPWSTR pszPassword,
    DWORD *pcchMaxPassword
);

static BOOL SafeCredUnpackAuthenticationBufferW(
    DWORD dwFlags,
    PVOID pAuthBuffer,
    DWORD cbAuthBuffer,
    LPWSTR pszUserName,
    DWORD *pcchMaxUserName,
    LPWSTR pszDomainName,
    DWORD *pcchMaxDomainName,
    LPWSTR pszPassword,
    DWORD *pcchMaxPassword)
{
    HMODULE hCredui = GetModuleHandleW(L"credui.dll");
    if (!hCredui) {
        hCredui = LoadLibraryW(L"credui.dll");
    }
    if (hCredui) {
        auto pfn = (pfnCredUnpackAuthenticationBufferW)GetProcAddress(hCredui, "CredUnpackAuthenticationBufferW");
        if (pfn) {
            return pfn(dwFlags, pAuthBuffer, cbAuthBuffer, pszUserName, pcchMaxUserName, pszDomainName, pcchMaxDomainName, pszPassword, pcchMaxPassword);
        }
    }
    return FALSE;
}

bool SecurityManager::VerifyWindowsCredentials(HWND hWndOwner, std::wstring& outError) {
    int remSec = 0;
    if (IsLockedOut(remSec)) {
        outError = L"Security Lockout! Too many failed attempts. Try again in " + std::to_wstring(remSec) + L"s.";
        return false;
    }

    std::wstring username = GetCurrentWindowsUsername();

    CREDUI_INFOW cui = { sizeof(CREDUI_INFOW) };
    cui.hwndParent = hWndOwner;
    cui.pszMessageText = L"Enter your Windows Password or PIN to authorize access.";
    cui.pszCaptionText = L"Suraksha Application Authorization";

    ULONG authPackage = 0;
    LPVOID outAuthBuffer = NULL;
    ULONG outAuthBufferSize = 0;
    BOOL save = FALSE;

    DWORD dwErr = CredUIPromptForWindowsCredentialsW(
        &cui,
        0,
        &authPackage,
        NULL, 0,
        &outAuthBuffer,
        &outAuthBufferSize,
        &save,
        CREDUIWIN_GENERIC | CREDUIWIN_IN_CRED_ONLY
    );

    if (dwErr == ERROR_SUCCESS) {
        wchar_t szUserName[CREDUI_MAX_USERNAME_LENGTH + 1] = { 0 };
        DWORD cchUserName = CREDUI_MAX_USERNAME_LENGTH + 1;
        wchar_t szDomainName[CREDUI_MAX_DOMAIN_TARGET_LENGTH + 1] = { 0 };
        DWORD cchDomainName = CREDUI_MAX_DOMAIN_TARGET_LENGTH + 1;
        wchar_t szPassword[CREDUI_MAX_PASSWORD_LENGTH + 1] = { 0 };
        DWORD cchPassword = CREDUI_MAX_PASSWORD_LENGTH + 1;

        BOOL unpacked = SafeCredUnpackAuthenticationBufferW(
            0,
            outAuthBuffer,
            outAuthBufferSize,
            szUserName,
            &cchUserName,
            szDomainName,
            &cchDomainName,
            szPassword,
            &cchPassword
        );

        CoTaskMemFree(outAuthBuffer);


        if (unpacked) {
            HANDLE hToken = NULL;
            BOOL logonOk = LogonUserW(
                szUserName,
                szDomainName[0] ? szDomainName : NULL,
                szPassword,
                LOGON32_LOGON_INTERACTIVE,
                LOGON32_PROVIDER_DEFAULT,
                &hToken
            );

            if (!logonOk) {
                logonOk = LogonUserW(
                    szUserName,
                    szDomainName[0] ? szDomainName : NULL,
                    szPassword,
                    LOGON32_LOGON_NETWORK_CLEARTEXT,
                    LOGON32_PROVIDER_DEFAULT,
                    &hToken
                );
            }

            SecureZeroMemory(szPassword, sizeof(szPassword));

            if (logonOk) {
                if (hToken) CloseHandle(hToken);
                ResetFailedAttempts();
                AuditLogger::GetInstance().LogEvent(L"AUTHENTICATION_SUCCESS", L"Windows authentication successful for user: " + std::wstring(szUserName));
                return true;
            }
        } else {
            SecureZeroMemory(szPassword, sizeof(szPassword));
        }

        RecordFailedAttempt();
        outError = L"Invalid Windows password or PIN! Please try again.";
        return false;
    } else if (dwErr == ERROR_CANCELLED) {
        outError = L"Authentication cancelled by user.";
        return false;
    } else {
        RecordFailedAttempt();
        outError = L"Invalid Windows password or PIN! Please try again.";
        return false;
    }
}

#include <aclapi.h>
#include <sddl.h>
#include <bcrypt.h>
#include <vector>

#pragma comment(lib, "bcrypt.lib")

std::wstring SecurityManager::PBKDF2Hash(const std::wstring& password, const std::wstring& salt, int iterations) {
    BCRYPT_ALG_HANDLE hAlg = NULL;
    std::wstring result = L"";

    if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG) == 0) {
        std::wstring passStr = password;
        std::vector<BYTE> passBytes(passStr.length() * sizeof(wchar_t));
        memcpy(passBytes.data(), passStr.c_str(), passBytes.size());

        std::wstring saltStr = salt;
        std::vector<BYTE> saltBytes(saltStr.length() * sizeof(wchar_t));
        memcpy(saltBytes.data(), saltStr.c_str(), saltBytes.size());

        BYTE derivedKey[32] = { 0 };
        std::vector<BYTE> uBlock(saltBytes.size() + 4);
        memcpy(uBlock.data(), saltBytes.data(), saltBytes.size());
        uBlock[saltBytes.size() + 0] = 0;
        uBlock[saltBytes.size() + 1] = 0;
        uBlock[saltBytes.size() + 2] = 0;
        uBlock[saltBytes.size() + 3] = 1;

        BYTE uPrev[32] = { 0 };
        BCRYPT_HASH_HANDLE hHash = NULL;
        if (BCryptCreateHash(hAlg, &hHash, NULL, 0, passBytes.data(), (ULONG)passBytes.size(), 0) == 0) {
            BCryptHashData(hHash, uBlock.data(), (ULONG)uBlock.size(), 0);
            BCryptFinishHash(hHash, uPrev, 32, 0);
            BCryptDestroyHash(hHash);
            memcpy(derivedKey, uPrev, 32);

            for (int i = 2; i <= iterations; ++i) {
                if (BCryptCreateHash(hAlg, &hHash, NULL, 0, passBytes.data(), (ULONG)passBytes.size(), 0) == 0) {
                    BCryptHashData(hHash, uPrev, 32, 0);
                    BCryptFinishHash(hHash, uPrev, 32, 0);
                    BCryptDestroyHash(hHash);
                    for (int k = 0; k < 32; ++k) {
                        derivedKey[k] ^= uPrev[k];
                    }
                }
            }

            wchar_t hexStr[65] = { 0 };
            for (int i = 0; i < 32; ++i) {
                swprintf_s(&hexStr[i * 2], 3, L"%02x", derivedKey[i]);
            }
            result = hexStr;
        }
        BCryptCloseAlgorithmProvider(hAlg, 0);
    }

    if (result.empty()) {
        result = HashSHA256(password, salt);
    }
    return result;
}

bool SecurityManager::ProtectProcessDACL() {
#ifdef _DEBUG
    if (IsDebuggerPresent()) {
        return true; // Skip DACL process shield in Debug mode under VS Debugger
    }
#endif
    PSECURITY_DESCRIPTOR pSD = NULL;
    // SDDL: Deny Terminate (0x1), Suspend/Resume (0x800), Write (0x20) to Everyone (WD)
    if (ConvertStringSecurityDescriptorToSecurityDescriptorW(
        L"D:(D;;0x0821;;;WD)(A;;GA;;;SY)(A;;GA;;;BA)(A;;GA;;;CO)",
        SDDL_REVISION_1, &pSD, NULL))
    {
        PACL pDacl = NULL;
        BOOL bDaclPresent = FALSE;
        BOOL bDaclDefaulted = FALSE;

        if (GetSecurityDescriptorDacl(pSD, &bDaclPresent, &pDacl, &bDaclDefaulted)) {
            SetSecurityInfo(
                GetCurrentProcess(),
                SE_KERNEL_OBJECT,
                DACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION,
                NULL, NULL, pDacl, NULL
            );
        }
        LocalFree(pSD);
        return true;
    }
    return false;
}

bool SecurityManager::CheckAntiDebugging() {
#ifdef _DEBUG
    return false; // Disable anti-debugging check in Debug build for VS Debugger
#endif
    if (IsDebuggerPresent()) return true;

    BOOL isRemoteDebugger = FALSE;
    if (CheckRemoteDebuggerPresent(GetCurrentProcess(), &isRemoteDebugger) && isRemoteDebugger) {
        return true;
    }

    typedef NTSTATUS(NTAPI* pfnNtQueryInformationProcess)(
        HANDLE ProcessHandle,
        ULONG ProcessInformationClass,
        PVOID ProcessInformation,
        ULONG ProcessInformationLength,
        PULONG ReturnLength
    );

    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (hNtdll) {
        auto pfnNtQuery = (pfnNtQueryInformationProcess)GetProcAddress(hNtdll, "NtQueryInformationProcess");
        if (pfnNtQuery) {
            DWORD debugPort = 0;
            if (pfnNtQuery(GetCurrentProcess(), 7 /* ProcessDebugPort */, &debugPort, sizeof(debugPort), NULL) == 0) {
                if (debugPort != 0) return true;
            }
        }
    }
    return false;
}


bool SecurityManager::VerifyCustomPin(const std::wstring& inputPin) {
    int remSec = 0;
    if (IsLockedOut(remSec)) {
        return false;
    }

    auto& settings = ConfigManager::GetInstance().GetSettings();
    if (settings.customPinHash.empty()) return false;

    std::wstring pbkdf2Input = PBKDF2Hash(inputPin, settings.customPinSalt, 100000);
    if (!settings.customPinSalt.empty() && pbkdf2Input == settings.customPinHash) {
        ResetFailedAttempts();
        AuditLogger::GetInstance().LogEvent(L"AUTHENTICATION_SUCCESS", L"Master passcode verified successfully via PBKDF2 (100k iterations).");
        return true;
    }

    std::wstring legacyHash = HashSHA256(inputPin, settings.customPinSalt);
    if (legacyHash == settings.customPinHash || inputPin == settings.customPinHash) {
        SetCustomPin(inputPin);
        ResetFailedAttempts();
        AuditLogger::GetInstance().LogEvent(L"AUTHENTICATION_SUCCESS", L"Master passcode verified & upgraded to Military PBKDF2-HMAC-SHA256 (100k iterations).");
        return true;
    }

    RecordFailedAttempt();
    return false;
}

void SecurityManager::SetCustomPin(const std::wstring& pin) {
    auto& settings = ConfigManager::GetInstance().GetSettings();
    std::wstring salt = GenerateSalt();
    settings.customPinSalt = salt;
    settings.customPinHash = PBKDF2Hash(pin, salt, 100000);
    settings.useCustomPin = true;
    ConfigManager::GetInstance().SaveSettings();
    AuditLogger::GetInstance().LogEvent(L"SECURITY_CHANGE", L"Master passcode updated with Military PBKDF2 key derivation.");
}

bool SecurityManager::HasCustomPin() {
    const auto& settings = ConfigManager::GetInstance().GetSettings();
    return !settings.customPinHash.empty();
}


