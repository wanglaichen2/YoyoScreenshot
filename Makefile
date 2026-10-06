# MinGW 编译（静态库），用法:
#   mingw32-make -f Makefile CFG=Release
# 产物: Exec/Release/x64/YoyoScreenshot/YoyoScreenshot.exe

CFG ?= Release
ARCH := x64
ROOT := $(CURDIR)
OUTDIR := $(ROOT)/Exec/$(CFG)/$(ARCH)/YoyoScreenshot
INTDIR := $(ROOT)/temp/YoyoScreenshot/mingw/$(CFG)/$(ARCH)
CXX ?= g++
WINDRES ?= windres

CXXFLAGS := -std=c++14 -O2 -fpermissive -w -mwindows \
	-DUNICODE -D_UNICODE -D_WINDOWS -DXC_LIB \
	-DWINVER=0x0601 -D_WIN32_WINNT=0x0601 \
	-D_CRT_SECURE_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS \
	-I$(ROOT) -I$(ROOT)/app -I$(ROOT)/core -I$(ROOT)/platform \
	-I$(ROOT)/bridge -I$(ROOT)/resources -I$(ROOT)/SDK/inc

LDFLAGS := -mwindows -static-libgcc -static-libstdc++
LDLIBS := $(ROOT)/libs/lib/$(CFG)/$(ARCH)/libXCGUI.a \
	-lgdi32 -luser32 -lshell32 -ladvapi32 -lole32 -lcomctl32 -lmsimg32 \
	-lcomdlg32 -ldbghelp -lgdiplus -limm32 -lwinmm -luuid -loleaut32 -lshlwapi -lws2_32

ifeq ($(wildcard $(ROOT)/libs/lib/$(CFG)/$(ARCH)/libXCGUI.a),)
LDLIBS := $(ROOT)/libs/lib/Release/$(ARCH)/libXCGUI.a \
	-lgdi32 -luser32 -lshell32 -ladvapi32 -lole32 -lcomctl32 -lmsimg32 \
	-lcomdlg32 -ldbghelp -lgdiplus -limm32 -lwinmm -luuid -loleaut32 -lshlwapi -lws2_32
endif

SRCS := \
	app/main.cpp \
	app/MainWnd.cpp \
	app/stdafx.cpp \
	core/ScreenshotService.cpp \
	core/CaptureOverlay.cpp \
	platform/TrayIcon.cpp \
	platform/MiniDump.cpp

OBJS := $(patsubst %.cpp,$(INTDIR)/%.o,$(SRCS))
RES := $(INTDIR)/YoyoScreenshot_rc.o
TARGET := $(OUTDIR)/YoyoScreenshot.exe

.PHONY: all clean
all: $(TARGET)

$(INTDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(RES): resources/YoyoScreenshot.rc resources/icon.ico resources/resource.h resources/YoyoScreenshot.manifest
	@mkdir -p $(dir $@)
	$(WINDRES) -O coff -DUNICODE -D_UNICODE -DWIN32 -D_WINDOWS \
		-I resources -i resources/YoyoScreenshot.rc -o $@

$(TARGET): $(OBJS) $(RES)
	@mkdir -p $(OUTDIR)/resources
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(RES) $(LDLIBS)
	cp -f resources/icon.ico $(OUTDIR)/icon.ico
	cp -f resources/icon.ico $(OUTDIR)/resources/icon.ico

clean:
	rm -rf $(INTDIR) $(TARGET)
