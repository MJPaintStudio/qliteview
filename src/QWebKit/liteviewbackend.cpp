#include "liteviewbackend.h"
#include "webengineview.h"
#include "qlitebridge.h"
#include "api/core/mb.h"
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeDatabase>
#include <QQuickWindow>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QHoverEvent>
#include <QQmlEngine>
#include <QThread>
#include <QDir>
#include <deque>
#include <utility>
#include <commdlg.h>

namespace {
QString literal(const QString& value) {
    QByteArray json = QJsonDocument(QJsonArray{value}).toJson(QJsonDocument::Compact);
    return QString::fromUtf8(json.mid(1, json.size()-2));
}
unsigned mouseFlags(Qt::KeyboardModifiers modifiers, Qt::MouseButtons buttons = {}) {
    return (modifiers.testFlag(Qt::ShiftModifier) ? MB_SHIFT : 0)
        | (modifiers.testFlag(Qt::ControlModifier) ? MB_CONTROL : 0)
        | (buttons.testFlag(Qt::LeftButton) ? MB_LBUTTON : 0)
        | (buttons.testFlag(Qt::RightButton) ? MB_RBUTTON : 0)
        | (buttons.testFlag(Qt::MiddleButton) ? MB_MBUTTON : 0);
}
constexpr intptr_t mainFrame = -2;
const char channelScript[] = R"JS((function(){
    if (%1) {
        window.qt = window.qt || {};
        window.qt.webChannelTransport = window.qt.webChannelTransport || {
            send: function(message) { window.mbQuery(0, message); }, onmessage: null
        };
    } else if (window.qt) delete window.qt.webChannelTransport;
    if (window.__qliteviewInstalled) return;
    window.__qliteviewInstalled = true;
    document.addEventListener('contextmenu', function(event) {
        event.preventDefault();
        const link = event.target.closest ? event.target.closest('a') : null;
        window.mbQuery(1, JSON.stringify({x:event.clientX,y:event.clientY,link:link?link.href:'',
            selection:String(window.getSelection()),
            editable:!!event.target.isContentEditable || /^(INPUT|TEXTAREA)$/.test(event.target.tagName)}));
    }, true);
})())JS";
}

struct LiteViewBackend::Private {
    LiteViewBackend* backend;
    WebEngineView* owner;
    mbWebView handle = 0;
    HWND host = nullptr;
    bool ready = false;
    bool back = false;
    bool forward = false;
    qreal dpr = 1;
    double appliedZoom = 0;
    QSize size;
    std::deque<std::function<void()>> pending;
    QHash<quintptr, std::function<void(QVariant)>> evaluations;
    quintptr nextEvaluation = 1;

