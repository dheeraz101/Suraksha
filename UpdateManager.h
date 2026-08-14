#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <atomic>

enum class UpdateStatus {
    Idle,
    Checking,
    UpToDate,
    UpdateAvailable,
    Downloading,
    ReadyToInstall,
    Error
};

struct RemoteReleaseInfo {
    std::wstring version;        // e.g. "2.0.1"
    std::wstring buildTag;       // e.g. "26B0815a"
    std::wstring releaseTitle;   // e.g. "Suraksha v2.0.1-beta [Beta] - Build: 26B0815a"
    std::wstring releaseNotes;   // Release notes summary
    std::wstring downloadUrl;    // Direct URL to download executable
    std::wstring assetName;      // e.g. "SurakshaSetup-v2.0.1-x64.exe"
    size_t assetSize = 0;        // Total bytes
    bool isPrerelease = false;
};

class UpdateManager {
public:
    static UpdateManager& GetInstance();

    void Initialize(HWND hWndNotify);

    // Asynchronous Actions
    void CheckForUpdatesAsync(bool isManual = true);
    void StartDownloadAsync();
    void InstallAndRelaunch();

    // Getters & Status
    UpdateStatus GetStatus() const { return m_status.load(); }
    int GetDownloadProgress() const { return m_downloadPercent.load(); }
    size_t GetDownloadedBytes() const { return m_downloadedBytes.load(); }
    size_t GetTotalBytes() const { return m_totalBytes.load(); }
    const RemoteReleaseInfo& GetAvailableRelease() const { return m_latestRelease; }
    std::wstring GetStatusMessage() const;
    std::wstring GetLastCheckedString() const;

    // Channel Selection
    std::wstring GetChannel() const;
    void SetChannel(const std::wstring& channel);

private:
    UpdateManager();
    ~UpdateManager();

    static DWORD WINAPI CheckThreadProc(LPVOID lpParam);
    static DWORD WINAPI DownloadThreadProc(LPVOID lpParam);

    bool FetchReleasesFromGitHub(std::string& outJson);
    bool ParseLatestMatchingRelease(const std::string& json, RemoteReleaseInfo& outInfo);
    bool CompareVersions(const std::wstring& remoteVer, const std::wstring& remoteBuild, bool remoteIsBeta);

    HWND m_hWndNotify = NULL;
    std::atomic<UpdateStatus> m_status{ UpdateStatus::Idle };
    std::atomic<int> m_downloadPercent{ 0 };
    std::atomic<size_t> m_downloadedBytes{ 0 };
    std::atomic<size_t> m_totalBytes{ 0 };

    RemoteReleaseInfo m_latestRelease;
    std::wstring m_tempDownloadPath;
    std::wstring m_errorMessage;
    std::wstring m_lastCheckedTime;
    HANDLE m_hWorkerThread = NULL;
    CRITICAL_SECTION m_cs;
};
