#define QLITEBRIDGE_BUILD
#include "qlitebridge.h"
#include "api/core/mb.h"
#include "content/browser/MbWebview.h"
#include "content/common/ThreadCall.h"
#include "content/renderer/RenderThreadImpl.h"
#include "third_party/blink/public/web/web_view.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_settings.h"
#include "third_party/blink/renderer/platform/weborigin/scheme_registry.h"
#include "third_party/blink/renderer/platform/loader/fetch/memory_cache.h"
#include "url/url_util.h"
#include <string>
#include <map>
#include <mutex>
#include "content/QtBridgeHooks.h"

namespace content {
extern bool g_enableNativeSetFocus;
extern bool g_enableNativeSetCapture;
}

namespace {
struct Callbacks {
    QlvNavigation navigation = nullptr;
    QlvLoadStarted started = nullptr;
    QlvProgress progress = nullptr;
    QlvLoadStarted close = nullptr;
    std::string userAgent;
    std::string documentScript;
};
std::mutex callbacksMutex;
std::map<int64_t, Callbacks> callbacks;
Callbacks forView(int64_t view) {
    std::lock_guard<std::mutex> lock(callbacksMutex);
    auto it = callbacks.find(view);
    return it == callbacks.end() ? Callbacks{} : it->second;
}
}
void __stdcall qlv_set_callbacks(int64_t view, QlvNavigation navigation, QlvLoadStarted started,
    QlvProgress progress, QlvLoadStarted close)
{
    std::lock_guard<std::mutex> lock(callbacksMutex);
    if (!navigation && !started) callbacks.erase(view);
    else {
        callbacks[view].navigation = navigation; callbacks[view].started = started;
        callbacks[view].progress = progress; callbacks[view].close = close;
    }
}
void __stdcall qlv_set_user_agent(int64_t view, const char* agent)
{
    std::lock_guard<std::mutex> lock(callbacksMutex);
    callbacks[view].userAgent = agent ? agent : "";
}
void __stdcall qlv_set_document_script(int64_t view, const char* script)
{
    std::lock_guard<std::mutex> lock(callbacksMutex);
    callbacks[view].documentScript = script ? script : "";
}
void __stdcall qlv_inject_document_script(int64_t view, void* frame, int world)
{
    if (world != 0 || !mbIsMainFrame(view, frame)) return;
    auto script = forView(view).documentScript;
    if (!script.empty()) mbRunJs(view, frame, script.c_str(), false, nullptr, nullptr, nullptr);
}
namespace qliteview {
bool navigation(int64_t view, int type, const char* url, bool mainFrame)
{
    bool accepted = true;
    content::ThreadCall::callUiThreadSync(MB_FROM_HERE, [&] {
        auto callback = forView(view).navigation;
        if (callback) accepted = callback(view, type, url, mainFrame) != 0;
    });
    return accepted;
}
void loadStarted(int64_t view)
{
    auto callback = forView(view).started;
    if (callback) callback(view);
}
std::string userAgent(int64_t view) { return forView(view).userAgent; }
void loadProgress(int64_t view, double progress) {
    auto callback = forView(view).progress;
    if (callback) callback(view, static_cast<int>(progress * 100));
}
bool requestClose(int64_t view) {
    auto callback = forView(view).close;
    if (!callback) return false;
    callback(view);
    return true;
}
}

void __stdcall qlv_start_runtime()
{
    // Qt owns focus and pointer capture; the hidden engine HWND must not
    // steal either from the QQuickWindow receiving input.
    content::g_enableNativeSetFocus = false;
    content::g_enableNativeSetCapture = false;
    // qrc:/path has no authority. Register it as local without applying the
    // standard URL parser, which would turn the first path segment into a host.
    url::AddLocalScheme("qrc");
    mbInit(nullptr);
    // The Qt adapter supplies physical viewport sizes and per-view zoom.
    // Keep the root compositor in the same pixel space as its child surface.
    content::RenderThreadImpl::get()->setZoom(1.0f);
}

