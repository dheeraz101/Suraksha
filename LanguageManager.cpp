#include "LanguageManager.h"

LanguageManager::LanguageManager() : m_currentLang(Language::English) {
    InitStringTables();
}

void LanguageManager::SetLanguage(Language lang) {
    m_currentLang = lang;
}

std::wstring LanguageManager::GetString(const std::wstring& key) const {
    auto langIt = m_tables.find(m_currentLang);
    if (langIt != m_tables.end()) {
        auto keyIt = langIt->second.find(key);
        if (keyIt != langIt->second.end()) {
            return keyIt->second;
        }
    }
    // Fallback to English
    auto engIt = m_tables.find(Language::English);
    if (engIt != m_tables.end()) {
        auto keyIt = engIt->second.find(key);
        if (keyIt != engIt->second.end()) {
            return keyIt->second;
        }
    }
    return key;
}

void LanguageManager::InitStringTables() {
    // English
    auto& en = m_tables[Language::English];
    en[L"APP_TITLE"] = L"Suraksha — Military Security Shield";
    en[L"PROTECTED_APPS"] = L"Protected Applications";
    en[L"SECURITY_DEFENSE"] = L"Security & Defense";
    en[L"AUDIT_INSPECTOR"] = L"Audit Inspector";
    en[L"SYSTEM_STATUS"] = L"System Status";
    en[L"ADD_APP"] = L"+ Add Application...";
    en[L"REMOVE_APP"] = L"- Remove Selected";
    en[L"STATUS_PROTECTED"] = L"Protected";
    en[L"STATUS_PAUSED"] = L"Paused";

    // Hindi (हिंदी)
    auto& hi = m_tables[Language::Hindi];
    hi[L"APP_TITLE"] = L"सुरक्षा — सैन्य सुरक्षा कवच";
    hi[L"PROTECTED_APPS"] = L"सुरक्षित एप्लिकेशन";
    hi[L"SECURITY_DEFENSE"] = L"सुरक्षा एवं नीति";
    hi[L"AUDIT_INSPECTOR"] = L"ऑडिट निरीक्षक";
    hi[L"SYSTEM_STATUS"] = L"सिस्टम स्थिति";
    hi[L"ADD_APP"] = L"+ ऐप जोड़ें...";
    hi[L"REMOVE_APP"] = L"- ऐप हटाएं";
    hi[L"STATUS_PROTECTED"] = L"सुरक्षित";
    hi[L"STATUS_PAUSED"] = L"रोका गया";

    // Spanish (Español)
    auto& es = m_tables[Language::Spanish];
    es[L"APP_TITLE"] = L"Suraksha — Escudo de Seguridad Militar";
    es[L"PROTECTED_APPS"] = L"Aplicaciones Protegidas";
    es[L"SECURITY_DEFENSE"] = L"Seguridad y Defensa";
    es[L"AUDIT_INSPECTOR"] = L"Inspector de Auditoría";
    es[L"SYSTEM_STATUS"] = L"Estado del Sistema";
    es[L"ADD_APP"] = L"+ Añadir Aplicación...";
    es[L"REMOVE_APP"] = L"- Eliminar Seleccionada";
    es[L"STATUS_PROTECTED"] = L"Protegido";
    es[L"STATUS_PAUSED"] = L"Pausado";

    // German (Deutsch)
    auto& de = m_tables[Language::German];
    de[L"APP_TITLE"] = L"Suraksha — Militärisches Sicherheitsschild";
    de[L"PROTECTED_APPS"] = L"Geschützte Anwendungen";
    de[L"SECURITY_DEFENSE"] = L"Sicherheit & Abwehr";
    de[L"AUDIT_INSPECTOR"] = L"Audit-Inspektor";
    de[L"SYSTEM_STATUS"] = L"Systemstatus";
    de[L"ADD_APP"] = L"+ Anwendung hinzufügen...";
    de[L"REMOVE_APP"] = L"- Ausgewählte entfernen";
    de[L"STATUS_PROTECTED"] = L"Geschützt";
    de[L"STATUS_PAUSED"] = L"Pausiert";

    // French (Français)
    auto& fr = m_tables[Language::French];
    fr[L"APP_TITLE"] = L"Suraksha — Bouclier de Sécurité Militaire";
    fr[L"PROTECTED_APPS"] = L"Applications Protégées";
    fr[L"SECURITY_DEFENSE"] = L"Sécurité & Défense";
    fr[L"AUDIT_INSPECTOR"] = L"Inspecteur d'Audit";
    fr[L"SYSTEM_STATUS"] = L"État du Système";
    fr[L"ADD_APP"] = L"+ Ajouter une Application...";
    fr[L"REMOVE_APP"] = L"- Supprimer la Sélection";
    fr[L"STATUS_PROTECTED"] = L"Protégé";
    fr[L"STATUS_PAUSED"] = L"En Pause";

    // Japanese (日本語)
    auto& ja = m_tables[Language::Japanese];
    ja[L"APP_TITLE"] = L"Suraksha — ミリタリーセキュリティシールド";
    ja[L"PROTECTED_APPS"] = L"保護されたアプリケーション";
    ja[L"SECURITY_DEFENSE"] = L"セキュリティと防衛";
    ja[L"AUDIT_INSPECTOR"] = L"監査インスペクター";
    ja[L"SYSTEM_STATUS"] = L"システム状態";
    ja[L"ADD_APP"] = L"+ アプリを追加...";
    ja[L"REMOVE_APP"] = L"- 選択を削除";
    ja[L"STATUS_PROTECTED"] = L"保護中";
    ja[L"STATUS_PAUSED"] = L"一時停止";
}
