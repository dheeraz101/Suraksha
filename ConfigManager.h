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

    std::vector<std::wstring> lockedApps;

    bool isAppLocked(const std::wstring& appNameOrPath) const {
        std::wstring target = appNameOrPath;
        std::transform(target.begin(), target.end(), target.begin(), ::tolower);

        for (const auto& app : lockedApps) {
            std::wstring item = app;
            std::transform(item.begin(), item.end(), item.begin(), ::tolower);

            // Compare filename or full path
            if (target == item) return true;
            
            // If item is just exe name like "notepad.exe" and target is full path
            size_t slash = target.find_last_of(L"\\/");
            if (slash != std::wstring::npos) {
                std::wstring fileName = target.substr(slash + 1);
                if (fileName == item) return true;
            }
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

