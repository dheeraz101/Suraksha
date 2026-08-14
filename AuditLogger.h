#pragma once

#include <windows.h>
#include <string>
#include <fstream>
#include <mutex>

#include <vector>

struct LogEntry {
    std::wstring timestamp;
    std::wstring category;
    std::wstring message;
    std::wstring hmacHash;
};

class AuditLogger {
public:
    static AuditLogger& GetInstance() {
        static AuditLogger instance;
        return instance;
    }

    void Init();
    void LogEvent(const std::wstring& category, const std::wstring& message);
    bool VerifyLogIntegrity();
    std::vector<LogEntry> GetRecentLogs(int maxCount = 50);

    bool ExportComplianceCSV(const std::wstring& filePath);
    bool ExportComplianceHTMLReport(const std::wstring& filePath);

    bool IsIntegrityIntact() const { return m_isIntegrityIntact; }

    std::wstring GetLogFilePath() const { return m_logFilePath; }

private:
    AuditLogger();
    ~AuditLogger();
    AuditLogger(const AuditLogger&) = delete;
    AuditLogger& operator=(const AuditLogger&) = delete;

    std::wstring CalculateLineHMAC(const std::wstring& prevHash, const std::wstring& timestamp, const std::wstring& category, const std::wstring& message);

    std::wstring m_logFilePath;
    std::wstring m_lastHash;
    bool m_isIntegrityIntact;
    CRITICAL_SECTION m_cs;
};


