#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <algorithm>

struct AppSettings {
    std::wstring customPinHash;
    std::wstring customPinSalt;
    bool useWindowsAuth = true;
    bool useCustomPin = true;
    bool protectionEnabled = true;
    bool autoStartWithWindows = false;

    // Advanced Enterprise Features
    bool lineageProtectionEnabled = true;
    bool scheduleEnabled = false;
    int scheduleStartHour = 9;
    int scheduleEndHour = 17;
    bool geofenceEnabled = false;
    std::wstring trustedSSID = L"Home_Secure_WiFi";
    bool windowsHelloEnabled = true;
    bool fido2HardwareAuthEnabled = false;
    bool aesCloudSyncEnabled = false;
    bool remoteLockdownAlerts = true;
    std::wstring updateChannel = L"stable";
    bool autoCheckUpdates = true;

    std::vector<std::wstring> lockedApps;

    bool isAppLocked(const std::wstring& appNameOrPath) const {
        std::wstring target = appNameOrPath;
        std::transform(target.begin(), target.end(), target.begin(), ::towlower);

        std::wstring targetName = target;
        size_t tSlash = target.find_last_of(L"\\/");
        if (tSlash != std::wstring::npos) {
            targetName = target.substr(tSlash + 1);
        }

        for (const auto& app : lockedApps) {
            std::wstring item = app;
            std::transform(item.begin(), item.end(), item.begin(), ::towlower);

            // 1. Direct path or string match
            if (target == item) return true;

            // 2. Extract filename from item and compare
            std::wstring itemName = item;
            size_t iSlash = item.find_last_of(L"\\/");
            if (iSlash != std::wstring::npos) {
                itemName = item.substr(iSlash + 1);
            }

            if (targetName == itemName) return true;
        }
        return false;
    }
};

class ConfigManager {
public:
    static ConfigManager& GetInstance();

    bool LoadSettings();
    bool SaveSettings();

    bool ExportEncryptedPolicy(const std::wstring& exportPath, const std::wstring& passKey);
    bool ImportEncryptedPolicy(const std::wstring& importPath, const std::wstring& passKey);

    AppSettings& GetSettings() { return m_settings; }
    void AddLockedApp(const std::wstring& appPath);
    void RemoveLockedApp(const std::wstring& appPath);
    void SetAutoStart(bool enable);
    bool IsAutoStartEnabled() const;

private:
    ConfigManager() = default;
    std::wstring GetConfigFilePath();

    AppSettings m_settings;
};
