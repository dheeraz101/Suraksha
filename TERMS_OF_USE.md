# Terms of Use & Privacy Policy - Suraksha

**Effective Date**: August 14, 2026  
**Developer**: [Dheeraz](https://dheeraz.dpdns.org/) ([GitHub](https://github.com/dheeraz101))  
**Initiative**: An **YABP** Initiative - *Yet Another Boring Project* ([yabp.netlify.app](https://yabp.netlify.app/))  
**License**: GNU General Public License v3.0 (GPLv3)

---

## 1. Terms of Use
By installing, downloading, or using **Suraksha**, you agree to be bound by these terms:
- Suraksha is provided **"AS IS"** without warranties of any kind, express or implied, as detailed in Section 15 of the GPLv3 license.
- You are granted full permission to run, modify, and distribute Suraksha in accordance with the GNU General Public License v3.0.

---

## 2. Privacy Policy (Zero Telemetry Guarantee)
- **100% Local Processing**: Suraksha processes all app locking, process scans, and authentication checks locally on your Windows device.
- **Zero Telemetry / Data Collection**: Suraksha does **NOT** collect, transmit, or store any personal data, passwords, file paths, or telemetry to external servers.
- **Encrypted Local Credentials**: Passcode hashes and settings are stored locally in `%APPDATA%\Suraksha\settings.json` and encrypted using Windows DPAPI (`CryptProtectData`).
- **Local Audit Logs**: Security audit logs are saved locally in `%APPDATA%\Suraksha\logs\audit.log` and never leave your machine.
