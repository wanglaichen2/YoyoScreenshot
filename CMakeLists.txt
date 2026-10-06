cmake_minimum_required(VERSION 3.20)
project(YoyoScreenshot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT WIN32)
    message(FATAL_ERROR "YoyoScreenshot is Windows-only")
endif()
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "YoyoScreenshot is x64-only")
endif()

set(PROJECT_ROOT "${CMAKE_SOURCE_DIR}")
set(APP_ROOT "${CMAKE_SOURCE_DIR}")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(_cfg "${CMAKE_BUILD_TYPE}")
set(_plat "x64")

if(MINGW)
    set(_xcgui_lib "${PROJECT_ROOT}/libs/lib/${_cfg}/${_plat}/libXCGUI.a")
    if(NOT EXISTS "${_xcgui_lib}")
        set(_xcgui_lib "${PROJECT_ROOT}/libs/lib/Release/${_plat}/libXCGUI.a")
    endif()
else()
    set(_xcgui_lib "${PROJECT_ROOT}/libs/lib/${_cfg}/${_plat}/XCGUI.lib")
    if(NOT EXISTS "${_xcgui_lib}")
        set(_xcgui_lib "${PROJECT_ROOT}/libs/lib/Release/${_plat}/XCGUI.lib")
    endif()
endif()

if(NOT EXISTS "${_xcgui_lib}")
    message(FATAL_ERROR "未找到静态库: ${_xcgui_lib}\n请把 MSVC 的 .lib 或 MinGW 的 .a 放到 libs/lib/<Debug|Release>/x64/")
endif()

set(_out_dir "${PROJECT_ROOT}/Exec/${_cfg}/x64/YoyoScreenshot")
set(_int_dir "${PROJECT_ROOT}/temp/YoyoScreenshot/cmake/${_cfg}/x64")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${_out_dir}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_ROOT}/Exec/Debug/x64/YoyoScreenshot")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_ROOT}/Exec/Release/x64/YoyoScreenshot")

set(APP_SOURCES
    "${APP_ROOT}/app/main.cpp"
    "${APP_ROOT}/app/MainWnd.cpp"
    "${APP_ROOT}/app/stdafx.cpp"
    "${APP_ROOT}/core/ScreenshotService.cpp"
    "${APP_ROOT}/core/CaptureOverlay.cpp"
    "${APP_ROOT}/platform/TrayIcon.cpp"
    "${APP_ROOT}/platform/MiniDump.cpp"
)

set(_rc "${APP_ROOT}/resources/YoyoScreenshot.rc")
if(EXISTS "${_rc}" AND MINGW)
    enable_language(RC)
    set(_rc_obj "${CMAKE_CURRENT_BINARY_DIR}/YoyoScreenshot_rc.obj")
    add_custom_command(
        OUTPUT "${_rc_obj}"
        COMMAND "${CMAKE_RC_COMPILER}"
            -O coff
            -DUNICODE -D_UNICODE -DWIN32 -D_WINDOWS
            -I "${APP_ROOT}/resources"
            -i "YoyoScreenshot.rc"
            -o "${_rc_obj}"
        DEPENDS "${_rc}" "${APP_ROOT}/resources/icon.ico" "${APP_ROOT}/resources/resource.h" "${APP_ROOT}/resources/YoyoScreenshot.manifest"
        WORKING_DIRECTORY "${APP_ROOT}/resources"
        COMMENT "windres YoyoScreenshot.rc"
        VERBATIM
    )
    list(APPEND APP_SOURCES "${_rc_obj}")
    set_source_files_properties("${_rc_obj}" PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)
elseif(EXISTS "${_rc}")
    enable_language(RC)
    list(APPEND APP_SOURCES "${_rc}")
endif()

add_executable(YoyoScreenshot WIN32 ${APP_SOURCES})
set_target_properties(YoyoScreenshot PROPERTIES OUTPUT_NAME "YoyoScreenshot")

target_include_directories(YoyoScreenshot PRIVATE
    "${APP_ROOT}"
    "${APP_ROOT}/app"
    "${APP_ROOT}/core"
    "${APP_ROOT}/platform"
    "${APP_ROOT}/bridge"
    "${APP_ROOT}/resources"
    "${APP_ROOT}/SDK/inc"
)

target_compile_definitions(YoyoScreenshot PRIVATE
    UNICODE
    _UNICODE
    _WINDOWS
    XC_LIB
    WINVER=0x0601
    _WIN32_WINNT=0x0601
    _CRT_SECURE_NO_DEPRECATE
    _CRT_SECURE_NO_WARNINGS
)

message(STATUS "YoyoScreenshot: static UI lib -> ${_xcgui_lib}")

if(MSVC)
    target_compile_options(YoyoScreenshot PRIVATE /W3 /utf-8 /wd4819 /Zc:wchar_t-)
    target_link_options(YoyoScreenshot PRIVATE "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup")
elseif(MINGW)
    target_compile_options(YoyoScreenshot PRIVATE -fpermissive -w)
    target_link_options(YoyoScreenshot PRIVATE -mwindows -static-libgcc -static-libstdc++)
endif()

target_link_libraries(YoyoScreenshot PRIVATE
    "${_xcgui_lib}"
    gdi32
    user32
    shell32
    advapi32
    ole32
    comctl32
    msimg32
    comdlg32
    dbghelp
    gdiplus
    imm32
    winmm
    uuid
    oleaut32
    shlwapi
    ws2_32
)

add_custom_command(TARGET YoyoScreenshot POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:YoyoScreenshot>/resources"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${APP_ROOT}/resources/icon.ico" "$<TARGET_FILE_DIR:YoyoScreenshot>/icon.ico"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${APP_ROOT}/resources/icon.ico" "$<TARGET_FILE_DIR:YoyoScreenshot>/resources/icon.ico"
    COMMENT "Copy YoyoScreenshot icons"
)

add_custom_target(build_yoyo_screenshot DEPENDS YoyoScreenshot)
