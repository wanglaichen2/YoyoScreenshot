Microsoft Store 上架资料目录
============================

本目录集中存放「悠悠截图」上架 Microsoft Store 所需文案与徽标。

文件说明
--------
隐私政策.txt          隐私策略全文（可粘贴到 Partner Center「隐私策略文本」）
商店文案.txt          含：1说明 / 2此版本新增功能 / 3产品功能
Logos\                商店与 MSIX 用徽标 PNG

Logos 建议用法
--------------
StoreLogo_1080x1080.png   商店徽标 1:1（1080×1080）
StoreLogo_720x1080.png    商店徽标竖版（720×1080）
StoreLogo_300x300.png     Partner Center 商店列表 / 应用图标（常用）
StoreLogo_71x71.png       商店徽标小尺寸（71×71）
StoreLogo_50x50.png       包清单 StoreLogo
Square44x44Logo.png       小磁贴 / 任务栏
Square150x150Logo.png     中磁贴
Square310x310Logo.png     大磁贴
Wide310x150Logo.png       宽磁贴
SplashScreen_620x300.png  启动画面
StoreLogo_master.jpg      设计原稿

注意：上传商店前请在 Partner Center 核对图标清晰度；如对生成图不满意，可用原稿自行微调后再导出各尺寸。

重新生成图标（圆角外框透明，并同步程序 icon.ico / MSIX Assets）
--------------------------------------------------------------
python Store\make_icons.py
然后重新编译 Release，再 pack_msix.bat / build_msix.sh。
