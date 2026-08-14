#pragma once

#include <string>
#include <map>

enum class Language {
    English = 0,
    Hindi,
    Spanish,
    German,
    French,
    Japanese
};

class LanguageManager {
public:
    static LanguageManager& GetInstance() {
        static LanguageManager instance;
        return instance;
    }

    void SetLanguage(Language lang);
    Language GetCurrentLanguage() const { return m_currentLang; }

    std::wstring GetString(const std::wstring& key) const;

private:
    LanguageManager();
    void InitStringTables();

    Language m_currentLang;
    std::map<Language, std::map<std::wstring, std::wstring>> m_tables;
};