void __stdcall qlv_prepare_view(int64_t handle, const char* storage,
    const char* cookies, QlvCallback ready, void* user)
{
    const std::string storagePath(storage ? storage : "");
    const std::string cookiePath(cookies ? cookies : "");
    // mbCreateWebView itself posts through the UI queue. Post behind it so
    // settings are applied after the Blink view exists and before first load.
    content::ThreadCall::callUiThreadAsync(MB_FROM_HERE, [=] {
        content::ThreadCall::callBlinkThreadAsyncWithValid(MB_FROM_HERE, handle,
            [=](content::MbWebView* view) {
                blink::SchemeRegistry::RegisterURLSchemeAsSupportingFetchAPI("qrc");
                view->setLocalStorageFullPath(storagePath.c_str());
                view->setCookieJarFullPath(cookiePath.c_str());
                content::ThreadCall::callUiThreadAsync(MB_FROM_HERE, [=] {
                    if (ready) ready(handle, user);
                });
            });
    });
}

void __stdcall qlv_set_settings(int64_t handle, const QlvSettings* input)
{
    if (!input) return;
    const uint64_t flags = input->flags;
    const std::string encoding(input->encoding ? input->encoding : "UTF-8");
    content::ThreadCall::callBlinkThreadAsyncWithValid(MB_FROM_HERE, handle,
        [flags, encoding](content::MbWebView* view) {
            if (!view->getWebView()) return;
            auto* s = view->getWebView()->GetSettings();
            s->SetJavaScriptEnabled(flags & QlvJavaScript);
            s->SetLocalStorageEnabled(flags & QlvLocalStorage);
            s->SetLoadsImagesAutomatically(flags & QlvImages);
            s->SetJavaScriptCanAccessClipboard(flags & QlvClipboard);
            s->SetDOMPasteAllowed(flags & QlvPaste);
            s->SetAllowFileAccessFromFileURLs(flags & QlvFileAccess);
            s->SetAllowUniversalAccessFromFileURLs(flags & QlvRemoteAccess);
            s->SetHideScrollbars(!(flags & QlvScrollbars));
            s->SetSupportsMultipleWindows(flags & QlvWindows);
            s->SetAllowRunningOfInsecureContent(flags & QlvInsecureContent);
            s->SetDNSPrefetchingEnabled(flags & QlvDnsPrefetch);
            s->SetHyperlinkAuditingEnabled(flags & QlvHyperlinkAudit);
            s->SetShouldPrintBackgrounds(flags & QlvPrintBackgrounds);
            s->SetSpatialNavigationEnabled(flags & QlvSpatialNavigation);
            s->SetDefaultTextEncodingName(blink::WebString::FromUTF8(encoding));
            // Match the supplied qwebkit product's disabled plugin/WebGL surface.
            s->SetPluginsEnabled(false);
            s->SetWebGL1Enabled(false);
            s->SetWebGL2Enabled(false);
        });
}

void __stdcall qlv_set_background(int64_t handle, uint32_t argb)
{
    content::ThreadCall::callBlinkThreadAsyncWithValid(MB_FROM_HERE, handle,
        [argb](content::MbWebView* view) { view->setBackgroundColor(argb); });
}

void __stdcall qlv_reload(int64_t handle, int bypassCache)
{
    content::ThreadCall::callBlinkThreadAsyncWithValid(MB_FROM_HERE, handle,
        [bypassCache](content::MbWebView* view) { view->reload(bypassCache != 0); });
}

void __stdcall qlv_edit(int64_t handle, const char* command)
{
    const std::string copy(command ? command : "");
    content::ThreadCall::callBlinkThreadAsyncWithValid(MB_FROM_HERE, handle,
        [copy](content::MbWebView* view) {
            auto* webView = view->getWebView();
            if (!webView) return;
            auto* frame = webView->FocusedFrame();
            if (!frame && webView->MainFrame() && webView->MainFrame()->IsWebLocalFrame())
                frame = webView->MainFrame()->ToWebLocalFrame();
            if (frame) frame->ExecuteCommand(blink::WebString::FromUTF8(copy));
        });
}

void __stdcall qlv_clear_cache(QlvCallback done, void* user)
{
    content::ThreadCall::callBlinkThreadAsync(MB_FROM_HERE, [=] {
        blink::MemoryCache::Get()->EvictResources();
        content::ThreadCall::callUiThreadAsync(MB_FROM_HERE, [=] {
            if (done) done(0, user);
        });
    });
}
