#ifndef in_

#define in_

#endif

#ifndef out_

#define out_

#endif

#ifndef in_out_

#define in_out_

#endif

XC_API HELE WINAPI XBtn_Create(int x, int y, int cx, int cy,wchar_t *pTitle=NULL,HXCGUI hParent=NULL);  //创建按钮

XC_API void WINAPI XBtn_SetCheck(HELE hEle,BOOL bCheck);  //设置按钮选中状态

XC_API void WINAPI XBtn_SetImageLeave(HELE hEle,HIMAGE hImage=NULL);  //鼠标离开时图片

XC_API void WINAPI XBtn_SetImageStay(HELE hEle,HIMAGE hImage=NULL);  //鼠标停留时图片

XC_API void WINAPI XBtn_SetImageDown(HELE hEle,HIMAGE hImage=NULL);  //鼠标按下时图片

XC_API HELE WINAPI XComboBox_Create(int x, int y, int cx, int cy,HXCGUI hParent=NULL);  //创建下拉组合框元素

XC_API BOOL WINAPI XComboBox_AddString(HELE hEle,wchar_t *pText,INT_PTR data=0);  //增加一个字符串到组合框

XC_API void WINAPI XComboBox_DeleteItemAll(HELE hEle);  //删除所有项

XC_API void WINAPI XComboBox_EnableEdit(HELE hEle,BOOL bEdit);  //启动编辑内容

XC_API void WINAPI XComboBox_SetText(HELE hEle,wchar_t *pText);  //设置编辑框内容

XC_API void WINAPI XComboBox_GetText(HELE hEle,wchar_t *pOut,int len);  //获取编辑框文本

XC_API void WINAPI XComboBox_SetListHeight(HELE hEle,int height);  //设置下拉列表高度

XC_API HPEN WINAPI XDraw_CreatePen_(HDRAW hDraw,int fnPenStyle,int nWidth,COLORREF crColor);  //创建画笔

XC_API HGDIOBJ WINAPI XDraw_SelectObject_(HDRAW hDraw,HGDIOBJ hObj);  //选择GDI对象

XC_API BOOL WINAPI XDraw_DeleteObject_(HDRAW hDraw,HGDIOBJ hObj);  //删除GDI对象

XC_API BOOL WINAPI XDraw_MoveToEx_(HDRAW hDraw,int X,int Y,LPPOINT lpPoint=NULL);  //移动画笔位置

XC_API BOOL WINAPI XDraw_LineTo_(HDRAW hDraw,int nXEnd,int nYEnd);  //画线到指定位置

XC_API void WINAPI XDraw_Dottedline_(HDRAW hDraw,int x,int y,int length,COLORREF color,BOOL bHorizontal=TRUE);  //绘制虚线

XC_API void WINAPI XDraw_HImage2_(HDRAW hDraw,HIMAGE hImage,int x,int y,int width,int height);  //绘制图片(指定宽高)
XC_API void WINAPI XDraw_FillSolidRect_(HDRAW hDraw,RECT *pRect,COLORREF clr);  //填充实心矩形
XC_API int WINAPI XDraw_DrawTextA_(HDRAW hDraw,char * lpString,RECT* lpRect,UINT uFormat);  //绘制ANSI文本

XC_API HELE WINAPI XEle_Create(int x, int y, int cx, int cy,HXCGUI hParent=NULL);  //创建

XC_API void WINAPI XEle_SetBkColor(HELE hEle,COLORREF color);

XC_API void WINAPI XEle_SetBorderColor(HELE hEle,COLORREF color);

XC_API void WINAPI XEle_RegisterEvent(HELE hEle,int type,void *pFun);  //注册元素事件处理函数

XC_API void WINAPI XEle_SetBkTransparent(HELE hEle,BOOL bTran);  //设置元素背景透明

XC_API void WINAPI XEle_SetFocus(HELE hEle,BOOL bFocus);  //设置焦点

XC_API void WINAPI XEle_SetFont(HELE hEle,HFONTX hFontX);  //设置元素字体

XC_API HFONTX WINAPI XEle_GetFont(HELE hEle);  //获取字体

XC_API void WINAPI XEle_SetToolTips(HELE hEle,wchar_t *pText);  //设置工具提示内容

XC_API HWND WINAPI XEle_GetHWnd(HELE hEle);  //获取元素所在窗口句柄

