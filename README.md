# Suraksha — Privacy & Security (v2.0)

<p align="center">
  <img src="logo.png" width="128" alt="Suraksha Logo" />
  <br>
  <b>A Lightweight, Native, Ultra-Fast Windows Application Locker inspired by macOS Human Interface Guidelines (HIG).</b>
  <br>
  <i>An <b>YABP</b> Initiative — Yet Another Boring Project</i>
</p>

<p align="center">
  <a href="https://yabp.netlify.app/"><img src="https://img.shields.io/badge/YABP-Initiative-0A84FF?style=for-the-badge" alt="YABP Initiative" /></a>
  <a href="https://dheeraz.dpdns.org/"><img src="https://img.shields.io/badge/Developer-Dheeraz-34C759?style=for-the-badge" alt="Developer Dheeraz" /></a>
  <a href="https://github.com/dheeraz101"><img src="https://img.shields.io/badge/GitHub-dheeraz101-181717?style=for-the-badge&logo=github" alt="GitHub" /></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-GPLv3-FF453A?style=for-the-badge" alt="GPLv3 License" /></a>
  <img src="https://img.shields.io/badge/Platform-Windows%2010%20%7C%2011%20(x64)-0078D4?style=for-the-badge&logo=windows" alt="Windows 10/11 x64" />
</p>

---

## 🌟 Key Features

- **⚡ Sub-50ms Process Interception Engine**: Instant detection, multi-threaded window hiding (`ShowWindow(SW_HIDE)`), and process suspension (`NtSuspendProcess`) before target content is rendered.
- **🎨 100% Direct-Canvas GDI+ UI**: Built with zero child HWND controls, physically eliminating all white corner bounding box halos, rectangular outlines, and background bleeding.
- **🔐 Dual Authentication System**:
  - **Windows Security (PIN / Hello / Password)** via official Microsoft CredUI (`CredUIPromptForWindowsCredentialsW`).
  - **Master Passcode Protection** with hardware-bound DPAPI hashing.
- **🛡️ Enterprise Anti-Brute-Force Rate Limiting**: Automated 30-second security lockout after 3 consecutive failed attempts.
- **📝 Audit Logging Engine**: Writes timestamped security logs to `%APPDATA%\Suraksha\logs\audit.log`.
- **⌨️ Global System Hotkeys**: Quick accessibility anywhere in Windows.
- **🚀 System Tray Integration**: Minimizes silently to system tray with dynamic metric updates.

---

## ⌨️ Global System Hotkeys

Suraksha includes system-wide hotkeys active from anywhere in Windows:

| Shortcut | Action | Description |
| :--- | :--- | :--- |
| **`Ctrl + Alt + L`** | **Instant Lock All** | Immediately locks and suspends all running protected application sessions. |
| **`Ctrl + Alt + S`** | **Toggle Control Panel** | Instantly shows or hides the Suraksha main window. |

---

## 🛡️ Security Architecture & Threat Mitigation

| Security Layer | Technology Used | Protection Benefit |
| :--- | :--- | :--- |
| **Credential Authentication** | Microsoft CredUI (`CredUIPromptForWindowsCredentialsW`) | Supports Windows Hello Face, Fingerprint, System PIN, and Domain Passwords. |
| **Passcode Encryption** | Windows Data Protection API (`CryptProtectData`) | Master passcodes are hardware-bound and user-account encrypted on disk. |
| **Brute-Force Mitigation** | Exponential Cooldown Lockout | Enforces a 30-second cooldown after 3 consecutive invalid attempts. |
| **Audit Trails** | Thread-Safe Local Audit Engine | Timestamped events logged to `%APPDATA%\Suraksha\logs\audit.log`. |
| **Single-Instance Protection** | Windows Kernel Mutex (`CreateMutexW`) | Prevents duplicate instances or unauthorized concurrent hooks. |

---

## 💻 System Requirements & Performance

- **Operating System**: Windows 10 (64-bit) or Windows 11 (64-bit) (Build 19041 or newer).
- **RAM Footprint**: **< 4 MB RAM** (24/7 background operation).
- **CPU Footprint**: **0.0% CPU** on idle.
- **Architecture**: 100% Native C++20 (Zero .NET, Electron, or Web runtime dependencies).

---

## 🛠️ Building from Source

### Prerequisites
- **Visual Studio 2022 / 2026** with **Desktop development with C++** workload (`/std:c++20`).
- Windows 10/11 SDK (`10.0.22621.0` or newer).

### Build Steps
1. Clone the repository:
   ```bash
   git clone https://github.com/dheeraz101/Suraksha.git
   cd Suraksha
   ```
2. Build via MSBuild:
   ```cmd
   MSBuild Suraksha.vcxproj /p:Configuration=Release /p:Platform=x64
   ```
3. Run the compiled binary: `x64\Release\Suraksha.exe`

---

## 📜 License & Credits

- **Developer**: [Dheeraz](https://dheeraz.dpdns.org/) ([GitHub](https://github.com/dheeraz101))
- **Initiative**: An **YABP** Initiative — *Yet Another Boring Project* ([yabp.netlify.app](https://yabp.netlify.app/))
- **License**: Released under the **GNU General Public License v3.0 (GPLv3)**. See [LICENSE](LICENSE) for full details.
