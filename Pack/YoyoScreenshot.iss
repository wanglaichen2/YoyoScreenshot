; 悠悠截图 — Inno Setup 打包脚本
; 编译器: Inno Setup 5.x / 6.x / 7.x
; 用法:
;   1) 先 ./build_vs.sh Release x64  或  ./build_mingw.sh Release x64
;   2) 运行本目录 pack.bat（从 Exec 同步文件并调用 ISCC）
;   或手动: ISCC.exe YoyoScreenshot.iss
;
; 产物: Output\YoyoScreenshot_<version>_x64_Setup.exe
; 版本可在命令行覆盖: ISCC.exe /DMyAppVersion=1.2.3 YoyoScreenshot.iss

#ifndef MyAppVersion
#define MyAppVersion "1.0.0"
#endif
#ifndef MyAppName
#define MyAppName      "YoyoScreenshot"
#endif
#define MyAppNameCn    "悠悠截图"
#define MyAppPublisher "PinZhun"
#define MyAppExeName   "YoyoScreenshot.exe"

[Setup]
AppId={{C4D8E20A-7B31-4F9E-A1D6-YOYOSHOT0001}
AppName={#MyAppNameCn}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppVerName={#MyAppNameCn} {#MyAppVersion}
DefaultDirName={autopf}\{#MyAppNameCn}
DefaultGroupName={#MyAppNameCn}
DisableProgramGroupPage=yes
UninstallDisplayIcon={app}\{#MyAppExeName}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
OutputDir=Output
OutputBaseFilename=YoyoScreenshot_{#MyAppVersion}_x64_Setup
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
SetupIconFile=icon.ico

[Languages]
; 默认英文向导（各版 Inno 均自带）。若已装简体语言包可改为:
; Name: "chinesesimplified"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "icon.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "resources\*"; DestDir: "{app}\resources"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*.pdb"
; MinGW 运行时（静态链接时仍可能需要）
Source: "libgcc_s_seh-1.dll"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "libstdc++-6.dll"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "libwinpthread-1.dll"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "libatomic-1.dll"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "libgomp-1.dll"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "libssp-0.dll"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
; 若改为动态链接 UI 库，把 dll 放进本目录后取消下一行注释
; Source: "libXCGUI.dll"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
; Source: "XCGUI.dll"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist

[Icons]
Name: "{group}\{#MyAppNameCn}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; IconFilename: "{app}\icon.ico"
Name: "{group}\卸载 {#MyAppNameCn}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppNameCn}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; IconFilename: "{app}\icon.ico"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "启动 {#MyAppNameCn}"; Flags: nowait postinstall skipifsilent shellexec

[UninstallDelete]
Type: files; Name: "{app}\*.log"
Type: files; Name: "{app}\xcgui_debug.txt"
Type: filesandordirs; Name: "{app}\dumps"
Type: filesandordirs; Name: "{app}\shots"
Type: dirifempty; Name: "{app}"