XC_API HWINDOW WINAPI XEle_GetHWindow(HELE hEle);  //获取元素所在窗口的窗口句柄

XC_API void WINAPI XEle_EnableDrawFocus(HELE hEle,BOOL bFocus);  //是否绘制焦点

XC_API void WINAPI XEle_EnableBorder(HELE hEle,BOOL bEnable);  //启用边框

XC_API void WINAPI XEle_EnableToolTips(HELE hEle,BOOL bTips);  //启用工具提示

XC_API void WINAPI XEle_RedrawEle(HELE hEle,BOOL bImmediately=FALSE);  //重绘元素

XC_API void WINAPI XEle_ShowEle(HELE hEle,BOOL bShow);  //显示隐藏元素

XC_API void WINAPI XEle_SetRect(HELE hEle,in_ RECT *pRect,BOOL bRedraw=FALSE);
XC_API BOOL WINAPI XEle_IsFocus(HELE hEle);  //元素是否拥有焦点

XC_API void WINAPI XEle_GetClientRect(HELE hEle,RECT *pRect);  //元素客户区坐标 xy(0,0)

XC_API void WINAPI XEle_GetRect(HELE hEle,out_ RECT *pRect);

XC_API BOOL WINAPI XEle_Destroy(HELE hEle);

XC_API void WINAPI XEle_SetTextColor(HELE hEle,COLORREF color);  //设置文本颜色

XC_API HELE WINAPI XEdit_Create(int x, int y, int cx, int cy,HXCGUI hParent=NULL);

XC_API void WINAPI XEdit_GetText(HELE hEle,wchar_t *pOut,int len);

XC_API void WINAPI XEdit_SetText(HELE hEle,wchar_t *pText);  //设置文本

XC_API HELE WINAPI XList_Create(int x,int y,int cx,int cy,HXCGUI hParent=NULL);

XC_API int WINAPI XList_AddColumn(HELE hEle,int width,wchar_t *pText,int imageId=-1);

XC_API int WINAPI XList_AddItem(HELE hEle,wchar_t *pText, int imageId=-1);

XC_API BOOL WINAPI XList_SetItemText(HELE hEle,int item,int subItem,wchar_t *pText,int imageId=-1);

XC_API int WINAPI XList_GetSelectItem(HELE hEle);

XC_API void WINAPI XList_DeleteAllItems(HELE hEle);

XC_API void WINAPI XList_DeleteAllColumns(HELE hEle);

XC_API void WINAPI XList_EnableGrid(HELE hEle,BOOL bGrid);

XC_API void WINAPI XList_ShowHeader(HELE hEle,BOOL bShow);

XC_API void WINAPI XList_EnableMultilineSelect(HELE hEle,BOOL bEnable);

XC_API int WINAPI XList_HitTest(HELE hEle,POINT *pPt);

XC_API BOOL WINAPI XList_SetSelectItem(HELE hEle,int index);

XC_API void WINAPI XList_CancelSelectAll(HELE hEle);

XC_API int WINAPI XList_GetItemCount(HELE hEle);

XC_API void WINAPI XList_SetItemHeight(HELE hEle,int height);

XC_API void WINAPI XSView_SetSize(HELE hEle,int cx, int cy);

XC_API HELE WINAPI XSView_Create(int x, int y, int cx, int cy,HXCGUI hParent=NULL);

XC_API void WINAPI XSView_EnableHScroll(HELE hEle,BOOL bEnable);

XC_API void WINAPI XSView_EnableVScroll(HELE hEle,BOOL bEnable);

XC_API void WINAPI XSView_SetSpacing(HELE hEle,int left, int top, int right, int bottom);

XC_API void WINAPI XSView_Adjust(HELE hEle);

XC_API void WINAPI XEle_SetUserData(HELE hEle,INT_PTR data);

XC_API INT_PTR WINAPI XEle_GetUserData(HELE hEle);

XC_API BOOL WINAPI XImage_SetDrawType(HIMAGE hImage,int type);

XC_API void WINAPI XBtn_SetText(HELE hEle,wchar_t *pText);

XC_API void WINAPI XEle_PointToWndClient(HELE hEle,in_out_ POINT *pPt);

