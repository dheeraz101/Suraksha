#include "ConfigManager.h"
#include <shlobj.h>
#include <wincrypt.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdio>

#pragma comment(lib, "crypt32.lib")

ConfigManager& ConfigManager::GetInstance() {
    static ConfigManager instance;
    return instance;
}

std::wstring ConfigManager::GetConfigFilePath() {
    wchar_t appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        std::wstring dir = std::wstring(appDataPath) + L"\\Suraksha";
        CreateDirectoryW(dir.c_str(), NULL);
        return dir + L"\\config.dat";
    }
    return L"config.dat";
}

static std::wstring BytesToHex(const BYTE* data, DWORD len) {
    std::wstring hexStr;
    hexStr.reserve(len * 2);
    wchar_t buf[3];
    for (DWORD i = 0; i < len; ++i) {
        swprintf_s(buf, 3, L"%02x", data[i]);
        hexStr.append(buf);
    }
    return hexStr;
}

static std::vector<BYTE> HexToBytes(const std::wstring& hexStr) {
    std::vector<BYTE> bytes;
    bytes.reserve(hexStr.length() / 2);
    for (size_t i = 0; i + 1 < hexStr.length(); i += 2) {
        wchar_t byteString[3] = { hexStr[i], hexStr[i + 1], L'\0' };
        BYTE b = (BYTE)wcstoul(byteString, NULL, 16);
        bytes.push_back(b);
    }
    return bytes;
}

static void ParseSettingsStream(std::wistream& stream, AppSettings& settings) {
    settings.lockedApps.clear();
    std::wstring line;
    while (std::getline(stream, line)) {
        if (line.empty() || line[0] == L'#') continue;

        size_t eqPos = line.find(L'=');
        if (eqPos != std::wstring::npos) {
            std::wstring key = line.substr(0, eqPos);
            std::wstring val = line.substr(eqPos + 1);

            if (key == L"customPinHash") settings.customPinHash = val;
            else if (key == L"customPinSalt") settings.customPinSalt = val;
            else if (key == L"useWindowsAuth") settings.useWindowsAuth = (val == L"1");
            else if (key == L"useCustomPin") settings.useCustomPin = (val == L"1");
            else if (key == L"protectionEnabled") settings.protectionEnabled = (val == L"1");
            else if (key == L"autoStartWithWindows") settings.autoStartWithWindows = (val == L"1");
            else if (key == L"lineageProtectionEnabled") settings.lineageProtectionEnabled = (val == L"1");
            else if (key == L"scheduleEnabled") settings.scheduleEnabled = (val == L"1");
            else if (key == L"scheduleStartHour") settings.scheduleStartHour = _wtoi(val.c_str());
            else if (key == L"scheduleEndHour") settings.scheduleEndHour = _wtoi(val.c_str());
            else if (key == L"geofenceEnabled") settings.geofenceEnabled = (val == L"1");
            else if (key == L"trustedSSID") settings.trustedSSID = val;
            else if (key == L"windowsHelloEnabled") settings.windowsHelloEnabled = (val == L"1");
            else if (key == L"fido2HardwareAuthEnabled") settings.fido2HardwareAuthEnabled = (val == L"1");
            else if (key == L"aesCloudSyncEnabled") settings.aesCloudSyncEnabled = (val == L"1");
            else if (key == L"remoteLockdownAlerts") settings.remoteLockdownAlerts = (val == L"1");
            else if (key == L"updateChannel") settings.updateChannel = val.empty() ? L"stable" : val;
            else if (key == L"autoCheckUpdates") settings.autoCheckUpdates = (val == L"1");
            else if (key == L"language") settings.language = _wtoi(val.c_str());
            else if (key == L"lockedApp") {
                if (!val.empty()) {
                    settings.lockedApps.push_back(val);
                }
            }
        }
    }
}

