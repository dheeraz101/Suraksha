#include "UpdateManager.h"
#include "ConfigManager.h"
#include "Version.h"
#include "AuditLogger.h"

#include <wininet.h>
#include <shlobj.h>
#include <shellapi.h>
#include <sstream>
#include <algorithm>

#pragma comment(lib, "wininet.lib")

struct CSLock {
    CRITICAL_SECTION& cs;
    CSLock(CRITICAL_SECTION& c) : cs(c) { EnterCriticalSection(&cs); }
    ~CSLock() { LeaveCriticalSection(&cs); }
};

UpdateManager::UpdateManager() {
    InitializeCriticalSection(&m_cs);
    m_lastCheckedTime = L"Not checked yet";
}

UpdateManager::~UpdateManager() {
    if (m_hWorkerThread) {
        WaitForSingleObject(m_hWorkerThread, 1000);
        CloseHandle(m_hWorkerThread);
        m_hWorkerThread = NULL;
    }
    DeleteCriticalSection(&m_cs);
}

UpdateManager& UpdateManager::GetInstance() {
    static UpdateManager instance;
    return instance;
}

void UpdateManager::Initialize(HWND hWndNotify) {
    m_hWndNotify = hWndNotify;
}

std::wstring UpdateManager::GetChannel() const {
    return ConfigManager::GetInstance().GetSettings().updateChannel;
}

void UpdateManager::SetChannel(const std::wstring& channel) {
    auto& settings = ConfigManager::GetInstance().GetSettings();
    settings.updateChannel = channel;
    ConfigManager::GetInstance().SaveSettings();
    AuditLogger::GetInstance().LogEvent(L"UPDATE_CHANNEL", L"Update channel switched to: " + channel);
    
    // Automatically trigger a refresh on channel switch
    CheckForUpdatesAsync(true);
}

std::wstring UpdateManager::GetLastCheckedString() const {
    CSLock lock((CRITICAL_SECTION&)m_cs);
    return m_lastCheckedTime;
}

std::wstring UpdateManager::GetStatusMessage() const {
    switch (m_status.load()) {
    case UpdateStatus::Idle:
        return L"Ready to check for software updates.";
    case UpdateStatus::Checking:
        return L"Checking for updates on GitHub...";
    case UpdateStatus::UpToDate:
        return L"Suraksha is up to date.";
    case UpdateStatus::UpdateAvailable:
        return L"A new version of Suraksha is available.";
    case UpdateStatus::Downloading:
        return L"Downloading update package...";
    case UpdateStatus::ReadyToInstall:
        return L"Update downloaded. Ready to install.";
    case UpdateStatus::Error:
        return m_errorMessage.empty() ? L"Unable to connect to update server." : m_errorMessage;
    }
    return L"";
}

void UpdateManager::CheckForUpdatesAsync(bool isManual) {
    if (m_status.load() == UpdateStatus::Checking || m_status.load() == UpdateStatus::Downloading) {
        return;
    }

    m_status.store(UpdateStatus::Checking);
    if (m_hWndNotify) InvalidateRect(m_hWndNotify, NULL, FALSE);

    if (m_hWorkerThread) {
        CloseHandle(m_hWorkerThread);
        m_hWorkerThread = NULL;
    }

    m_hWorkerThread = CreateThread(NULL, 0, CheckThreadProc, this, 0, NULL);
}

// Simple Helper: Convert UTF-8 std::string to std::wstring
static std::wstring Utf8ToWstring(const std::string& str) {
    if (str.empty()) return L"";
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), NULL, 0);
    std::wstring wstr(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), &wstr[0], sizeNeeded);
    return wstr;
}

// Simple JSON String Extractor helper
static std::string ExtractJsonStringValue(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\":";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return "";

    pos += searchKey.length();
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n')) pos++;

    if (pos >= json.length() || json[pos] != '\"') return "";
    pos++; // skip open quote

    size_t endPos = json.find('\"', pos);
    if (endPos == std::string::npos) return "";

    return json.substr(pos, endPos - pos);
}

// Simple JSON Bool Extractor helper
static bool ExtractJsonBoolValue(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\":";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return false;

    pos += searchKey.length();
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n')) pos++;

    if (json.substr(pos, 4) == "true") return true;
    return false;
}

