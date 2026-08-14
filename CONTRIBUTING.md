# Contributing to Suraksha

Thank you for your interest in contributing to **Suraksha - Privacy & Security**!

Suraksha is an open-source project licensed under the **GNU General Public License v3.0 (GPLv3)** and is part of **An YABP Initiative** (*Yet Another Boring Project*).

---

## How Can I Contribute?

### 1. Reporting Bugs
- Search existing issues to avoid duplicates.
- Provide step-by-step reproduction instructions, Windows OS version, and error logs (`%APPDATA%\Suraksha\logs\audit.log`).

### 2. Suggesting Enhancements
- Open a GitHub issue with the `enhancement` tag.
- Explain clearly why the feature would benefit users while adhering to native Windows C++ performance guidelines.

### 3. Pull Requests
1. Fork the repository: `https://github.com/dheeraz101/Suraksha`
2. Create a feature branch: `git checkout -b feature/amazing-feature`
3. Commit your changes: `git commit -m 'Add amazing feature'`
4. Push to your branch: `git push origin feature/amazing-feature`
5. Open a Pull Request on GitHub.

---

## Coding Standards

- **Language**: C++20 standard (`/std:c++20`).
- **UI Architecture**: Single-canvas Direct GDI+ rendering (zero child HWND controls for non-modal elements).
- **Naming**: UpperCamelCase for class names and functions (`SecurityManager`, `VerifyCustomPin`).
- **Performance**: Zero external dependencies outside standard Windows APIs (`user32.lib`, `gdi32.lib`, `gdiplus.lib`, `credui.lib`, `dwmapi.lib`).