bool ConfigManager::LoadSettings() {
    std::wstring path = GetConfigFilePath();
    std::wifstream inFile(path);
    if (!inFile.is_open()) {
        SaveSettings();
        return true;
    }

    std::wstring firstLine;
    if (std::getline(inFile, firstLine)) {
        if (firstLine.find(L"SURAKSHA_DPAPI_CONFIG_V1") != std::wstring::npos) {
            std::wstring hexData;
            std::getline(inFile, hexData);
            inFile.close();

            std::vector<BYTE> cipherBytes = HexToBytes(hexData);
            if (!cipherBytes.empty()) {
                DATA_BLOB inBlob;
                inBlob.cbData = (DWORD)cipherBytes.size();
                inBlob.pbData = cipherBytes.data();

                DATA_BLOB outBlob;
                if (CryptUnprotectData(&inBlob, NULL, NULL, NULL, NULL, 0, &outBlob)) {
                    std::wstring plainText((wchar_t*)outBlob.pbData, outBlob.cbData / sizeof(wchar_t));
                    LocalFree(outBlob.pbData);

                    std::wstringstream ss(plainText);
                    ParseSettingsStream(ss, m_settings);
                    return true;
                }
            }
        }
    }

    inFile.clear();
    inFile.seekg(0, std::ios::beg);
    ParseSettingsStream(inFile, m_settings);
    inFile.close();

    SaveSettings();
    return true;
}

bool ConfigManager::SaveSettings() {
    std::wstringstream ss;
    ss << L"# Suraksha Configuration File\n";
    ss << L"customPinHash=" << m_settings.customPinHash << L"\n";
    ss << L"customPinSalt=" << m_settings.customPinSalt << L"\n";
    ss << L"useWindowsAuth=" << (m_settings.useWindowsAuth ? L"1" : L"0") << L"\n";
    ss << L"useCustomPin=" << (m_settings.useCustomPin ? L"1" : L"0") << L"\n";
    ss << L"protectionEnabled=" << (m_settings.protectionEnabled ? L"1" : L"0") << L"\n";
    ss << L"autoStartWithWindows=" << (m_settings.autoStartWithWindows ? L"1" : L"0") << L"\n";

    ss << L"lineageProtectionEnabled=" << (m_settings.lineageProtectionEnabled ? L"1" : L"0") << L"\n";
    ss << L"scheduleEnabled=" << (m_settings.scheduleEnabled ? L"1" : L"0") << L"\n";
    ss << L"scheduleStartHour=" << m_settings.scheduleStartHour << L"\n";
    ss << L"scheduleEndHour=" << m_settings.scheduleEndHour << L"\n";
    ss << L"geofenceEnabled=" << (m_settings.geofenceEnabled ? L"1" : L"0") << L"\n";
    ss << L"trustedSSID=" << m_settings.trustedSSID << L"\n";
    ss << L"windowsHelloEnabled=" << (m_settings.windowsHelloEnabled ? L"1" : L"0") << L"\n";
    ss << L"fido2HardwareAuthEnabled=" << (m_settings.fido2HardwareAuthEnabled ? L"1" : L"0") << L"\n";
    ss << L"aesCloudSyncEnabled=" << (m_settings.aesCloudSyncEnabled ? L"1" : L"0") << L"\n";
    ss << L"remoteLockdownAlerts=" << (m_settings.remoteLockdownAlerts ? L"1" : L"0") << L"\n";
    ss << L"updateChannel=" << m_settings.updateChannel << L"\n";
    ss << L"autoCheckUpdates=" << (m_settings.autoCheckUpdates ? L"1" : L"0") << L"\n";
    ss << L"language=" << m_settings.language << L"\n";

    for (const auto& app : m_settings.lockedApps) {
        ss << L"lockedApp=" << app << L"\n";
    }

    std::wstring plainStr = ss.str();
    DATA_BLOB inBlob;
    inBlob.cbData = (DWORD)(plainStr.length() * sizeof(wchar_t));
    inBlob.pbData = (BYTE*)plainStr.c_str();

    DATA_BLOB outBlob;
    std::wstring encryptedHex = L"";
    if (CryptProtectData(&inBlob, L"SurakshaConfig", NULL, NULL, NULL, 0, &outBlob)) {
        encryptedHex = BytesToHex(outBlob.pbData, outBlob.cbData);
        LocalFree(outBlob.pbData);
    }

    std::wstring path = GetConfigFilePath();
    std::wofstream outFile(path, std::ios::trunc);
    if (!outFile.is_open()) return false;

    if (!encryptedHex.empty()) {
        outFile << L"# SURAKSHA_DPAPI_CONFIG_V1\n";
        outFile << encryptedHex << L"\n";
    } else {
        outFile << plainStr;
    }

    outFile.close();
    SetAutoStart(m_settings.autoStartWithWindows);
    return true;
}