bool UpdateManager::FetchReleasesFromGitHub(std::string& outJson) {
    HINTERNET hInternet = InternetOpenW(L"Suraksha-AppLocker-Updater/2.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) return false;

    // Direct GitHub Releases REST API endpoint
    HINTERNET hConnect = InternetOpenUrlW(
        hInternet,
        L"https://api.github.com/repos/dheeraz101/Suraksha/releases",
        L"Accept: application/vnd.github.v3+json\r\nUser-Agent: Suraksha-AppLocker\r\n",
        -1L,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE,
        0
    );

    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return false;
    }

    std::string responseData;
    char buffer[4096];
    DWORD bytesRead = 0;

    while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        responseData.append(buffer, bytesRead);
    }

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    if (responseData.empty()) return false;
    outJson = responseData;
    return true;
}

bool UpdateManager::ParseLatestMatchingRelease(const std::string& json, RemoteReleaseInfo& outInfo) {
    bool wantBeta = (GetChannel() == L"beta");

    // Split releases array by objects
    size_t curPos = 0;
    while ((curPos = json.find("{\"url\":", curPos)) != std::string::npos) {
        size_t nextPos = json.find("{\"url\":", curPos + 7);
        std::string releaseObj = (nextPos == std::string::npos) ? json.substr(curPos) : json.substr(curPos, nextPos - curPos);
        curPos = (nextPos == std::string::npos) ? json.length() : nextPos;

        bool isPrerelease = ExtractJsonBoolValue(releaseObj, "prerelease");
        
        // If channel is stable, skip prereleases
        if (!wantBeta && isPrerelease) {
            continue;
        }

        std::string tagName = ExtractJsonStringValue(releaseObj, "tag_name");
        std::string name = ExtractJsonStringValue(releaseObj, "name");
        std::string body = ExtractJsonStringValue(releaseObj, "body");

        if (tagName.empty()) continue;

        // Clean version tag: "v2.0.1" -> "2.0.1"
        std::string verClean = tagName;
        if (!verClean.empty() && (verClean[0] == 'v' || verClean[0] == 'V')) {
            verClean = verClean.substr(1);
        }
        size_t dashPos = verClean.find('-');
        std::string rawVer = (dashPos != std::string::npos) ? verClean.substr(0, dashPos) : verClean;

        // Extract build code from name if formatted like "Build: 26B0815a"
        std::string buildCode = "";
        size_t bPos = name.find("Build: ");
        if (bPos != std::string::npos) {
            buildCode = name.substr(bPos + 7);
            size_t endB = buildCode.find_first_of(" )]\r\n");
            if (endB != std::string::npos) buildCode = buildCode.substr(0, endB);
        }

        // Find Download Asset (.exe preferred, fallback to .msix)
        std::string assetUrl = "";
        std::string assetName = "";
        size_t assetSize = 0;

        size_t assetFindPos = 0;
        while ((assetFindPos = releaseObj.find("\"browser_download_url\":", assetFindPos)) != std::string::npos) {
            size_t urlStart = releaseObj.find('\"', assetFindPos + 23) + 1;
            size_t urlEnd = releaseObj.find('\"', urlStart);
            std::string url = releaseObj.substr(urlStart, urlEnd - urlStart);

            size_t nameFind = releaseObj.rfind("\"name\":\"", urlStart);
            std::string aName = "";
            if (nameFind != std::string::npos) {
                size_t nStart = nameFind + 8;
                size_t nEnd = releaseObj.find('\"', nStart);
                aName = releaseObj.substr(nStart, nEnd - nStart);
            }

            // Prefer setup installer, or fallback to standalone exe
            if (aName.find("Setup") != std::string::npos && aName.find(".exe") != std::string::npos) {
                assetUrl = url;
                assetName = aName;
                break;
            }
            if (aName.find(".exe") != std::string::npos && assetUrl.empty()) {
                assetUrl = url;
                assetName = aName;
            }
            if (aName.find(".msix") != std::string::npos && assetUrl.empty()) {
                assetUrl = url;
                assetName = aName;
            }
            assetFindPos = urlEnd;
        }

        if (assetUrl.empty()) {
            // Fallback: direct release tag download
            assetUrl = "https://github.com/dheeraz101/Suraksha/releases/tag/" + tagName;
            assetName = "SurakshaInstaller.exe";
        }

        outInfo.version = Utf8ToWstring(rawVer);
        outInfo.buildTag = Utf8ToWstring(buildCode);
        outInfo.releaseTitle = Utf8ToWstring(name.empty() ? tagName : name);
        outInfo.releaseNotes = Utf8ToWstring(body);
        outInfo.downloadUrl = Utf8ToWstring(assetUrl);
        outInfo.assetName = Utf8ToWstring(assetName);
        outInfo.isPrerelease = isPrerelease;
        outInfo.assetSize = assetSize;

        return true;
    }

    return false;
}

