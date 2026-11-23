; Signal Forge Audio Labs — Green Onions Windows Installer (Inno Setup)
; Installs Standalone (.exe), VST3 (.vst3), Start Menu shortcuts, and clean Uninstaller.

#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif

#define AppName "Green Onions"
#define AppPublisher "Signal Forge Audio Labs"
#define AppURL "https://github.com/SignalForgeLabs/Green-Onions-SFAL-B3"
#define AppExeName "Green Onions.exe"

[Setup]
AppId={{8F5013BE-90A8-4B5E-8783-D3760C18A567}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}
DefaultDirName={autopf}\{#AppPublisher}\{#AppName}
DefaultGroupName={#AppPublisher}\{#AppName}
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
OutputDir=..\..\dist-installer
OutputBaseFilename=Green-Onions-{#AppVersion}-Setup-Windows-x64
UninstallDisplayName={#AppName} ({#AppPublisher})
UninstallDisplayIcon={app}\{#AppExeName}
PrivilegesRequired=admin

[Types]
Name: "full"; Description: "Full Installation (Standalone + VST3 Plugin)"
Name: "custom"; Description: "Custom Installation"; Flags: iscustom

[Components]
Name: "standalone"; Description: "Green Onions Standalone Application"; Types: full custom
Name: "vst3"; Description: "Green Onions VST3 Plugin (64-bit)"; Types: full custom

[Tasks]
Name: "desktopicon"; Description: "Create a &Desktop shortcut for Green Onions Standalone"; GroupDescription: "Additional shortcuts:"; Components: standalone

[Files]
; Standalone Executable
Source: "..\..\build\GreenOnions_artefacts\Release\Standalone\Green Onions.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion

; VST3 Bundle -> C:\Program Files\Common Files\VST3\Green Onions.vst3
Source: "..\..\build\GreenOnions_artefacts\Release\VST3\Green Onions.vst3\*"; DestDir: "{commoncf64}\VST3\Green Onions.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs

; Overview documentation
Source: "..\..\GREEN_ONIONS_OVERVIEW.txt"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExeName}"; Components: standalone
Name: "{group}\Uninstall {#AppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExeName}"; Tasks: desktopicon; Components: standalone

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Green Onions.vst3"
Type: filesandordirs; Name: "{app}"

[Run]
Filename: "{app}\{#AppExeName}"; Description: "Launch {#AppName}"; Flags: nowait postinstall skipifsilent; Components: standalone
