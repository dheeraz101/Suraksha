#pragma once

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
// Suraksha Version & Codename Definition
// Automatically maintained by scripts/Set-Version.ps1
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•

#define SURAKSHA_VERSION_MAJOR      2
#define SURAKSHA_VERSION_MINOR      0
#define SURAKSHA_VERSION_PATCH      0
#define SURAKSHA_VERSION_BUILD      0

#define SURAKSHA_VERSION_STRING     L"2.0.0"
#define SURAKSHA_CODENAME           L"Kavach"
#define SURAKSHA_IS_BETA            false

#if SURAKSHA_IS_BETA
    #define SURAKSHA_DISPLAY_VERSION L"2.0.0-beta (Kavach)"
#else
    #define SURAKSHA_DISPLAY_VERSION L"2.0.0 (Kavach)"
#endif
