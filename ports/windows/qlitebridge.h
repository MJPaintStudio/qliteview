#pragma once
#include <stdint.h>

#ifdef QLITEBRIDGE_BUILD
#define QLV_API __declspec(dllexport)
#else
#define QLV_API __declspec(dllimport)
#endif

// Private C boundary between the Qt DLL and the bundled native runtime.
// None of Blink's C++ types cross this boundary.
struct QlvSettings {
    uint64_t flags;
    const char* encoding;
};
enum QlvSetting : uint64_t {
    QlvJavaScript = 1ull << 0, QlvLocalStorage = 1ull << 1,
    QlvImages = 1ull << 2, QlvClipboard = 1ull << 3,
    QlvPaste = 1ull << 4, QlvFileAccess = 1ull << 5,
    QlvRemoteAccess = 1ull << 6, QlvScrollbars = 1ull << 7,
    QlvWindows = 1ull << 8, QlvInsecureContent = 1ull << 9,
    QlvDnsPrefetch = 1ull << 10, QlvHyperlinkAudit = 1ull << 11,
    QlvPrintBackgrounds = 1ull << 12, QlvSpatialNavigation = 1ull << 13
};
using QlvCallback = void(__stdcall*)(int64_t view, void* user);
using QlvNavigation = int(__stdcall*)(int64_t view, int type, const char* url, int mainFrame);
using QlvLoadStarted = void(__stdcall*)(int64_t view);
using QlvProgress = void(__stdcall*)(int64_t view, int percent);
extern "C" {
QLV_API void __stdcall qlv_set_callbacks(int64_t view, QlvNavigation navigation, QlvLoadStarted started,
    QlvProgress progress, QlvLoadStarted close);
QLV_API void __stdcall qlv_set_user_agent(int64_t view, const char* agent);
QLV_API void __stdcall qlv_set_document_script(int64_t view, const char* script);
QLV_API void __stdcall qlv_inject_document_script(int64_t view, void* frame, int world);
QLV_API void __stdcall qlv_start_runtime();
QLV_API void __stdcall qlv_prepare_view(int64_t view, const char* storage,
    const char* cookies, QlvCallback ready, void* user);
QLV_API void __stdcall qlv_set_settings(int64_t view, const QlvSettings* settings);
QLV_API void __stdcall qlv_set_background(int64_t view, uint32_t argb);
QLV_API void __stdcall qlv_reload(int64_t view, int bypassCache);
QLV_API void __stdcall qlv_edit(int64_t view, const char* command);
QLV_API void __stdcall qlv_clear_cache(QlvCallback done, void* user);
}