XC_API HFONTX WINAPI XFont_Create2(const wchar_t *pName=L"SimSun",int size=12,BOOL bBold=FALSE,BOOL bItalic=FALSE,BOOL bUnderline=FALSE,BOOL bStrikeOut=FALSE);  //创建炫彩字体

XC_API HFONT WINAPI XFont_GetHFONT(HFONTX hFontX);  //获取HFONT句柄

XC_API HIMAGE WINAPI XImage_LoadFile(wchar_t *pImageName,BOOL bStretch=FALSE);  //加载图片从文件

XC_API HIMAGE WINAPI XImage_LoadRes(int id,wchar_t *pType,BOOL bStretch=FALSE);  //加载图片从资源

XC_API HIMAGE WINAPI XImage_LoadFileFromExtractIcon(wchar_t *pImageName);  //从 exe/dll/ico 提取图标

XC_API HIMAGE WINAPI XImage_LoadFileFromHICON(HICON hIcon);  //从 HICON 创建图片

XC_API void WINAPI XImage_Destroy(HIMAGE hImage);  //销毁图片

XC_API void WINAPI XImage_EnableAutoDestroy(HIMAGE hImage,BOOL bEnable);  //启用或关闭自动销毁

XC_API HXCGUI WINAPI XImageList_Create(int cx,int cy);

XC_API void WINAPI XImageList_Destroy(HXCGUI hImageList);

XC_API void WINAPI XImageList_AddImage(HXCGUI hImageList,HIMAGE hImage);

XC_API int WINAPI XImageList_GetCount(HXCGUI hImageList);

XC_API void WINAPI XList_SetImageList(HELE hEle,HXCGUI hImageList);  //设置列表图片列表

XC_API HELE WINAPI XMenuBar_Create(int x, int y, int cx, int cy,HXCGUI hParent=NULL);

XC_API int WINAPI XMenuBar_AddButton(HELE hEle,wchar_t *pText);

XC_API void WINAPI XMenuBar_AddMenuItem(HELE hEle,int index,int id,wchar_t *pText,int parentId=XMENU_ROOT,int uFlags=0);

XC_API HMENUX WINAPI XMenu_Create();  //创建菜单

XC_API void WINAPI XMenu_AddItem(HMENUX hMenu,int id,wchar_t *pText,int parentId=XMENU_ROOT,int uFlags=0);  //添加菜单项

XC_API BOOL WINAPI XMenu_Popup(HMENUX hMenu,HWND hParentWnd,int x,int y,HELE hParentEle=NULL,int uFlags=NULL);  //弹出菜单

XC_API HWINDOW WINAPI XModalWnd_CreateWindow(int width,int height,wchar_t *pTitle,HWND hWndParent,int XCStyle=XC_SY_MODAL_WINDOW);  //创建模态窗口;当模态窗口关闭时,会自动销毁模态窗口资源句柄. 默认情况下,模态窗口会基于父窗口坐标居中显示

XC_API int WINAPI XModalWnd_DoModal(HWINDOW hWindow);  //启动模态窗口

XC_API void WINAPI XModalWnd_DoOk(HWINDOW hWindow);  //确定

XC_API void WINAPI XModalWnd_DoCancel(HWINDOW hWindow);  //取消

XC_API HELE WINAPI XRadio_Create(int x, int y, int cx, int cy,wchar_t *pTitle=NULL,HXCGUI hParent=NULL);  //创建单选按钮元素

XC_API void WINAPI XRadio_SetImageLeave_UnCheck(HELE hEle,HIMAGE hImage=NULL);  //鼠标离开时图片

XC_API void WINAPI XRadio_SetImageStay_UnCheck(HELE hEle,HIMAGE hImage=NULL);  //鼠标停留时图片

XC_API void WINAPI XRadio_SetImageDown_UnCheck(HELE hEle,HIMAGE hImage=NULL);  //鼠标按下时图片

XC_API void WINAPI XRadio_SetImageLeave_Check(HELE hEle,HIMAGE hImage=NULL);  //鼠标离开时图片

XC_API void WINAPI XRadio_SetImageStay_Check(HELE hEle,HIMAGE hImage=NULL);  //鼠标停留时图片

XC_API void WINAPI XRadio_SetImageDown_Check(HELE hEle,HIMAGE hImage=NULL);  //鼠标按下时图片

XC_API void WINAPI XRadio_SetBindEle(HELE hEle,HELE bindEle);  //绑定元素