bool UpdateManager::CompareVersions(const std::wstring& remoteVer, const std::wstring& remoteBuild, bool remoteIsBeta) {
    int curMajor = SURAKSHA_VERSION_MAJOR;
    int curMinor = SURAKSHA_VERSION_MINOR;
    int curPatch = SURAKSHA_VERSION_PATCH;

    int rMajor = 0, rMinor = 0, rPatch = 0;
    swscanf_s(remoteVer.c_str(), L"%d.%d.%d", &rMajor, &rMinor, &rPatch);

    // 1. Semantic Version comparison
    if (rMajor > curMajor) return true;
    if (rMajor < curMajor) return false;

    if (rMinor > curMinor) return true;
    if (rMinor < curMinor) return false;

    if (rPatch > curPatch) return true;
    if (rPatch < curPatch) return false;

    // 2. If same semantic version, check build tag
    if (!remoteBuild.empty()) {
        std::wstring curBuild = SURAKSHA_BUILD_TAG;
        if (remoteBuild != curBuild) {
            // If remote build tag string is lexically higher (e.g. 26B0815b > 26B0815a)
            if (remoteBuild > curBuild) return true;
        }
    }

    return false;
}

DWORD WINAPI UpdateManager::CheckThreadProc(LPVOID lpParam) {
    UpdateManager* pThis = (UpdateManager*)lpParam;
    DWORD startTick = GetTickCount();

    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t timeBuf[64];
    swprintf_s(timeBuf, 64, L"Today, %02d:%02d", st.wHour, st.wMinute);

    std::string json;
    bool fetchOk = pThis->FetchReleasesFromGitHub(json);

    RemoteReleaseInfo remoteInfo;
    bool parseOk = false;
    if (fetchOk) {
        parseOk = pThis->ParseLatestMatchingRelease(json, remoteInfo);
    }

    // Ensure a smooth, realistic checking feel of at least 2.0 seconds
    DWORD elapsed = GetTickCount() - startTick;
    if (elapsed < 2000) {
        Sleep(2000 - elapsed);
    }

    if (!fetchOk) {
        pThis->m_errorMessage = L"Unable to connect to GitHub update server.";
        pThis->m_status.store(UpdateStatus::Error);
        {
            CSLock lock(pThis->m_cs);
            pThis->m_lastCheckedTime = timeBuf;
        }
        if (pThis->m_hWndNotify) InvalidateRect(pThis->m_hWndNotify, NULL, FALSE);
        return 0;
    }

    if (!parseOk) {
        pThis->m_status.store(UpdateStatus::UpToDate);
        {
            CSLock lock(pThis->m_cs);
            pThis->m_lastCheckedTime = timeBuf;
        }
        if (pThis->m_hWndNotify) InvalidateRect(pThis->m_hWndNotify, NULL, FALSE);
        return 0;
    }

    bool isNewer = pThis->CompareVersions(remoteInfo.version, remoteInfo.buildTag, remoteInfo.isPrerelease);

    {
        CSLock lock(pThis->m_cs);
        pThis->m_latestRelease = remoteInfo;
        pThis->m_lastCheckedTime = timeBuf;
    }

    if (isNewer) {
        pThis->m_status.store(UpdateStatus::UpdateAvailable);
        AuditLogger::GetInstance().LogEvent(L"UPDATE_FOUND", L"New update available: " + remoteInfo.releaseTitle);
    } else {
        pThis->m_status.store(UpdateStatus::UpToDate);
    }

    if (pThis->m_hWndNotify) InvalidateRect(pThis->m_hWndNotify, NULL, FALSE);
    return 0;
}

void UpdateManager::StartDownloadAsync() {
    if (m_status.load() != UpdateStatus::UpdateAvailable) return;

    m_status.store(UpdateStatus::Downloading);
    m_downloadPercent.store(0);
    m_downloadedBytes.store(0);
    m_totalBytes.store(0);

    if (m_hWndNotify) InvalidateRect(m_hWndNotify, NULL, FALSE);

    if (m_hWorkerThread) {
        CloseHandle(m_hWorkerThread);
        m_hWorkerThread = NULL;
    }

    m_hWorkerThread = CreateThread(NULL, 0, DownloadThreadProc, this, 0, NULL);
}

