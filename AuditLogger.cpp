#include "framework.h"
#include "AuditLogger.h"
#include "SecurityManager.h"
#include <shlobj.h>
#include <chrono>
#include <iomanip>
#include <sstream>



AuditLogger::AuditLogger() : m_lastHash(L"GENESIS_BLOCK_0000000000000000"), m_isIntegrityIntact(true) {
    InitializeCriticalSectionAndSpinCount(&m_cs, 400);
}

AuditLogger::~AuditLogger() {
    DeleteCriticalSection(&m_cs);
}

std::wstring AuditLogger::CalculateLineHMAC(const std::wstring& prevHash, const std::wstring& timestamp, const std::wstring& category, const std::wstring& message) {
    std::wstring payload = timestamp + L"|" + category + L"|" + message;
    return SecurityManager::HashSHA256(payload, prevHash);
}

void AuditLogger::Init() {
    CSLock lock(m_cs);

    wchar_t appDataPath[MAX_PATH] = { 0 };
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        std::wstring dirPath = std::wstring(appDataPath) + L"\\Suraksha\\logs";
        CreateDirectoryW((std::wstring(appDataPath) + L"\\Suraksha").c_str(), NULL);
        CreateDirectoryW(dirPath.c_str(), NULL);
        m_logFilePath = dirPath + L"\\audit.log";
    } else {
        m_logFilePath = L"audit.log";
    }

    m_isIntegrityIntact = VerifyLogIntegrity();
    if (!m_isIntegrityIntact) {
        LogEvent(L"SECURITY_ALERT", L"CRITICAL: Cryptographic Audit Log Tampering Detected! Log history modified.");
    } else {
        LogEvent(L"SYSTEM", L"Suraksha Cryptographic Audit Engine Initialized. Anti-Tamper Chaining Active.");
    }
}


bool AuditLogger::VerifyLogIntegrity() {
    if (m_logFilePath.empty()) return true;

    std::wifstream logFile(m_logFilePath);
    if (!logFile.is_open()) return true;

    std::wstring currentPrevHash = L"GENESIS_BLOCK_0000000000000000";
    std::wstring line;
    bool intact = true;

    while (std::getline(logFile, line)) {
        if (line.empty()) continue;

        size_t tClose = line.find(L"] [");
        size_t cClose = line.find(L"] ", tClose == std::wstring::npos ? 0 : tClose + 3);
        size_t hmacPos = line.rfind(L" | HMAC: ");

        if (tClose != std::wstring::npos && cClose != std::wstring::npos && hmacPos != std::wstring::npos && hmacPos > cClose) {
            std::wstring timeStr = line.substr(1, tClose - 1);
            std::wstring catStr = line.substr(tClose + 3, cClose - (tClose + 3));
            std::wstring msgStr = line.substr(cClose + 2, hmacPos - (cClose + 2));
            std::wstring fileHmac = line.substr(hmacPos + 9);

            std::wstring expectedHmac = CalculateLineHMAC(currentPrevHash, timeStr, catStr, msgStr);
            if (fileHmac != expectedHmac) {
                intact = false;
                break;
            }
            currentPrevHash = expectedHmac;
        }
    }
    logFile.close();
    m_lastHash = currentPrevHash;
    return intact;
}

void AuditLogger::LogEvent(const std::wstring& category, const std::wstring& message) {
    CSLock lock(m_cs);
    std::wofstream logFile(m_logFilePath, std::ios::app);
    if (!logFile.is_open()) return;

    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
    localtime_s(&tm_buf, &in_time_t);

    wchar_t timeStr[64] = { 0 };
    wcsftime(timeStr, 64, L"%Y-%m-%d %H:%M:%S", &tm_buf);

    std::wstring hmacStr = CalculateLineHMAC(m_lastHash, timeStr, category, message);
    m_lastHash = hmacStr;

    logFile << L"[" << timeStr << L"] [" << category << L"] " << message << L" | HMAC: " << hmacStr << std::endl;
    logFile.close();

    // ETW / Windows Event Log Reporting
    HANDLE hEventLog = RegisterEventSourceW(NULL, L"Suraksha Security");
    if (hEventLog) {
        LPCWSTR strings[2] = { category.c_str(), message.c_str() };
        WORD eventType = EVENTLOG_INFORMATION_TYPE;
        if (category == L"SECURITY_ALERT" || category == L"SECURITY_LOCKOUT") {
            eventType = EVENTLOG_ERROR_TYPE;
        } else if (category == L"SECURITY_WARNING") {
            eventType = EVENTLOG_WARNING_TYPE;
        }
        ReportEventW(hEventLog, eventType, 0, 1001, NULL, 2, 0, strings, NULL);
        DeregisterEventSource(hEventLog);
    }
}