XC_API HELE WINAPI XStatic_Create(int x, int y, int cx, int cy,wchar_t *pTitle=NULL,HXCGUI hParent=NULL);  //创建静态文本元素

XC_API void WINAPI XStatic_SetText(HELE hEle,wchar_t *pText);  //设置文本内容

XC_API void WINAPI XStatic_SetTextAlign(HELE hEle,int flag);  //设置文本对齐

XC_API HWINDOW WINAPI XWnd_CreateWindow(int x,int y,int cx,int cy,wchar_t *pTitle,HWND hWndParent=NULL,int XCStyle=XC_SY_DEFAULT);  //创建窗口

XC_API void WINAPI XWnd_RegisterMessage(HWINDOW hWindow,int message,void *pFun);  //注册消息回调函数

XC_API void WINAPI XWnd_RegisterMessageProc(HWINDOW hWindow,pFunWndMsgProc pFun);  //注册窗口消息处理过程,可以在这里拦截所有消息

XC_API HWND WINAPI XWnd_GetHWnd(HWINDOW hWindow);  //获取窗口句柄HWND

XC_API DWORD WINAPI XWnd_SetIcon(HWINDOW hWindow,HICON hIcon,BOOL bBigIcon);  //设置窗口图标

XC_API UINT WINAPI XWnd_SetTimerEx(HWINDOW hWindow,UINT nIDEvent,UINT uElapse,int userData=0);  //设置定时器(扩展)

XC_API BOOL WINAPI XWnd_KillTimerEx(HWINDOW hWindow,UINT nIDEvent);  //销毁定时器(扩展)

XC_API void WINAPI XWnd_RedrawWnd(HWINDOW hWindow,BOOL bImmediately=FALSE);  //重绘窗口

XC_API void WINAPI XWnd_EnableDragBorder(HWINDOW hWindow,BOOL bDrag);  //使用拖动边框

XC_API void WINAPI XWnd_SetTransparentFlag(HWINDOW hWindow,int flag);  //设置透明窗口

XC_API void WINAPI XWnd_SetTransparentAlpha(HWINDOW hWindow,BYTE alpha);  //设置窗口透明度

XC_API void WINAPI XWnd_SetImage(HWINDOW hWindow,HIMAGE hImage=NULL);  //客户区图片

XC_API void WINAPI XWnd_SetBkColor(HWINDOW hWindow,COLORREF color);  //设置背景颜色

XC_API void WINAPI XWnd_EnableBorderStrokeInner(HWINDOW hWindow,BOOL bStroke);  //启用绘制描边

XC_API void WINAPI XWnd_EnableBorderStrokeOuter(HWINDOW hWindow,BOOL bStroke);  //启用绘制描边

XC_API void WINAPI XWnd_CloseWindow(HWINDOW hWindow);  //关闭窗口

XC_API BOOL WINAPI XWnd_GetClientRect(HWINDOW hWindow,RECT *pRect);  //获取客户区坐标

XC_API BOOL WINAPI XWnd_ShowWindow(HWINDOW hWindow,int nCmdShow);  //显示或隐藏窗口

XC_API BOOL WINAPI XWnd_SetWindowPos(HWINDOW hWindow,HWND hWndInsertAfter,int x,int y,int cx,int cy,UINT uFlags);  //设置窗口位置与大小

XC_API HELE WINAPI XPPage_Create(int x, int y, int cx, int cy, HXCGUI hParent=NULL);

XC_API int WINAPI XPPage_AddLabel(HELE hEle, wchar_t *pName, HELE hPage=NULL);

XC_API void WINAPI XPPage_SetSelect(HELE hEle, int index, BOOL reDraw=TRUE);

XC_API int WINAPI XPPage_GetSelect(HELE hEle);

XC_API void WINAPI XPPage_SetLabelHeight(HELE hEle, int height);

XC_API void WINAPI XPPage_SetLabelWidth(HELE hEle, int index, int width);

XC_API void WINAPI XPPage_SetLabelSpacing(HELE hEle, int spacing);

XC_API void WINAPI XPPage_EnableScrollButton(HELE hEle, BOOL bEnable);

XC_API BOOL WINAPI XInitXCGUI(wchar_t *pText=NULL);  //界面库初始化,不得重复调用

XC_API void WINAPI XRunXCGUI();  //运行程序,不得重复调用