DWORD WINAPI UpdateManager::DownloadThreadProc(LPVOID lpParam) {
    UpdateManager* pThis = (UpdateManager*)lpParam;

    std::wstring url = pThis->m_latestRelease.downloadUrl;
    if (url.empty()) {
        pThis->m_status.store(UpdateStatus::Error);
        pThis->m_errorMessage = L"Download URL is invalid.";
        if (pThis->m_hWndNotify) InvalidateRect(pThis->m_hWndNotify, NULL, FALSE);
        return 0;
    }

    // Setup Temp Directory
    wchar_t tempDir[MAX_PATH];
    GetTempPathW(MAX_PATH, tempDir);
    std::wstring outDir = std::wstring(tempDir) + L"SurakshaUpdate";
    CreateDirectoryW(outDir.c_str(), NULL);

    std::wstring outFilePath = outDir + L"\\" + (pThis->m_latestRelease.assetName.empty() ? L"SurakshaSetup.exe" : pThis->m_latestRelease.assetName);

    HINTERNET hInternet = InternetOpenW(L"Suraksha-AppLocker-Updater/2.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) {
        pThis->m_status.store(UpdateStatus::Error);
        pThis->m_errorMessage = L"Could not initialize download connection.";
        if (pThis->m_hWndNotify) InvalidateRect(pThis->m_hWndNotify, NULL, FALSE);
        return 0;
    }

    HINTERNET hFile = InternetOpenUrlW(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0);
    if (!hFile) {
        InternetCloseHandle(hInternet);
        pThis->m_status.store(UpdateStatus::Error);
        pThis->m_errorMessage = L"Could not connect to download mirror.";
        if (pThis->m_hWndNotify) InvalidateRect(pThis->m_hWndNotify, NULL, FALSE);
        return 0;
    }

    // Get Content-Length
    DWORD contentLength = 0;
    DWORD sizeLength = sizeof(contentLength);
    HttpQueryInfoW(hFile, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &contentLength, &sizeLength, NULL);
    pThis->m_totalBytes.store(contentLength);

    HANDLE hOut = CreateFileW(outFilePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hOut == INVALID_HANDLE_VALUE) {
        InternetCloseHandle(hFile);
        InternetCloseHandle(hInternet);
        pThis->m_status.store(UpdateStatus::Error);
        pThis->m_errorMessage = L"Could not write installer to disk.";
        if (pThis->m_hWndNotify) InvalidateRect(pThis->m_hWndNotify, NULL, FALSE);
        return 0;
    }

    BYTE buffer[8192];
    DWORD bytesRead = 0;
    DWORD totalDownloaded = 0;

    while (InternetReadFile(hFile, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        DWORD bytesWritten = 0;
        WriteFile(hOut, buffer, bytesRead, &bytesWritten, NULL);
        totalDownloaded += bytesRead;
        pThis->m_downloadedBytes.store(totalDownloaded);

        if (contentLength > 0) {
            int pct = (int)((totalDownloaded * 100) / contentLength);
            pThis->m_downloadPercent.store(pct);
        }

        if (pThis->m_hWndNotify) InvalidateRect(pThis->m_hWndNotify, NULL, FALSE);
    }

    CloseHandle(hOut);
    InternetCloseHandle(hFile);
    InternetCloseHandle(hInternet);

    {
        CSLock lock(pThis->m_cs);
        pThis->m_tempDownloadPath = outFilePath;
    }

    pThis->m_downloadPercent.store(100);
    pThis->m_status.store(UpdateStatus::ReadyToInstall);
    AuditLogger::GetInstance().LogEvent(L"UPDATE_DOWNLOADED", L"Update package downloaded successfully: " + outFilePath);

    if (pThis->m_hWndNotify) InvalidateRect(pThis->m_hWndNotify, NULL, FALSE);
    return 0;
}

void UpdateManager::InstallAndRelaunch() {
    std::wstring installerPath;
    {
        CSLock lock(m_cs);
        installerPath = m_tempDownloadPath;
    }

    if (!installerPath.empty() && (GetFileAttributesW(installerPath.c_str()) != INVALID_FILE_ATTRIBUTES)) {
        // Launch installer with silent flag
        ShellExecuteW(NULL, L"open", installerPath.c_str(), L"/SILENT /RESTART", NULL, SW_SHOWNORMAL);
        AuditLogger::GetInstance().LogEvent(L"UPDATE_INSTALLING", L"Executing update installer: " + installerPath);
        PostQuitMessage(0);
    }
}
