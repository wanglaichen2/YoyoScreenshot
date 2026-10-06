# 悠悠截图

Windows 截图工具：框选、全屏、标注与历史管理。

## 功能

- 框选截图（画笔 / 矩形 / 箭头等标注）
- 全屏截图
- 历史缩略图：单击选中、双击编辑、右键预览/复制/贴图/删除
- 托盘常驻；关闭窗口最小化到托盘
- 快捷键：`Ctrl+Alt+I` 框选，`Ctrl+Alt+Shift+I` 全屏
- 文件写入 `exe\shots\`

## 目录

```
app/           入口、主窗、PCH
core/          截图与框选标注
platform/      托盘、MiniDump
bridge/        UI 接口声明
SDK/inc/       UI 头文件
resources/     图标与清单
libs/          预编译静态库（MSVC / MinGW）
Pack/          安装包脚本
```

## 编译

MSVC：

```bash
./build_vs.sh Release x64
```

或打开 `YoyoScreenshot.sln`。

MinGW（CMake；没有 cmake 时走 `Makefile`）：

```bash
./build_mingw.sh Release x64
# 或: mingw32-make -f Makefile CFG=Release
```

产物：`Exec/Release/x64/YoyoScreenshot/YoyoScreenshot.exe`

## 发布 GitHub Release

推送版本 tag 后，Actions 会编译并上传 `Pack` 打出的安装包：

```bash
git tag v1.0.0
git push origin v1.0.0
```

也可在 Actions 里手动跑 **Release** 工作流，填写版本号。

Release 附件：`YoyoScreenshot_<版本>_x64_Setup.exe`（例如 `YoyoScreenshot_1.0.0_x64_Setup.exe`）。

本地打包：先编译，再 `Pack\pack.bat 1.0.0`。
