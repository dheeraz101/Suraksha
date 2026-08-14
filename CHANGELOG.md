# Changelog — Suraksha

All notable changes to **Suraksha — Privacy & Security** will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [2.0.0] - 2026-08-14

### Added
- **100% Direct-Canvas GDI+ UI Engine**: Completely eliminated legacy Win32 child HWND controls, physically erasing all white corner bounding box halos, rectangular outlines, and background bleeding artifacts.
- **Brand Identity & Iconography**: Embedded official white padlock with vibrant green 'S' multi-resolution ICO icon (`Suraksha.ico`, 16x16 to 256x256) and vector GDI+ rendering (`UIComponents::DrawAppLogo`).
- **About Suraksha Modal**: Added full About sheet displaying **An YABP Initiative** (*Yet Another Boring Project*), developer credits for **Dheeraz**, and embedded GPLv3 license text.
- **Enterprise Security Audit Logging**: Automatic timestamped audit logging to `%APPDATA%\Suraksha\logs\audit.log` tracking interceptions, logins, and setting changes.
- **Anti-Brute-Force Rate Limiting**: 30-second automated lockout after 3 consecutive failed passcode attempts.
- **Global System Hotkeys**:
  - `Ctrl + Alt + L`: Instant Lock All protected sessions across Windows.
  - `Ctrl + Alt + S`: Toggle Suraksha control panel visibility.
- **Dynamic System Tray Tooltip**: Live metrics on hover (`"Suraksha — 3 Apps Protected | Status: Active"`).
- **GitHub Open-Source Suite**: Official `LICENSE` (GPLv3), `README.md`, `CONTRIBUTING.md`, `TERMS_OF_USE.md`, `SECURITY.md`, `CODE_OF_CONDUCT.md`, `.gitignore`, and Issue/PR templates.

### Changed
- **Windows Security Auth**: Integrated official Microsoft CredUI dialog (`CredUIPromptForWindowsCredentialsW`) for native Windows Hello/PIN authentication.
- **Startup Performance**: Handled `WM_ERASEBKGND` and `BLACK_BRUSH` background to achieve 0% white flash on cold startup.
- **Preset Buttons**: Aligned and widened preset buttons (`+ Notepad`, `+ Chrome`, `+ Terminal`, `+ Calculator`) to prevent multi-line text wrapping.
- **Windows 11 DWM Border**: Native dark slate border (`RGB(36, 36, 40)`) with extended DWM frame margins.

---

## [1.0.0] - 2026-08-01

### Added
- Initial release of Suraksha Windows App Locker.
- Sub-50ms process monitoring and window hiding engine.
- DPAPI hardware-encrypted master passcode storage.
- Auto-start with Windows registry integration (`HKCU\Software\Microsoft\Windows\CurrentVersion\Run`).