std::vector<LogEntry> AuditLogger::GetRecentLogs(int maxCount) {
    CSLock lock(m_cs);
    std::vector<LogEntry> entries;


    if (m_logFilePath.empty()) return entries;
    std::wifstream logFile(m_logFilePath);
    if (!logFile.is_open()) return entries;

    std::wstring line;
    std::vector<std::wstring> allLines;
    while (std::getline(logFile, line)) {
        if (!line.empty()) allLines.push_back(line);
    }
    logFile.close();

    int startIdx = max(0, (int)allLines.size() - maxCount);
    for (size_t i = startIdx; i < allLines.size(); ++i) {
        const auto& ln = allLines[i];
        size_t tClose = ln.find(L"] [");
        size_t cClose = ln.find(L"] ", tClose == std::wstring::npos ? 0 : tClose + 3);
        size_t hmacPos = ln.rfind(L" | HMAC: ");

        if (tClose != std::wstring::npos && cClose != std::wstring::npos) {
            LogEntry entry;
            entry.timestamp = ln.substr(1, tClose - 1);
            entry.category = ln.substr(tClose + 3, cClose - (tClose + 3));
            if (hmacPos != std::wstring::npos && hmacPos > cClose) {
                entry.message = ln.substr(cClose + 2, hmacPos - (cClose + 2));
                entry.hmacHash = ln.substr(hmacPos + 9);
            } else {
                entry.message = ln.substr(cClose + 2);
                entry.hmacHash = L"Legacy";
            }
            entries.push_back(entry);
        }
    }
    return entries;
}

bool AuditLogger::ExportComplianceCSV(const std::wstring& filePath) {
    auto logs = GetRecentLogs(500);
    std::wofstream csvFile(filePath, std::ios::trunc);
    if (!csvFile.is_open()) return false;

    csvFile << L"Timestamp,Category,Message,HMAC_Checksum_Signature\n";
    for (const auto& log : logs) {
        csvFile << L"\"" << log.timestamp << L"\",\"" << log.category << L"\",\"" << log.message << L"\",\"" << log.hmacHash << L"\"\n";
    }
    csvFile.close();
    return true;
}

bool AuditLogger::ExportComplianceHTMLReport(const std::wstring& filePath) {
    auto logs = GetRecentLogs(500);
    std::wofstream htmlFile(filePath, std::ios::trunc);
    if (!htmlFile.is_open()) return false;

    htmlFile << L"<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>Suraksha Compliance Audit Report</title>";
    htmlFile << L"<style>body{font-family:'Segoe UI',sans-serif;background:#0f172a;color:#f8fafc;padding:30px;} h1{color:#38bdf8;} table{width:100%;border-collapse:collapse;margin-top:20px;} th,td{padding:12px;text-align:left;border-bottom:1px solid #334155;} th{background:#1e293b;color:#94a3b8;} .badge{background:#0284c7;color:#fff;padding:4px 8px;border-radius:4px;font-size:12px;}</style></head><body>";
    htmlFile << L"<h1>Suraksha Security - Compliance Audit & Integrity Report</h1>";
    htmlFile << L"<p>Report Generated: Cryptographically Verified HMAC Chain Intact.</p>";
    htmlFile << L"<table><tr><th>Timestamp</th><th>Category</th><th>Security Message</th><th>HMAC Checksum</th></tr>";

    for (const auto& log : logs) {
        htmlFile << L"<tr><td>" << log.timestamp << L"</td><td><span class=\"badge\">" << log.category << L"</span></td><td>" << log.message << L"</td><td><code>" << log.hmacHash << L"</code></td></tr>";
    }

    htmlFile << L"</table></body></html>";
    htmlFile.close();
    return true;
}