    // This registry is touched only on the Qt GUI thread. Worker callbacks
    // carry a view ID and owned data, never a pointer to a Qt object.
    static QHash<mbWebView, Private*>& views() {
        static QHash<mbWebView, Private*> registry;
        return registry;
    }
    template<class F> static void post(mbWebView handle, F callback) {
        if (auto app = QCoreApplication::instance()) {
            QMetaObject::invokeMethod(app, [handle, callback = std::move(callback)]() mutable {
                if (auto self = views().value(handle)) callback(*self);
            }, Qt::QueuedConnection);
        }
    }
    void afterReady(std::function<void()> action) {
        if (ready) action();
        else pending.push_back(std::move(action));
    }
    static void __stdcall prepared(int64_t handle, void*) {
        auto self = views().value(handle);
        if (!self) {
            qlv_set_callbacks(handle, nullptr, nullptr, nullptr, nullptr);
            mbDestroyWebView(handle);
            return;
        }
        self->ready = true;
        self->backend->updateSettings();
        self->backend->updateGeometry();
        self->backend->setBackground(self->owner->backgroundColor());
        auto pending = std::move(self->pending);
        for (auto& action : pending) {
            if (!views().contains(handle)) break;
            action();
        }
    }
    static int __stdcall navigation(int64_t handle, int type, const char* url, int main) {
        auto self = views().value(handle);
        if (!self) return false;
        auto request = new WebEngineNavigationRequest(self->owner);
        QQmlEngine::setObjectOwnership(request, QQmlEngine::CppOwnership);
        request->url = QUrl(QString::fromUtf8(url));
        request->isMainFrame = main != 0;
        switch (type) {
        case MB_NAVIGATION_TYPE_LINKCLICK: request->navigationType = WebEngineView::LinkClickedNavigation; break;
        case MB_NAVIGATION_TYPE_FORMSUBMITTE: case MB_NAVIGATION_TYPE_FORMRESUBMITT:
            request->navigationType = WebEngineView::FormSubmittedNavigation; break;
        case MB_NAVIGATION_TYPE_BACKFORWARD: request->navigationType = WebEngineView::BackForwardNavigation; break;
        case MB_NAVIGATION_TYPE_RELOAD: request->navigationType = WebEngineView::ReloadNavigation; break;
        default: request->navigationType = WebEngineView::OtherNavigation; break;
        }
        QPointer<WebEngineNavigationRequest> guard(request);
        emit self->owner->navigationRequested(request);
        bool accepted = guard && guard->action != WebEngineView::IgnoreRequest;
        if (guard) guard->deleteLater();
        return accepted;
    }
    static void __stdcall started(int64_t handle) {
        post(handle, [](Private& self) { self.owner->didLoad(WebEngineView::LoadStartedStatus); });
    }
    static void __stdcall progress(int64_t handle, int value) {
        post(handle, [value](Private& self) { self.owner->didChangeProgress(qBound(0, value, 100)); });
    }
    static void __stdcall close(int64_t handle) {
        post(handle, [](Private& self) { emit self.owner->windowCloseRequested(); });
    }
    static void MB_CALL_TYPE paint(mbWebView handle, void*, const void* pixels, const mbRect*, int width, int height) {
        if (!pixels || width <= 0 || height <= 0) return;
        QImage copy = QImage(static_cast<const uchar*>(pixels), width, height, QImage::Format_ARGB32_Premultiplied).copy();
        post(handle, [copy = std::move(copy)](Private& self) mutable {
            copy.setDevicePixelRatio(self.dpr);
            self.owner->didCapture(std::move(copy));
        });
    }
    static void MB_CALL_TYPE title(mbWebView handle, void*, const char* value) {
        post(handle, [value = QString::fromUtf8(value)](Private& self) { self.owner->didChangeTitle(value); });
    }
    static void MB_CALL_TYPE url(mbWebView handle, void*, const char* value, BOOL back, BOOL forward) {
        post(handle, [value = QUrl(QString::fromUtf8(value)), back, forward](Private& self) {
            self.back = back; self.forward = forward;
            self.owner->didChangeUrl(value);
            self.owner->didChangeHistory();
        });
    }
    static void MB_CALL_TYPE finished(mbWebView handle, void*, mbWebFrameHandle, const char*, mbLoadingResult result, const char* reason) {
        post(handle, [result, reason = QString::fromUtf8(reason)](Private& self) {
            bool numericError = false;
            int errorCode = reason.toInt(&numericError);
            self.owner->didLoad(result == MB_LOADING_SUCCEEDED ? WebEngineView::LoadSucceededStatus
                : result == MB_LOADING_CANCELED ? WebEngineView::LoadStoppedStatus : WebEngineView::LoadFailedStatus,
                reason, result == MB_LOADING_FAILED ? (numericError ? errorCode : -1) : 0);
            if (result == MB_LOADING_SUCCEEDED && self.owner->settings()->focusOnNavigationEnabled())
                self.owner->forceActiveFocus();
        });
    }
    static void MB_CALL_TYPE console(mbWebView handle, void*, mbConsoleLevel level, const char* message,
        const char* source, unsigned line, const char*) {
        post(handle, [level, message = QString::fromUtf8(message), source = QString::fromUtf8(source), line](Private& self) {
            emit self.owner->javaScriptConsoleMessage(level == mbLevelError ? WebEngineView::ErrorMessageLevel
                : level == mbLevelWarning ? WebEngineView::WarningMessageLevel : WebEngineView::InfoMessageLevel,
                message, int(line), source);
        });
    }
    static BOOL MB_CALL_TYPE resource(mbWebView, void*, const char* rawUrl, void* job) {
        QUrl url(QString::fromUtf8(rawUrl));
        if (url.scheme() != QStringLiteral("qrc")) return false;
        QString path = QDir::cleanPath(url.path());
        QFile file(':' + path);
        if (!url.host().isEmpty() || !path.startsWith('/') || !file.open(QIODevice::ReadOnly)) {
            mbNetCancelRequest(job);
            return true;
        }
        QByteArray data = file.readAll();
        QByteArray mime = QMimeDatabase().mimeTypeForFile(path).name().toUtf8();
        mbNetSetMIMEType(job, mime.constData());
        mbNetSetData(job, data.data(), int(data.size()));
        return true;
    }
    static void MB_CALL_TYPE context(mbWebView handle, void*, mbWebFrameHandle frame, void*, int, int world) {
        qlv_inject_document_script(handle, frame, world);
    }
    static void MB_CALL_TYPE query(mbWebView handle, void*, mbJsExecState, int64_t, int kind, const char* request) {
        auto object = QJsonDocument::fromJson(QByteArray(request)).object();
        post(handle, [object, kind](Private& self) {
            if (kind == 0 && self.owner->webChannel()) {
                emit self.backend->messageReceived(object, self.backend);
            } else if (kind == 1) {
                auto request = new WebEngineContextMenuRequest(self.owner);
                QQmlEngine::setObjectOwnership(request, QQmlEngine::CppOwnership);
                request->position = QPoint(object.value("x").toInt(), object.value("y").toInt());
                request->linkUrl = QUrl(object.value("link").toString());
                request->selectedText = object.value("selection").toString();
                request->isContentEditable = object.value("editable").toBool();
                QPointer<WebEngineContextMenuRequest> guard(request);
                emit self.owner->contextMenuRequested(request);
                if (guard) guard->deleteLater();
            }
        });
    }
    static void MB_CALL_TYPE evaluated(mbWebView handle, void* token, mbJsExecState es, mbJsValue value) {
        // mbJsValue strings are valid only for this callback.
        QByteArray json = value ? QByteArray(mbJsToString(es, value)) : QByteArray();
        auto document = QJsonDocument::fromJson(json);
        QVariant result;
        if (document.isObject() && document.object().value("ok").toBool())
            result = document.object().value("value").toVariant();
        post(handle, [token = reinterpret_cast<quintptr>(token), result](Private& self) {
            auto callback = self.evaluations.take(token);
            if (callback) callback(result);
        });
    }
};