static bool EncryptAES256(const std::wstring& plainText, const std::wstring& passKey, std::vector<BYTE>& outBytes) {
    BYTE salt[16] = { 0 };
    HCRYPTPROV hProv = 0;
    if (CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        CryptGenRandom(hProv, sizeof(salt), salt);
        CryptReleaseContext(hProv, 0);
    }

    HCRYPTPROV hAesProv = 0;
    if (!CryptAcquireContextW(&hAesProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) return false;

    HCRYPTHASH hHash = 0;
    if (!CryptCreateHash(hAesProv, CALG_SHA_256, 0, 0, &hHash)) {
        CryptReleaseContext(hAesProv, 0);
        return false;
    }

    std::wstring saltedKey = passKey + L":" + BytesToHex(salt, sizeof(salt));
    CryptHashData(hHash, (BYTE*)saltedKey.c_str(), (DWORD)(saltedKey.length() * sizeof(wchar_t)), 0);

    HCRYPTKEY hKey = 0;
    if (!CryptDeriveKey(hAesProv, CALG_AES_256, hHash, 0, &hKey)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hAesProv, 0);
        return false;
    }
    CryptDestroyHash(hHash);

    DWORD dataLen = (DWORD)(plainText.length() * sizeof(wchar_t));
    DWORD bufLen = dataLen + 64;
    std::vector<BYTE> cipherBuf(bufLen);
    memcpy(cipherBuf.data(), plainText.c_str(), dataLen);

    if (!CryptEncrypt(hKey, 0, TRUE, 0, cipherBuf.data(), &dataLen, bufLen)) {
        CryptDestroyKey(hKey);
        CryptReleaseContext(hAesProv, 0);
        return false;
    }

    CryptDestroyKey(hKey);
    CryptReleaseContext(hAesProv, 0);

    std::string header = "SURAKSHA_ENTERPRISE_POLICY_V1\n";
    outBytes.assign(header.begin(), header.end());
    outBytes.insert(outBytes.end(), salt, salt + sizeof(salt));
    outBytes.insert(outBytes.end(), cipherBuf.data(), cipherBuf.data() + dataLen);

    return true;
}

static bool DecryptAES256(const std::vector<BYTE>& fileBytes, const std::wstring& passKey, std::wstring& outPlainText) {
    std::string header = "SURAKSHA_ENTERPRISE_POLICY_V1\n";
    if (fileBytes.size() < header.size() + 16 + 16) return false;

    if (memcmp(fileBytes.data(), header.c_str(), header.size()) != 0) return false;

    BYTE salt[16];
    memcpy(salt, fileBytes.data() + header.size(), sizeof(salt));

    size_t cipherOffset = header.size() + sizeof(salt);
    DWORD cipherLen = (DWORD)(fileBytes.size() - cipherOffset);
    std::vector<BYTE> cipherBuf(cipherLen);
    memcpy(cipherBuf.data(), fileBytes.data() + cipherOffset, cipherLen);

    HCRYPTPROV hAesProv = 0;
    if (!CryptAcquireContextW(&hAesProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) return false;

    HCRYPTHASH hHash = 0;
    if (!CryptCreateHash(hAesProv, CALG_SHA_256, 0, 0, &hHash)) {
        CryptReleaseContext(hAesProv, 0);
        return false;
    }

    std::wstring saltedKey = passKey + L":" + BytesToHex(salt, sizeof(salt));
    CryptHashData(hHash, (BYTE*)saltedKey.c_str(), (DWORD)(saltedKey.length() * sizeof(wchar_t)), 0);

    HCRYPTKEY hKey = 0;
    if (!CryptDeriveKey(hAesProv, CALG_AES_256, hHash, 0, &hKey)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hAesProv, 0);
        return false;
    }
    CryptDestroyHash(hHash);

    if (!CryptDecrypt(hKey, 0, TRUE, 0, cipherBuf.data(), &cipherLen)) {
        CryptDestroyKey(hKey);
        CryptReleaseContext(hAesProv, 0);
        return false;
    }

    CryptDestroyKey(hKey);
    CryptReleaseContext(hAesProv, 0);

    outPlainText.assign((wchar_t*)cipherBuf.data(), cipherLen / sizeof(wchar_t));
    return true;
}

