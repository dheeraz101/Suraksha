; =====================================================================
; Suraksha - Windows Application Locker & Privacy System
; Inno Setup Script Configuration
; =====================================================================

#define MyAppName "Suraksha"
#define MyAppVersion "2.0.0"
#define MyAppPublisher "Dheeraz (YABP Initiative)"
#define MyAppURL "https://yabp.netlify.app/"
#define MyAppExeName "Suraksha.exe"
#define MyAppId "{{C5D8B632-4416-47F1-9BD9-C1635D016B61}"

[Setup]
AppId={#MyAppId}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} v{#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL=https://github.com/dheeraz101/Suraksha/issues
AppUpdatesURL=https://github.com/dheeraz101/Suraksha/releases
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
LicenseFile=LICENSE
OutputDir=Output
OutputBaseFilename=SurakshaSetup-v{#MyAppVersion}-x64
SetupIconFile=Suraksha.ico
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
CloseApplications=yes
CloseApplicationsFilter={#MyAppExeName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
UninstallDisplayIcon={app}\{#MyAppExeName}
VersionInfoVersion={#MyAppVersion}
VersionInfoCompany={#MyAppPublisher}
VersionInfoDescription=Suraksha Windows Application Locker
VersionInfoProductName=Suraksha

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "startupicon"; Description: "Launch Suraksha automatically on Windows startup"; GroupDescription: "Startup:"; Flags: unchecked

[Files]
Source: "x64\Release\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "Suraksha.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "README.md"; DestDir: "{app}"; Flags: isreadme
Source: "LICENSE"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\Suraksha.ico"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\Suraksha.ico"; Tasks: desktopicon
Name: "{autostartup}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: startupicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent



