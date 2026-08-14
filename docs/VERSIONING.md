# Suraksha Release & Versioning Documentation

This document outlines the **Semantic Versioning**, **Apple-Style Build Codes**, and **Automated Release Pipeline** for the Suraksha project.

---

## 📌 1. Version Semantics & Build Number Format

Suraksha follows standard [Semantic Versioning (SemVer 2.0.0)](https://semver.org/) paired with **Apple-style compact build codes**:

$$\textbf{v[Major].[Minor].[Patch][-beta]} \quad \mathbf{(Build \ [YY][Channel][MMDD][Iteration])}$$

### Structure Breakdown:

| Component | Format | Description | Example |
| :--- | :--- | :--- | :--- |
| **Major** | `X.0.0` | Architectural overhauls, major redesigns | `2.0.0`, `3.0.0` |
| **Minor** | `X.Y.0` | New feature additions, UI upgrades | `2.1.0`, `2.2.0` |
| **Patch** | `X.Y.Z` | Security updates, bug fixes | `2.0.1`, `2.0.2` |
| **Channel** | `B` / `S` | **`B`** for Beta Channel, **`S`** for Stable Release | `26B...`, `26S...` |
| **Date Code** | `MMDD` | 2-digit Month + 2-digit Day | `0815` (August 15) |
| **Iteration** | `a, b, c...` | Automatically increments for same-day builds | `26B0815a` $\rightarrow$ `26B0815b` |

### Examples in Action:
- **1st Beta today**: `2.0.1-beta (26B0815a)`
- **2nd Beta today** *(after a quick bug fix)*: `2.0.1-beta (26B0815b)`
- **3rd Beta today** *(after another hotfix)*: `2.0.1-beta (26B0815c)`
- **Official Stable Release**: `2.0.1 (26S0815)`

---

## 🛠️ 2. Version Management Script (`scripts/Set-Version.ps1`)

The script [`scripts/Set-Version.ps1`](file:///d:/Windows-Project/Vs%20studio/Suraksha/scripts/Set-Version.ps1) is the single source of truth for versions across the entire codebase. It automatically synchronizes:
1. `Version.h` (C++ source header)
2. `SurakshaSetup.iss` (Inno Setup Installer script)
3. `packaging/AppxManifest.xml` (MSIX Package manifest)
4. `README.md` (Project documentation)

### Command Usage:

#### 🟢 Advance or Create a **Beta** Build:
```powershell
.\scripts\Set-Version.ps1 -beta
```
* Automatically detects the previous build tag.
* If run on the same date, increments the letter suffix: `26B0815a` $\rightarrow$ `26B0815b`.
* Sets beta flags and updates all project files.

#### 🔵 Graduate to an Official **Stable** Release:
```powershell
.\scripts\Set-Version.ps1 -release
```
* Converts active beta builds to stable production release.
* Generates the stable build code: `26S0815`.
* Sets stable production flags and removes pre-release indicators.

#### 🔢 Set an Explicit Custom Version:
```powershell
.\scripts\Set-Version.ps1 -beta -Version "2.1.0"
.\scripts\Set-Version.ps1 -release -Version "2.1.0"
```

---

## 📦 3. Local Release Builder (`scripts/Build-LocalRelease.ps1`)

To test and verify all 4 release deliverables locally on your machine without pushing to GitHub, run:

```powershell
.\scripts\Build-LocalRelease.ps1
```

### Generated Deliverables in `dist/`:
1. **`Suraksha.exe`**: 64-bit standalone portable executable (Signed).
2. **`SurakshaSetup-vX.Y.Z-x64.exe`**: Inno Setup Installation Wizard (Signed).
3. **`Suraksha-vX.Y.Z-x64.msix`**: Modern Windows Native App Package (Signed).
4. **`SHA256SUMS.txt`**: Cryptographic hash manifest for integrity verification.

---

## 🚀 4. GitHub Actions CI/CD Release Channels

The automated release pipeline [`.github/workflows/release.yml`](file:///d:/Windows-Project/Vs%20studio/Suraksha/.github/workflows/release.yml) automatically detects which channel to publish to:

| Git Action | Target Channel | GitHub Release Type | Release Title |
| :--- | :--- | :--- | :--- |
| Push to **`beta`** branch | **Beta Preview** | Marked as **Pre-release** | `Suraksha vX.Y.Z-beta [Beta] - Build: 26B0815a` |
| Push to **`release`** branch | **Production** | Official **Stable Release** | `Suraksha vX.Y.Z [Stable] - Build: 26S0815` |
| Manual Dispatch | Selected | Follows selection | Matches selection |

---

## 📋 5. Standard Release Workflow (Step-by-Step)

### Publishing a Beta:
```bash
# 1. Checkout or create beta branch
git checkout -b beta

# 2. Bump build code
powershell -File .\scripts\Set-Version.ps1 -beta

# 3. Commit and push to GitHub
git commit -am "chore(release): bump beta build"
git push origin beta
```

### Publishing a Stable Release:
```bash
# 1. Checkout release branch
git checkout release
git merge beta

# 2. Graduate to stable
powershell -File .\scripts\Set-Version.ps1 -release

# 3. Commit and push to GitHub
git commit -am "chore(release): publish stable release"
git push origin release
```