LiteViewBackend::LiteViewBackend(WebEngineView* view)
    : QWebChannelAbstractTransport(view), d(std::make_unique<Private>())
{
    d->backend = this;
    d->owner = view;
    d->handle = mbCreateWebView();
    Private::views().insert(d->handle, d.get());
    // The engine posts WM_CLOSE to its host on destruction. Never give it
    // the application's HWND; this hidden window belongs exclusively to it.
    d->host = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, L"STATIC", L"QLiteView host",
        WS_POPUP, 0, 0, 1, 1, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    mbSetHandle(d->handle, d->host);
    mbSetTransparent(d->handle, TRUE);
    mbOnPaintBitUpdated(d->handle, Private::paint, nullptr);
    mbOnTitleChanged(d->handle, Private::title, nullptr);
    mbOnURLChanged(d->handle, Private::url, nullptr);
    mbOnLoadingFinish(d->handle, Private::finished, nullptr);
    mbOnConsole(d->handle, Private::console, nullptr);
    mbOnLoadUrlBegin(d->handle, Private::resource, nullptr);
    mbOnDidCreateScriptContext(d->handle, Private::context, nullptr);
    mbOnJsQuery(d->handle, Private::query, nullptr);
    qlv_set_callbacks(d->handle, Private::navigation, Private::started, Private::progress, Private::close);
    updateChannel();
    auto profile = view->profile();
    qlv_set_user_agent(d->handle, profile->httpUserAgent().toUtf8().constData());
    connect(profile, &WebEngineProfile::httpUserAgentChanged, this, [this, profile] {
        qlv_set_user_agent(d->handle, profile->httpUserAgent().toUtf8().constData());
    });
    qlv_prepare_view(d->handle, profile->runtimeStoragePath().toUtf8().constData(),
        profile->runtimeCookiePath().toUtf8().constData(), Private::prepared, nullptr);
}
LiteViewBackend::~LiteViewBackend()
{
    Private::views().remove(d->handle);
    qlv_set_callbacks(d->handle, nullptr, nullptr, nullptr, nullptr);
    if (d->ready) mbDestroyWebView(d->handle);
    // If creation is still pending, prepared() destroys it once safe.
}
void LiteViewBackend::load(const QUrl& url) {
    d->afterReady([this, url] { mbLoadURL(d->handle, url.toEncoded().constData()); });
}
void LiteViewBackend::loadHtml(const QString& html, const QUrl& base) {
    d->afterReady([this, html, base] { mbLoadHtmlWithBaseUrl(d->handle, html.toUtf8().constData(), base.toEncoded().constData()); });
}
void LiteViewBackend::evaluate(const QString& script, std::function<void(QVariant)> callback) {
    d->afterReady([this, script, callback = std::move(callback)]() mutable {
        quintptr token = d->nextEvaluation++;
        if (callback) d->evaluations.insert(token, std::move(callback));
        // The upstream mbJsValue API flattens objects/arrays into strings.
        // Serialize in the page to preserve Qt's JSON-compatible value types.
        QString wrapped = QStringLiteral("(function(){try{return JSON.stringify({ok:true,value:(0,eval)(%1)})}catch(e){console.error(String(e));return JSON.stringify({ok:false})}})()")
            .arg(literal(script));
        mbRunJs(d->handle, reinterpret_cast<mbWebFrameHandle>(mainFrame), wrapped.toUtf8().constData(), false,
            Private::evaluated, reinterpret_cast<void*>(token), nullptr);
    });
}
void LiteViewBackend::sendMessage(const QJsonObject& message) {
    QString json = QString::fromUtf8(QJsonDocument(message).toJson(QJsonDocument::Compact));
    evaluate(QStringLiteral("if(window.qt&&qt.webChannelTransport.onmessage)qt.webChannelTransport.onmessage({data:%1})").arg(literal(json)));
}
void LiteViewBackend::updateChannel() {
    QString script = QString::fromLatin1(channelScript).arg(d->owner->webChannel() ? "true" : "false");
    qlv_set_document_script(d->handle, script.toUtf8().constData());
    if (d->ready) evaluate(script);
}
void LiteViewBackend::updateSettings() {
    if (!d->ready) return;
    auto s = d->owner->settings();
    uint64_t flags = 0;
#define FLAG(setting, bit) if (s->setting()) flags |= bit
    FLAG(javascriptEnabled, QlvJavaScript); FLAG(localStorageEnabled, QlvLocalStorage);
    FLAG(autoLoadImages, QlvImages); FLAG(javascriptCanAccessClipboard, QlvClipboard);
    FLAG(javascriptCanPaste, QlvPaste); FLAG(localContentCanAccessFileUrls, QlvFileAccess);
    FLAG(localContentCanAccessRemoteUrls, QlvRemoteAccess); FLAG(showScrollBars, QlvScrollbars);
    FLAG(javascriptCanOpenWindows, QlvWindows); FLAG(allowRunningInsecureContent, QlvInsecureContent);
    FLAG(dnsPrefetchEnabled, QlvDnsPrefetch); FLAG(hyperlinkAuditingEnabled, QlvHyperlinkAudit);
    FLAG(printElementBackgrounds, QlvPrintBackgrounds); FLAG(spatialNavigationEnabled, QlvSpatialNavigation);
#undef FLAG
    QByteArray encoding = s->defaultTextEncoding().toUtf8();
    QlvSettings settings{flags, encoding.constData()};
    qlv_set_settings(d->handle, &settings);
}
void LiteViewBackend::updateGeometry() {
    if (!d->ready) return;
    d->dpr = d->owner->window() ? d->owner->window()->effectiveDevicePixelRatio() : 1;
    QSize size(qMax(1, qRound(d->owner->width()*d->dpr)), qMax(1, qRound(d->owner->height()*d->dpr)));
    if (size != d->size) {
        d->size = size;
        SetWindowPos(d->host, nullptr, 0, 0, size.width(), size.height(), SWP_NOZORDER | SWP_NOACTIVATE);
        mbResize(d->handle, size.width(), size.height());
    }
    setZoom(d->owner->zoomFactor());
}
void LiteViewBackend::setZoom(double zoom) {
    double scale = zoom*d->dpr;
    if (!d->ready || qFuzzyCompare(d->appliedZoom, scale)) return;
    d->appliedZoom = scale;
    mbSetZoomFactor(d->handle, float(scale));
}
void LiteViewBackend::setBackground(const QColor& color) { if (d->ready) qlv_set_background(d->handle, color.rgba()); }
void LiteViewBackend::navigate(int action) {
    d->afterReady([this, action] {
        switch(action) {
        case WebEngineView::Back: mbGoBack(d->handle); break;
        case WebEngineView::Forward: mbGoForward(d->handle); break;
        case WebEngineView::Reload: qlv_reload(d->handle, false); break;
        case WebEngineView::ReloadAndBypassCache: qlv_reload(d->handle, true); break;
        case WebEngineView::Stop: mbStopLoading(d->handle); d->owner->didLoad(WebEngineView::LoadStoppedStatus); break;
        case WebEngineView::RequestClose: emit d->owner->windowCloseRequested(); break;
        case WebEngineView::SavePage: {
            wchar_t path[32768] = L"page.html";
            OPENFILENAMEW dialog{};
            dialog.lStructSize = sizeof(dialog);
            dialog.hwndOwner = d->owner->window() ? reinterpret_cast<HWND>(d->owner->window()->winId()) : nullptr;
            dialog.lpstrFilter = L"HTML\0*.html\0\0";
            dialog.lpstrFile = path;
            dialog.nMaxFile = 32768;
            dialog.lpstrDefExt = L"html";
            dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
            if (GetSaveFileNameW(&dialog)) d->owner->saveHtml(QUrl::fromLocalFile(QString::fromWCharArray(path)));
            break;
        }
        case WebEngineView::InspectElement: inspect(); break;
        default:
            const char* command = nullptr;
            switch(action) {
#define EDIT(a, c) case WebEngineView::a: command = c; break
            EDIT(Cut, "Cut"); EDIT(Copy, "Copy"); EDIT(Paste, "Paste"); EDIT(Undo, "Undo"); EDIT(Redo, "Redo");
            EDIT(SelectAll, "SelectAll"); EDIT(Unselect, "Unselect"); EDIT(PasteAndMatchStyle, "PasteAndMatchStyle");
            EDIT(ToggleBold, "ToggleBold"); EDIT(ToggleItalic, "ToggleItalic"); EDIT(ToggleUnderline, "ToggleUnderline");
            EDIT(ToggleStrikethrough, "Strikethrough"); EDIT(AlignLeft, "JustifyLeft"); EDIT(AlignRight, "JustifyRight");
            EDIT(AlignCenter, "JustifyCenter"); EDIT(AlignJustified, "JustifyFull"); EDIT(Indent, "Indent"); EDIT(Outdent, "Outdent");
            EDIT(InsertOrderedList, "InsertOrderedList"); EDIT(InsertUnorderedList, "InsertUnorderedList");
            EDIT(ChangeTextDirectionLTR, "MakeTextWritingDirectionLeftToRight"); EDIT(ChangeTextDirectionRTL, "MakeTextWritingDirectionRightToLeft");
#undef EDIT
            default: break;
            }
            if (command) qlv_edit(d->handle, command);
            else d->owner->reportUnsupported(QStringLiteral("WebAction:%1").arg(action));
        }
    });
}
void LiteViewBackend::validateAction(int action, WebEngineAction* target) {
    bool enabled = false;
    switch (action) {
    case WebEngineView::Back: enabled = d->back; break;
    case WebEngineView::Forward: enabled = d->forward; break;
    case WebEngineView::Stop: enabled = d->owner->loading(); break;
    case WebEngineView::Reload: case WebEngineView::ReloadAndBypassCache:
    case WebEngineView::RequestClose: case WebEngineView::SavePage:
    case WebEngineView::Cut: case WebEngineView::Copy: case WebEngineView::Paste:
    case WebEngineView::Undo: case WebEngineView::Redo: case WebEngineView::SelectAll:
    case WebEngineView::Unselect: case WebEngineView::PasteAndMatchStyle:
    case WebEngineView::ToggleBold: case WebEngineView::ToggleItalic:
    case WebEngineView::ToggleUnderline: case WebEngineView::ToggleStrikethrough:
    case WebEngineView::AlignLeft: case WebEngineView::AlignRight:
    case WebEngineView::AlignCenter: case WebEngineView::AlignJustified:
    case WebEngineView::Indent: case WebEngineView::Outdent:
    case WebEngineView::InsertOrderedList: case WebEngineView::InsertUnorderedList:
    case WebEngineView::ChangeTextDirectionLTR: case WebEngineView::ChangeTextDirectionRTL:
        enabled = true; break;
    default: break;
    }
    target->setEnabled(d->ready && enabled);
}
void LiteViewBackend::inspect() { d->owner->reportUnsupported(QStringLiteral("showInspector")); }
void LiteViewBackend::mouse(QMouseEvent* event) {
    if (!d->ready) return;
    UINT message = WM_MOUSEMOVE;
    bool down = event->type() == QEvent::MouseButtonPress;
    bool doubleClick = event->type() == QEvent::MouseButtonDblClick;
    if (event->type() != QEvent::MouseMove) {
        if (event->button() == Qt::LeftButton) message = doubleClick ? WM_LBUTTONDBLCLK : down ? WM_LBUTTONDOWN : WM_LBUTTONUP;
        else if (event->button() == Qt::RightButton) message = doubleClick ? WM_RBUTTONDBLCLK : down ? WM_RBUTTONDOWN : WM_RBUTTONUP;
        else if (event->button() == Qt::MiddleButton) message = doubleClick ? WM_MBUTTONDBLCLK : down ? WM_MBUTTONDOWN : WM_MBUTTONUP;
    }
    mbFireMouseEvent(d->handle, message, qRound(event->position().x()*d->dpr), qRound(event->position().y()*d->dpr),
        mouseFlags(event->modifiers(), event->buttons()));
    event->accept();
}
void LiteViewBackend::hover(QHoverEvent* event) {
    if (!d->ready) return;
    bool leave = event->type() == QEvent::HoverLeave;
    mbFireMouseEvent(d->handle, leave ? WM_MOUSELEAVE : WM_MOUSEMOVE, leave ? -1 : qRound(event->position().x()*d->dpr),
        leave ? -1 : qRound(event->position().y()*d->dpr), mouseFlags(event->modifiers()));
}
void LiteViewBackend::wheel(QWheelEvent* event) {
    if (!d->ready) return;
    mbFireMouseWheelEvent(d->handle, qRound(event->position().x()*d->dpr), qRound(event->position().y()*d->dpr),
        event->angleDelta().y(), mouseFlags(event->modifiers(), event->buttons()));
    event->accept();
}
void LiteViewBackend::key(QKeyEvent* event) {
    if (!d->ready) return;
    unsigned key = event->nativeVirtualKey();
    if (!key && event->key() >= Qt::Key_A && event->key() <= Qt::Key_Z) key = event->key();
    if (!key) {
        switch (event->key()) {
        case Qt::Key_Backspace: key = VK_BACK; break; case Qt::Key_Delete: key = VK_DELETE; break;
        case Qt::Key_Return: case Qt::Key_Enter: key = VK_RETURN; break; case Qt::Key_Tab: key = VK_TAB; break;
        case Qt::Key_Left: key = VK_LEFT; break; case Qt::Key_Right: key = VK_RIGHT; break;
        case Qt::Key_Up: key = VK_UP; break; case Qt::Key_Down: key = VK_DOWN; break;
        case Qt::Key_Escape: key = VK_ESCAPE; break; default: key = event->key() & 0xff; break;
        }
    }
    BOOL system = event->modifiers().testFlag(Qt::AltModifier);
    if (event->type() == QEvent::KeyRelease) mbFireKeyUpEvent(d->handle, key, 0, system);
    else {
        mbFireKeyDownEvent(d->handle, key, event->isAutoRepeat() ? MB_REPEAT : 0, system);
        if (!event->modifiers().testFlag(Qt::ControlModifier) && !system) commitText(event->text());
    }
    event->accept();
}
void LiteViewBackend::commitText(const QString& text) {
    if (d->ready) for (QChar character : text) mbFireKeyPressEvent(d->handle, character.unicode(), 0, FALSE);
}
void LiteViewBackend::focus(bool value) { if (d->ready) { if (value) mbSetFocus(d->handle); else mbKillFocus(d->handle); } }
void LiteViewBackend::capture() { updateGeometry(); }
bool LiteViewBackend::canGoBack() const { return d->back; }
bool LiteViewBackend::canGoForward() const { return d->forward; }
