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
    en[L"ADD_APP"] = L"+ Add Application...";
    en[L"REMOVE_APP"] = L"Remove App";

    // ═══════════════════════════════════════════════════════════════
    // Hindi (हिंदी)
    // ═══════════════════════════════════════════════════════════════
    auto& hi = m_tables[Language::Hindi];
    hi[L"TAB_APPLOCKER"] = L"ऐप लॉकर";
    hi[L"TAB_SECURITY"] = L"सुरक्षा एवं प्रमाणीकरण";
    hi[L"TAB_UPDATES"] = L"सॉफ़्टवेयर अपडेट";
    hi[L"TAB_AUDIT"] = L"ऑडिट लॉग";
    hi[L"TAB_ABOUT"] = L"सुरक्षा के बारे में";
    hi[L"PROTECTED_APPS"] = L"सुरक्षित एप्लिकेशन";
    hi[L"SECURITY_DEFENSE"] = L"सुरक्षा एवं प्रमाणीकरण";
    hi[L"SCHEDULE_PROTECTION"] = L"निर्धारित सुरक्षा (सक्रिय घंटे)";
    hi[L"SCHEDULE_DESC"] = L"केवल निर्धारित कार्य घंटों के दौरान ऐप लॉक लागू करें।";
    hi[L"SCHEDULE_ENABLE"] = L"सक्रिय घंटे फ़िल्टर सक्षम करें";
    hi[L"SCHEDULE_START"] = L"प्रारंभ समय";
    hi[L"SCHEDULE_END"] = L"समाप्ति समय";
    hi[L"POLICY_ENTERPRISE"] = L"एंटरप्राइज़ नीति एवं बैकअप";
    hi[L"POLICY_DESC"] = L"AES-256 एन्क्रिप्टेड सुरक्षा कॉन्फ़िगरेशन आयात/निर्यात करें।";
    hi[L"POLICY_EXPORT"] = L"नीति निर्यात करें (.suraksha)";
    hi[L"POLICY_IMPORT"] = L"नीति आयात करें (.suraksha)";
    hi[L"LANGUAGE_SECTION"] = L"प्रदर्शन भाषा";
    hi[L"STATUS_PROTECTED"] = L"सुरक्षित";
    hi[L"STATUS_PAUSED"] = L"रोका गया";
    hi[L"ADD_APP"] = L"+ ऐप जोड़ें...";
    hi[L"REMOVE_APP"] = L"ऐप हटाएं";

    // ═══════════════════════════════════════════════════════════════
    // Spanish (Español)
    // ═══════════════════════════════════════════════════════════════
    auto& es = m_tables[Language::Spanish];
    es[L"TAB_APPLOCKER"] = L"Bloqueador";
    es[L"TAB_SECURITY"] = L"Seguridad";
    es[L"TAB_UPDATES"] = L"Actualizaciones";
    es[L"TAB_AUDIT"] = L"Auditoría";
    es[L"TAB_ABOUT"] = L"Acerca de";
    es[L"PROTECTED_APPS"] = L"Aplicaciones Protegidas";
    es[L"SECURITY_DEFENSE"] = L"Seguridad y Autenticación";
    es[L"SCHEDULE_PROTECTION"] = L"Protección Programada (Horario)";
    es[L"SCHEDULE_DESC"] = L"Limita el bloqueo a horas de trabajo específicas.";
    es[L"SCHEDULE_ENABLE"] = L"Activar Filtro de Horas Activas";
    es[L"SCHEDULE_START"] = L"Hora Inicio";
    es[L"SCHEDULE_END"] = L"Hora Fin";
    es[L"POLICY_ENTERPRISE"] = L"Políticas Empresariales y Respaldo";
    es[L"POLICY_DESC"] = L"Exporta e importa configuraciones cifradas en AES-256.";
    es[L"POLICY_EXPORT"] = L"Exportar Política (.suraksha)";
    es[L"POLICY_IMPORT"] = L"Importar Política (.suraksha)";
    es[L"LANGUAGE_SECTION"] = L"Idioma";
    es[L"STATUS_PROTECTED"] = L"Protegido";
    es[L"STATUS_PAUSED"] = L"Pausado";
    es[L"ADD_APP"] = L"+ Añadir Aplicación...";
    es[L"REMOVE_APP"] = L"Eliminar";

    // ═══════════════════════════════════════════════════════════════
    // German (Deutsch)
    // ═══════════════════════════════════════════════════════════════
    auto& de = m_tables[Language::German];
    de[L"TAB_APPLOCKER"] = L"App-Sperre";
    de[L"TAB_SECURITY"] = L"Sicherheit";
    de[L"TAB_UPDATES"] = L"Software-Update";
    de[L"TAB_AUDIT"] = L"Audit-Protokoll";
    de[L"TAB_ABOUT"] = L"Über Suraksha";
    de[L"PROTECTED_APPS"] = L"Geschützte Anwendungen";
    de[L"SECURITY_DEFENSE"] = L"Sicherheit & Authentifizierung";
    de[L"SCHEDULE_PROTECTION"] = L"Geplanter Schutz (Aktive Zeiten)";
    de[L"SCHEDULE_DESC"] = L"Beschränke die App-Sperre auf bestimmte Arbeitszeiten.";
    de[L"SCHEDULE_ENABLE"] = L"Zeitfilter für aktive Stunden aktivieren";
    de[L"SCHEDULE_START"] = L"Startzeit";
    de[L"SCHEDULE_END"] = L"Endzeit";
    de[L"POLICY_ENTERPRISE"] = L"Unternehmensrichtlinie & Backup";
    de[L"POLICY_DESC"] = L"AES-256-verschlüsselte Konfigurationen exportieren/importieren.";
    de[L"POLICY_EXPORT"] = L"Richtlinie exportieren (.suraksha)";
    de[L"POLICY_IMPORT"] = L"Richtlinie importieren (.suraksha)";
    de[L"LANGUAGE_SECTION"] = L"Sprache";
    de[L"STATUS_PROTECTED"] = L"Geschützt";
    de[L"STATUS_PAUSED"] = L"Pausiert";
    de[L"ADD_APP"] = L"+ App hinzufügen...";
    de[L"REMOVE_APP"] = L"Entfernen";

    // ═══════════════════════════════════════════════════════════════
    // French (Français)
    // ═══════════════════════════════════════════════════════════════
    auto& fr = m_tables[Language::French];
    fr[L"TAB_APPLOCKER"] = L"Verrou d'app";
    fr[L"TAB_SECURITY"] = L"Sécurité";
    fr[L"TAB_UPDATES"] = L"Mise à jour";
    fr[L"TAB_AUDIT"] = L"Journaux";
    fr[L"TAB_ABOUT"] = L"À propos";
    fr[L"PROTECTED_APPS"] = L"Applications Protégées";
    fr[L"SECURITY_DEFENSE"] = L"Sécurité & Authentification";
    fr[L"SCHEDULE_PROTECTION"] = L"Protection Programmée (Heures Actives)";
    fr[L"SCHEDULE_DESC"] = L"Limiter le verrouillage à des horaires de travail précis.";
    fr[L"SCHEDULE_ENABLE"] = L"Activer le filtre d'heures de travail";
    fr[L"SCHEDULE_START"] = L"Heure Début";
    fr[L"SCHEDULE_END"] = L"Heure Fin";
    fr[L"POLICY_ENTERPRISE"] = L"Politique Entreprise & Sauvegarde";
    fr[L"POLICY_DESC"] = L"Exporter et importer des configurations chiffrées en AES-256.";
    fr[L"POLICY_EXPORT"] = L"Exporter la politique (.suraksha)";
    fr[L"POLICY_IMPORT"] = L"Importer la politique (.suraksha)";
    fr[L"LANGUAGE_SECTION"] = L"Langue";
    fr[L"STATUS_PROTECTED"] = L"Protégé";
    fr[L"STATUS_PAUSED"] = L"En Pause";
    fr[L"ADD_APP"] = L"+ Ajouter une appli...";
    fr[L"REMOVE_APP"] = L"Supprimer";

    // ═══════════════════════════════════════════════════════════════
    // Japanese (日本語)
    // ═══════════════════════════════════════════════════════════════
    auto& ja = m_tables[Language::Japanese];
    ja[L"TAB_APPLOCKER"] = L"アプリロック";
    ja[L"TAB_SECURITY"] = L"セキュリティ";
    ja[L"TAB_UPDATES"] = L"ソフトウェア更新";
    ja[L"TAB_AUDIT"] = L"監査ログ";
    ja[L"TAB_ABOUT"] = L"Suraksha について";
    ja[L"PROTECTED_APPS"] = L"保護されたアプリ";
    ja[L"SECURITY_DEFENSE"] = L"セキュリティと認証";
    ja[L"SCHEDULE_PROTECTION"] = L"スケジュール保護（勤務時間）";
    ja[L"SCHEDULE_DESC"] = L"指定した時間帯のみアプリロックを適用します。";
    ja[L"SCHEDULE_ENABLE"] = L"時間帯フィルタを有効化";
    ja[L"SCHEDULE_START"] = L"開始時刻";
    ja[L"SCHEDULE_END"] = L"終了時刻";
    ja[L"POLICY_ENTERPRISE"] = L"エンタープライズ ポリシー & バックアップ";
    ja[L"POLICY_DESC"] = L"AES-256 暗号化された設定をエクスポート/インポートします。";
    ja[L"POLICY_EXPORT"] = L"ポリシーをエクスポート (.suraksha)";
    ja[L"POLICY_IMPORT"] = L"ポリシーをインポート (.suraksha)";
    ja[L"LANGUAGE_SECTION"] = L"表示言語";
    ja[L"STATUS_PROTECTED"] = L"保護中";
    ja[L"STATUS_PAUSED"] = L"一時停止";
    ja[L"ADD_APP"] = L"+ アプリを追加...";
    ja[L"REMOVE_APP"] = L"削除";
}