bool ConfigManager::ExportEncryptedPolicy(const std::wstring& exportPath, const std::wstring& passKey) {
    SaveSettings();

    std::wstringstream ss;
    ss << L"# Suraksha Configuration File\n";
    ss << L"customPinHash=" << m_settings.customPinHash << L"\n";
    ss << L"customPinSalt=" << m_settings.customPinSalt << L"\n";
    ss << L"useWindowsAuth=" << (m_settings.useWindowsAuth ? L"1" : L"0") << L"\n";
    ss << L"useCustomPin=" << (m_settings.useCustomPin ? L"1" : L"0") << L"\n";
    ss << L"protectionEnabled=" << (m_settings.protectionEnabled ? L"1" : L"0") << L"\n";
    ss << L"autoStartWithWindows=" << (m_settings.autoStartWithWindows ? L"1" : L"0") << L"\n";
    ss << L"scheduleEnabled=" << (m_settings.scheduleEnabled ? L"1" : L"0") << L"\n";
    ss << L"scheduleStartHour=" << m_settings.scheduleStartHour << L"\n";
    ss << L"scheduleEndHour=" << m_settings.scheduleEndHour << L"\n";
    ss << L"updateChannel=" << m_settings.updateChannel << L"\n";
    ss << L"autoCheckUpdates=" << (m_settings.autoCheckUpdates ? L"1" : L"0") << L"\n";
    ss << L"language=" << m_settings.language << L"\n";

    for (const auto& app : m_settings.lockedApps) {
        ss << L"lockedApp=" << app << L"\n";
    }

    std::vector<BYTE> encryptedBytes;
    if (!EncryptAES256(ss.str(), passKey, encryptedBytes)) return false;

    std::ofstream outFile(exportPath, std::ios::binary);
    if (!outFile.is_open()) return false;

    outFile.write((char*)encryptedBytes.data(), encryptedBytes.size());
    outFile.close();
    return true;
}

bool ConfigManager::ImportEncryptedPolicy(const std::wstring& importPath, const std::wstring& passKey) {
    std::ifstream inFile(importPath, std::ios::binary | std::ios::ate);
    if (!inFile.is_open()) return false;

    std::streamsize size = inFile.tellg();
    inFile.seekg(0, std::ios::beg);

    std::vector<BYTE> buffer(size);
    if (!inFile.read((char*)buffer.data(), size)) return false;
    inFile.close();

    std::wstring plainText;
    if (!DecryptAES256(buffer, passKey, plainText)) return false;

    std::wstringstream ss(plainText);
    ParseSettingsStream(ss, m_settings);
    SaveSettings();
    return true;
}

void ConfigManager::AddLockedApp(const std::wstring& appPath) {
    if (!m_settings.isAppLocked(appPath)) {
        m_settings.lockedApps.push_back(appPath);
        SaveSettings();
    }
}

void ConfigManager::RemoveLockedApp(const std::wstring& appPath) {
    auto& list = m_settings.lockedApps;
    list.erase(std::remove_if(list.begin(), list.end(), [&](const std::wstring& item) {
        return item == appPath;
    }), list.end());
    SaveSettings();
}

void ConfigManager::SetAutoStart(bool enable) {
    m_settings.autoStartWithWindows = enable;
    HKEY hKey;
    LONG lRes = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &hKey);
    if (lRes == ERROR_SUCCESS) {
        if (enable) {
            wchar_t szPath[MAX_PATH];
            GetModuleFileNameW(NULL, szPath, MAX_PATH);
            RegSetValueExW(hKey, L"SurakshaAppLocker", 0, REG_SZ, (BYTE*)szPath, (DWORD)((wcslen(szPath) + 1) * sizeof(wchar_t)));
        } else {
            RegDeleteValueW(hKey, L"SurakshaAppLocker");
        }
        RegCloseKey(hKey);
    }
}

bool ConfigManager::IsAutoStartEnabled() const {
    return m_settings.autoStartWithWindows;
}
