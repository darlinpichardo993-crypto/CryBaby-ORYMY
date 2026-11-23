; Inno Setup 6 script for CRY BABY AUTO WAH (Windows x64).
; Build locally (after compiling in Release):
;   "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" Installer\CryBabyAutoWah.iss
; Optional overrides:  /DMyAppVersion=1.0.1  /DBuildDir=C:\path\to\build
; The GitHub Actions workflow runs this script automatically.

#ifndef MyAppVersion
  #define MyAppVersion "1.0.0"
#endif
#ifndef BuildDir
  #define BuildDir "..\build"
#endif

#define MyAppName      "CRY BABY AUTO WAH"
#define MyAppPublisher "ORYMY / YMY Records"
#define Artefacts      BuildDir + "\CryBabyAutoWah_artefacts\Release"

[Setup]
AppId={{6C5D2C0B-3D8B-4E55-9B53-7B8F0F4C1A21}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\ORYMY\{#MyAppName}
DefaultGroupName=ORYMY
DisableProgramGroupPage=yes
OutputDir=..\Builds\Installer
OutputBaseFilename=CryBabyAutoWah-Setup-{#MyAppVersion}
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern
UninstallDisplayName={#MyAppName}

[Types]
Name: "full";   Description: "VST3 plugin + Standalone app"
Name: "vst3";   Description: "VST3 plugin only"
Name: "custom"; Description: "Custom"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plugin (installed in Common Files\VST3)"; Types: full vst3 custom; Flags: fixed
Name: "standalone"; Description: "Standalone application"; Types: full custom

[Files]
; VST3 bundle folder -> C:\Program Files\Common Files\VST3\CRY BABY AUTO WAH.vst3
Source: "{#Artefacts}\VST3\{#MyAppName}.vst3\*"; DestDir: "{commoncf64}\VST3\{#MyAppName}.vst3"; Components: vst3; Flags: recursesubdirs createallsubdirs ignoreversion
; Standalone (only present when built with CBAW_BUILD_STANDALONE=ON)
Source: "{#Artefacts}\Standalone\{#MyAppName}.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion skipifsourcedoesntexist

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppName}.exe"; Components: standalone
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\{#MyAppName}.exe"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent unchecked; Components: standalone
