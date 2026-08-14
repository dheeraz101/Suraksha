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
    // ═══════════════════════════════════════════════════════════════
    // English (Default)
    // ═══════════════════════════════════════════════════════════════
    auto& en = m_tables[Language::English];
    en[L"TAB_APPLOCKER"] = L"App Locker";
    en[L"TAB_SECURITY"] = L"Security & Auth";
    en[L"TAB_UPDATES"] = L"Software Update";
    en[L"TAB_AUDIT"] = L"Audit Logs";
    en[L"TAB_LOGS"] = L"Audit Logs";
    en[L"TAB_ABOUT"] = L"About Suraksha";
    en[L"PROTECTED_APPS"] = L"Protected Applications";
    en[L"SECURITY_DEFENSE"] = L"Security & Authentication";
    en[L"SCHEDULE_PROTECTION"] = L"Scheduled Protection (Active Hours)";
    en[L"SCHEDULE_DESC"] = L"Limit process lock enforcement to specific working hours.";
    en[L"SCHEDULE_ENABLE"] = L"Enable Active Hours Schedule Filter";
    en[L"SCHEDULE_START"] = L"Start Hour";
    en[L"SCHEDULE_END"] = L"End Hour";
    en[L"POLICY_ENTERPRISE"] = L"Enterprise Policy & Backup";
    en[L"POLICY_DESC"] = L"Export and import AES-256 encrypted security configurations.";
    en[L"POLICY_EXPORT"] = L"Export Policy (.suraksha)";
    en[L"POLICY_IMPORT"] = L"Import Policy (.suraksha)";
    en[L"LANGUAGE_SECTION"] = L"Display Language";
    en[L"STATUS_PROTECTED"] = L"Protected";
    en[L"STATUS_PAUSED"] = L"Paused";
    en[L"ADD_APP"] = L"Add Application...";
    en[L"REMOVE_APP"] = L"Remove App";

    // ═══════════════════════════════════════════════════════════════
    // Hindi (हिंदी)
    // ═══════════════════════════════════════════════════════════════
    auto& hi = m_tables[Language::Hindi];
    hi[L"TAB_APPLOCKER"] = L"\x0910\x092A \x0932\x0949\x0915\x0930";
    hi[L"TAB_SECURITY"] = L"\x0938\x0941\x0930\x0915\x094D\x0937\x093E \x090F\x0935\x0902 \x092A\x094D\x0930\x092E\x093E\x0923\x0940\x0915\x0930\x0923";
    hi[L"TAB_UPDATES"] = L"\x0938\x0949\x092B\x094D\x091F\x0935\x0947\x092F\x0930 \x0905\x092A\x0921\x0947\x091F";
    hi[L"TAB_AUDIT"] = L"\x0911\x0921\x093F\x091F \x0932\x0949\x0917";
    hi[L"TAB_LOGS"] = L"\x0911\x0921\x093F\x091F \x0932\x0949\x0917";
    hi[L"TAB_ABOUT"] = L"\x0938\x0941\x0930\x0915\x094D\x0937\x093E \x0915\x0947 \x092C\x093E\x0930\x0947 \x092E\x0947\x0902";
    hi[L"PROTECTED_APPS"] = L"\x0938\x0941\x0930\x0915\x094D\x0937\x093F\x0924 \x090F\x092A\x094D\x0932\x093F\x0915\x0947\x0936\x0928";
    hi[L"SECURITY_DEFENSE"] = L"\x0938\x0941\x0930\x0915\x094D\x0937\x093E \x090F\x0935\x0902 \x092A\x094D\x0930\x092E\x093E\x0923\x0940\x0915\x0930\x0923";
    hi[L"SCHEDULE_PROTECTION"] = L"\x0928\x093F\x0930\x094D\x0927\x093E\x0930\x093F\x0924 \x0938\x0941\x0930\x0915\x094D\x0937\x093E (\x0938\x0915\x094D\x0930\x093F\x092F \x0918\x0902\x091F\x0947)";
    hi[L"SCHEDULE_DESC"] = L"\x0915\x0947\x0935\x0932 \x0928\x093F\x0930\x094D\x0927\x093E\x0930\x093F\x0924 \x0915\x093E\x0930\x094D\x092F \x0918\x0902\x091F\x094B\x0902 \x0915\x0947 \x0926\x094C\x0930\x093E\x0928 \x090F\x092A \x0932\x0949\x0915 \x0932\x093E\x0917\x0942 \x0915\x0930\x0947\x0902\x0964";
    hi[L"SCHEDULE_ENABLE"] = L"\x0938\x0915\x094D\x0930\x093F\x092F \x0918\x0902\x091F\x0947 \x092B\x093C\x093F\x0932\x094D\x091F\x0930 \x0938\x0915\x094D\x0937\x092E \x0915\x0930\x0947\x0902";
    hi[L"SCHEDULE_START"] = L"\x092A\x094D\x0930\x093E\x0930\x0902\x092D \x0938\x092E\x092F";
    hi[L"SCHEDULE_END"] = L"\x0938\x092E\x093E\x092A\x094D\x0924\x093F \x0938\x092E\x092F";
    hi[L"POLICY_ENTERPRISE"] = L"\x090F\x0902\x091F\x0930\x092A\x094D\x0930\x093E\x0907\x091C\x093C \x0928\x0940\x0924\x093F \x090F\x0935\x0902 \x092C\x0948\x0915\x0905\x092A";
    hi[L"POLICY_DESC"] = L"AES-256 \x090F\x0928\x094D\x0915\x094D\x0930\x093F\x092A\x094D\x091F\x0947\x0921 \x0938\x0941\x0930\x0915\x094D\x0937\x093E \x0915\x0949\x0928\x094D\x092B\x093C\x093F\x0917\x0930\x0947\x0936\x0928 \x0906\x092F\x093E\x0924/\x0928\x093F\x0930\x094D\x092F\x093E\x0924 \x0915\x0930\x0947\x0902\x0964";
    hi[L"POLICY_EXPORT"] = L"\x0928\x0940\x0924\x093F \x0928\x093F\x0930\x094D\x092F\x093E\x0924 \x0915\x0930\x0947\x0902 (.suraksha)";
    hi[L"POLICY_IMPORT"] = L"\x0928\x0940\x0924\x093F \x0906\x092F\x093E\x0924 \x0915\x0930\x0947\x0902 (.suraksha)";
    hi[L"LANGUAGE_SECTION"] = L"\x092A\x094D\x0930\x0926\x0930\x094D\x0936\x0928 \x092D\x093E\x0937\x093E";
    hi[L"STATUS_PROTECTED"] = L"\x0938\x0941\x0930\x0915\x094D\x0937\x093F\x0924";
    hi[L"STATUS_PAUSED"] = L"\x0930\x094B\x0915\x093E \x0917\x092F\x093E";
    hi[L"ADD_APP"] = L"\x090F\x092A \x091C\x094B\x0921\x093c\x0947\x0902...";
    hi[L"REMOVE_APP"] = L"\x090F\x092A \x0939\x091F\x093E\x090F\x0902";

    // ═══════════════════════════════════════════════════════════════
    // Spanish (Español)
    // ═══════════════════════════════════════════════════════════════
    auto& es = m_tables[Language::Spanish];
    es[L"TAB_APPLOCKER"] = L"Bloqueador";
    es[L"TAB_SECURITY"] = L"Seguridad";
    es[L"TAB_UPDATES"] = L"Actualizaciones";
    es[L"TAB_AUDIT"] = L"Auditor\x00ED""a";
    es[L"TAB_LOGS"] = L"Auditor\x00ED""a";
    es[L"TAB_ABOUT"] = L"Acerca de";
    es[L"PROTECTED_APPS"] = L"Aplicaciones Protegidas";
    es[L"SECURITY_DEFENSE"] = L"Seguridad y Autenticaci\x00F3""n";
    es[L"SCHEDULE_PROTECTION"] = L"Protecci\x00F3""n Programada (Horario)";
    es[L"SCHEDULE_DESC"] = L"Limita el bloqueo a horas de trabajo espec\x00ED""ficas.";
    es[L"SCHEDULE_ENABLE"] = L"Activar Filtro de Horas Activas";
    es[L"SCHEDULE_START"] = L"Hora Inicio";
    es[L"SCHEDULE_END"] = L"Hora Fin";
    es[L"POLICY_ENTERPRISE"] = L"Pol\x00ED""ticas Empresariales y Respaldo";
    es[L"POLICY_DESC"] = L"Exporta e importa configuraciones cifradas en AES-256.";
    es[L"POLICY_EXPORT"] = L"Exportar Pol\x00ED""tica (.suraksha)";
    es[L"POLICY_IMPORT"] = L"Importar Pol\x00ED""tica (.suraksha)";
    es[L"LANGUAGE_SECTION"] = L"Idioma";
    es[L"STATUS_PROTECTED"] = L"Protegido";
    es[L"STATUS_PAUSED"] = L"Pausado";
    es[L"ADD_APP"] = L"A\x00F1""adir Aplicaci\x00F3""n...";
    es[L"REMOVE_APP"] = L"Eliminar";

    // ═══════════════════════════════════════════════════════════════
    // German (Deutsch)
    // ═══════════════════════════════════════════════════════════════
    auto& de = m_tables[Language::German];
    de[L"TAB_APPLOCKER"] = L"App-Sperre";
    de[L"TAB_SECURITY"] = L"Sicherheit";
    de[L"TAB_UPDATES"] = L"Software-Update";
    de[L"TAB_AUDIT"] = L"Audit-Protokoll";
    de[L"TAB_LOGS"] = L"Audit-Protokoll";
    de[L"TAB_ABOUT"] = L"\x00DC""ber Suraksha";
    de[L"PROTECTED_APPS"] = L"Gesch\x00FC""tzte Anwendungen";
    de[L"SECURITY_DEFENSE"] = L"Sicherheit & Authentifizierung";
    de[L"SCHEDULE_PROTECTION"] = L"Geplanter Schutz (Aktive Zeiten)";
    de[L"SCHEDULE_DESC"] = L"Beschr\x00E4""nke die App-Sperre auf bestimmte Arbeitszeiten.";
    de[L"SCHEDULE_ENABLE"] = L"Zeitfilter f\x00FC""r aktive Stunden aktivieren";
    de[L"SCHEDULE_START"] = L"Startzeit";
    de[L"SCHEDULE_END"] = L"Endzeit";
    de[L"POLICY_ENTERPRISE"] = L"Unternehmensrichtlinie & Backup";
    de[L"POLICY_DESC"] = L"AES-256-verschl\x00FC""sselte Konfigurationen exportieren/importieren.";
    de[L"POLICY_EXPORT"] = L"Richtlinie exportieren (.suraksha)";
    de[L"POLICY_IMPORT"] = L"Richtlinie importieren (.suraksha)";
    de[L"LANGUAGE_SECTION"] = L"Sprache";
    de[L"STATUS_PROTECTED"] = L"Gesch\x00FC""tzt";
    de[L"STATUS_PAUSED"] = L"Pausiert";
    de[L"ADD_APP"] = L"App hinzuf\x00FC""gen...";
    de[L"REMOVE_APP"] = L"Entfernen";

    // ═══════════════════════════════════════════════════════════════
    // French (Français)
    // ═══════════════════════════════════════════════════════════════
    auto& fr = m_tables[Language::French];
    fr[L"TAB_APPLOCKER"] = L"Verrou d'app";
    fr[L"TAB_SECURITY"] = L"S\x00E9""curit\x00E9";
    fr[L"TAB_UPDATES"] = L"Mise \x00E0 jour";
    fr[L"TAB_AUDIT"] = L"Journaux";
    fr[L"TAB_LOGS"] = L"Journaux";
    fr[L"TAB_ABOUT"] = L"\x00C0 propos";
    fr[L"PROTECTED_APPS"] = L"Applications Prot\x00E9""g\x00E9""es";
    fr[L"SECURITY_DEFENSE"] = L"S\x00E9""curit\x00E9 & Authentification";
    fr[L"SCHEDULE_PROTECTION"] = L"Protection Programm\x00E9""e (Heures Actives)";
    fr[L"SCHEDULE_DESC"] = L"Limiter le verrouillage \x00E0 des horaires de travail pr\x00E9""cis.";
    fr[L"SCHEDULE_ENABLE"] = L"Activer le filtre d'heures de travail";
    fr[L"SCHEDULE_START"] = L"Heure D\x00E9""but";
    fr[L"SCHEDULE_END"] = L"Heure Fin";
    fr[L"POLICY_ENTERPRISE"] = L"Politique Entreprise & Sauvegarde";
    fr[L"POLICY_DESC"] = L"Exporter et importer des configurations chiffr\x00E9""es en AES-256.";
    fr[L"POLICY_EXPORT"] = L"Exporter la politique (.suraksha)";
    fr[L"POLICY_IMPORT"] = L"Importer la politique (.suraksha)";
    fr[L"LANGUAGE_SECTION"] = L"Langue";
    fr[L"STATUS_PROTECTED"] = L"Prot\x00E9""g\x00E9";
    fr[L"STATUS_PAUSED"] = L"En Pause";
    fr[L"ADD_APP"] = L"Ajouter une appli...";
    fr[L"REMOVE_APP"] = L"Supprimer";

    // ═══════════════════════════════════════════════════════════════
    // Japanese (日本語)
    // ═══════════════════════════════════════════════════════════════
    auto& ja = m_tables[Language::Japanese];
    ja[L"TAB_APPLOCKER"] = L"\x30A2\x30D7\x30EA\x30ED\x30C3\x30AF";
    ja[L"TAB_SECURITY"] = L"\x30BB\x30AD\x30E5\x30EA\x30C6\x30A3";
    ja[L"TAB_UPDATES"] = L"\x30BD\x30D5\x30C8\x30A6\x30A7\x30A2\x66F4\x65B0";
    ja[L"TAB_AUDIT"] = L"\x76E3\x67FB\x30ED\x30B0";
    ja[L"TAB_LOGS"] = L"\x76E3\x67FB\x30ED\x30B0";
    ja[L"TAB_ABOUT"] = L"Suraksha \x306B\x3064\x3044\x3066";
    ja[L"PROTECTED_APPS"] = L"\x4FDD\x8B77\x3055\x308C\x305F\x30A2\x30D7\x30EA";
    ja[L"SECURITY_DEFENSE"] = L"\x30BB\x30AD\x30E5\x30EA\x30C6\x30A3\x3068\x8A8D\x8A3C";
    ja[L"SCHEDULE_PROTECTION"] = L"\x30B9\x30B1\x30B8\x30E5\x30FC\x30EB\x4FDD\x8B77\xFF08\x52E4\x52D9\x6642\x9593\xFF09";
    ja[L"SCHEDULE_DESC"] = L"\x6307\x5B9A\x3057\x305F\x6642\x9593\x5E2F\x306E\x307F\x30A2\x30D7\x30EA\x30ED\x30C3\x30AF\x3092\x9069\x7528\x3057\x307E\x3059\x3002";
    ja[L"SCHEDULE_ENABLE"] = L"\x6642\x9593\x5E2F\x30D5\x30A3\x30EB\x30BF\x3092\x6709\x52B9\x5316";
    ja[L"SCHEDULE_START"] = L"\x958B\x59CB\x6642\x523B";
    ja[L"SCHEDULE_END"] = L"\x7D42\x4E86\x6642\x523B";
    ja[L"POLICY_ENTERPRISE"] = L"\x30A8\x30F3\x30BF\x30FC\x30D7\x30E9\x30A4\x30BA \x30DD\x30EA\x30B7\x30FC & \x30D0\x30C3\x30AF\x30A2\x30C3\x30D7";
    ja[L"POLICY_DESC"] = L"AES-256 \x6697\x53F7\x5316\x3055\x308C\x305F\x8A2D\x5B9A\x3092\x30A8\x30AF\x30B9\x30DD\x30FC\x30C8/\x30A4\x30F3\x30DD\x30FC\x30C8\x3057\x307E\x3059\x3002";
    ja[L"POLICY_EXPORT"] = L"\x30DD\x30EA\x30B7\x30FC\x3092\x30A8\x30AF\x30B9\x30DD\x30FC\x30C8 (.suraksha)";
    ja[L"POLICY_IMPORT"] = L"\x30DD\x30EA\x30B7\x30FC\x3092\x30A4\x30F3\x30DD\x30FC\x30C8 (.suraksha)";
    ja[L"LANGUAGE_SECTION"] = L"\x8868\x793A\x8A00\x8A9E";
    ja[L"STATUS_PROTECTED"] = L"\x4FDD\x8B77\x4E2D";
    ja[L"STATUS_PAUSED"] = L"\x4E00\x6642\x505C\x6B62";
    ja[L"ADD_APP"] = L"\x30A2\x30D7\x30EA\x3092\x8FFD\x52A0...";
    ja[L"REMOVE_APP"] = L"\x524A\x9664";
}
