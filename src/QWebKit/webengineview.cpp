#include "webengineview.h"
#include "liteviewbackend.h"
#include "qwebkit.h"
#include <QPainter>
#include <QQuickWindow>
#include <QMouseEvent>
#include <QInputMethodEvent>
#include <QSaveFile>
#include <QJSEngine>
#include <QtQml/qqml.h>
#include <cmath>

WebEngineView::WebEngineView(QQuickItem* parent)
    : QQuickPaintedItem(parent), m_settings(this)
{
    // LiteView rasterizes at the window's device pixel ratio. Linear
    // filtering blurs that image when a parent places us between physical
    // pixels (for example, centered dialogs at 125% Windows scaling).
    setSmooth(false);
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);
    setFlag(ItemAcceptsInputMethod, true);
    setActiveFocusOnTab(true);
    m_captureTimer.setInterval(33);
    connect(&m_captureTimer, &QTimer::timeout, this, [this] {
        if (m_backend && isVisible() && window() && window()->isVisible()) m_backend->capture();
    });
    connect(&m_settings, &WebEngineSettings::settingChanged, this, [this](const QString& name) {
        if (m_backend) m_backend->updateSettings();
        if (unsupportedFeatures().contains(name)) reportUnsupported(name);
    });
    connect(this, &QQuickItem::windowChanged, this, [this] { if (m_backend) m_backend->updateGeometry(); });
    connect(this, &QQuickItem::visibleChanged, this, [this] { if (m_backend) m_backend->updateGeometry(); });
}

WebEngineView::~WebEngineView()
{
    m_captureTimer.stop();
    if (m_channel && m_backend) m_channel->disconnectFrom(m_backend.get());
    m_backend.reset();
}

void WebEngineView::componentComplete()
{
    QQuickPaintedItem::componentComplete();
    if (!qwebkit_initialize()) {
        didLoad(LoadFailedStatus, QString::fromUtf8(qwebkit_last_error()), -1);
        qWarning() << "QWebKit:" << qwebkit_last_error();
        return;
    }
    m_backend = std::make_unique<LiteViewBackend>(this);
    if (m_channel) m_channel->connectTo(m_backend.get());
    m_captureTimer.start();
    if (m_hasPendingHtml) m_backend->loadHtml(m_pendingHtml, m_pendingBase);
    else if (!m_url.isEmpty()) m_backend->load(m_url);
}

void WebEngineView::setUrl(const QUrl& value)
{
    if (m_url == value) return;
    m_hasPendingHtml = false;
    m_url = value;
    emit urlChanged();
    if (m_backend && !value.isEmpty()) m_backend->load(value);
}

bool WebEngineView::canGoBack() const { return m_backend && m_backend->canGoBack(); }
bool WebEngineView::canGoForward() const { return m_backend && m_backend->canGoForward(); }

WebEngineProfile* WebEngineView::profile() const
{ return m_profile?m_profile.data():WebEngineProfile::defaultProfile(); }
void WebEngineView::setProfile(WebEngineProfile* value)
{
    if(m_profile==value)return;
    if(m_backend) {
        reportUnsupported(QStringLiteral("changing profile after view creation"));
        return;
    }
    m_profile=value;
    emit profileChanged();
}

void WebEngineView::setZoomFactor(qreal value)
{
    if (!std::isfinite(value) || value < 0.25 || value > 5.0 || qFuzzyCompare(value, m_zoom)) return;
    m_zoom = value;
    if (m_backend) m_backend->setZoom(value);
    emit zoomFactorChanged();
}

void WebEngineView::setBackgroundColor(const QColor& value)
{
    if (value == m_background) return;
    m_background = value;
    if (m_backend) m_backend->setBackground(value);
    update();
    emit backgroundColorChanged();
}

void WebEngineView::setWebChannel(QWebChannel* channel)
{
    if (m_channel == channel) return;
    if (m_backend && m_channel) m_channel->disconnectFrom(m_backend.get());
    m_channel = channel;
    if (m_backend) {
        if (m_channel) m_channel->connectTo(m_backend.get());
        m_backend->updateChannel();
    }
    emit webChannelChanged();
}

void WebEngineView::setDevToolsView(WebEngineView* view)
{
    if (m_devTools == view) return;
    m_devTools = view;
    if (view) reportUnsupported(QStringLiteral("devToolsView"));
    emit devToolsViewChanged();
}

QStringList WebEngineView::unsupportedFeatures() const
{
    return {"webGLEnabled", "accelerated2dCanvasEnabled", "playbackRequiresUserGesture",
        "screenCaptureEnabled", "webRTCPublicInterfacesOnly", "pluginsEnabled", "pdfViewerEnabled",
        "allowGeolocationOnInsecureOrigins", "allowWindowActivationFromJavaScript", "autoLoadIconsForPage",
        "touchIconsEnabled", "errorPageEnabled", "devToolsView", "showInspector"};
}

void WebEngineView::loadHtml(const QString& html, const QUrl& base)
{
    m_hasPendingHtml = true;
    m_pendingHtml = html;
    m_pendingBase = base;
    if (m_backend) m_backend->loadHtml(html, base);
}

void WebEngineView::runJavaScript(const QString& script, const QJSValue& callback)
{
    if (!m_backend) { if (callback.isCallable()) callback.call({QJSValue()}); return; }
    QPointer<WebEngineView> guard(this);
    m_backend->evaluate(script, [guard, callback](QVariant result) {
        if (!guard || !callback.isCallable()) return;
        auto engine = qjsEngine(guard);
        callback.call({engine ? engine->toScriptValue(result) : QJSValue()});
    });
}

void WebEngineView::goBack() { triggerWebAction(Back); }
void WebEngineView::goForward() { triggerWebAction(Forward); }
void WebEngineView::reload() { triggerWebAction(Reload); }
void WebEngineView::reloadAndBypassCache() { triggerWebAction(ReloadAndBypassCache); }
void WebEngineView::stop() { triggerWebAction(Stop); }
void WebEngineView::showInspector() { if (m_backend) m_backend->inspect(); }

WebEngineAction* WebEngineView::action(WebAction value)
{
    if (auto existing = m_actions.value(value)) return existing;
    auto item = new WebEngineAction(this);
    const QHash<int, QString> names {{Back,"Back"},{Forward,"Forward"},{Stop,"Stop"},{Reload,"Reload"},
        {Cut,"Cut"},{Copy,"Copy"},{Paste,"Paste"},{Undo,"Undo"},{Redo,"Redo"},{SelectAll,"Select all"},
        {SavePage,"Save page"},{InspectElement,"Inspect"}};
    item->text = names.value(value, QStringLiteral("Unsupported action"));
    const QHash<int, QString> icons {{Back,"go-previous"},{Forward,"go-next"},{Reload,"view-refresh"},
        {Cut,"edit-cut"},{Copy,"edit-copy"},{Paste,"edit-paste"},{SavePage,"document-save"}};
    item->iconName = icons.value(value);
    connect(item, &WebEngineAction::triggered, this, [this,value] { triggerWebAction(value); });
    m_actions.insert(value, item);
    didChangeHistory();
    return item;
}

void WebEngineView::triggerWebAction(WebAction value)
{
    if (!m_backend) return;
    if (value == InspectElement) { showInspector(); return; }
    if (value == RequestClose) { emit windowCloseRequested(); return; }
    m_backend->navigate(value);
}

void WebEngineView::saveHtml(const QUrl& destination)
{
    if (!m_backend || !destination.isLocalFile()) { emit htmlSaved(destination, false); return; }
    QPointer<WebEngineView> guard(this);
    m_backend->evaluate(QStringLiteral("'<!doctype html>\\n' + document.documentElement.outerHTML"),
        [guard, destination](const QVariant& result) {
            if (!guard) return;
            QSaveFile file(destination.toLocalFile());
            auto data = result.toString().toUtf8();
            bool success = result.isValid() && file.open(QIODevice::WriteOnly)
                && file.write(data) == data.size() && file.commit();
            emit guard->htmlSaved(destination, success);
        });
}

void WebEngineView::didChangeUrl(const QUrl& value) { if (m_url != value) { m_url = value; emit urlChanged(); } }
void WebEngineView::didChangeTitle(const QString& value) { if (m_title != value) { m_title = value; emit titleChanged(); } }
void WebEngineView::didChangeProgress(int value) { if (m_progress != value) { m_progress = value; emit loadProgressChanged(); } }
void WebEngineView::didLoad(LoadStatus status, const QString& error, int code)
{
    bool loading = status == LoadStartedStatus;
    if (loading != m_loading) { m_loading = loading; emit loadingStateChanged(); }
    if (loading) didChangeProgress(0);
    else if (status == LoadSucceededStatus) didChangeProgress(100);
    WebEngineLoadRequest request {m_url, status, error, code, code ? 1 : 0};
    emit loadingChanged(request);
    didChangeHistory();
}
void WebEngineView::didChangeHistory()
{
    emit canGoBackChanged();
    emit canGoForwardChanged();
    for (auto it = m_actions.begin(); it != m_actions.end(); ++it) {
        int a = it.key();
        bool enabled = a == Back ? canGoBack() : a == Forward ? canGoForward() :
            a == Stop ? m_loading : (a >= Reload && a <= PasteAndMatchStyle) || a == InspectElement || a == SavePage;
        it.value()->setEnabled(m_backend && enabled);
        if(m_backend)m_backend->validateAction(a,it.value());
    }
}
void WebEngineView::didCapture(QImage image) { m_image = std::move(image); update(); }
void WebEngineView::reportUnsupported(const QString& feature)
{
    if (m_reported.contains(feature)) return;
    m_reported.insert(feature);
    qWarning().noquote() << "QWebKit: unsupported or disabled:" << feature;
    emit unsupportedFeatureRequested(feature);
}
void WebEngineView::paint(QPainter* painter)
{
    painter->fillRect(boundingRect(), m_background);
    if (!m_image.isNull()) painter->drawImage(boundingRect(), m_image);
}
void WebEngineView::geometryChange(const QRectF& current, const QRectF& previous)
{ QQuickPaintedItem::geometryChange(current,previous); if (m_backend) m_backend->updateGeometry(); }
void WebEngineView::mousePressEvent(QMouseEvent* e) { forceActiveFocus(Qt::MouseFocusReason); if(m_backend)m_backend->mouse(e); }
void WebEngineView::mouseReleaseEvent(QMouseEvent* e) { if(m_backend)m_backend->mouse(e); }
void WebEngineView::mouseMoveEvent(QMouseEvent* e) { if(m_backend)m_backend->mouse(e); }
void WebEngineView::mouseDoubleClickEvent(QMouseEvent* e) { if(m_backend)m_backend->mouse(e); }
void WebEngineView::hoverMoveEvent(QHoverEvent* e) { if(m_backend)m_backend->hover(e); }
void WebEngineView::hoverLeaveEvent(QHoverEvent* e) { if(m_backend)m_backend->hover(e); }
void WebEngineView::wheelEvent(QWheelEvent* e) { if(m_backend)m_backend->wheel(e); }
void WebEngineView::keyPressEvent(QKeyEvent* e) { if(m_backend)m_backend->key(e); }
void WebEngineView::keyReleaseEvent(QKeyEvent* e) { if(m_backend)m_backend->key(e); }
void WebEngineView::focusInEvent(QFocusEvent* e) { QQuickPaintedItem::focusInEvent(e); if(m_backend)m_backend->focus(true); }
void WebEngineView::focusOutEvent(QFocusEvent* e) { if(m_backend)m_backend->focus(false); QQuickPaintedItem::focusOutEvent(e); }
void WebEngineView::inputMethodEvent(QInputMethodEvent* e)
{ if(m_backend) m_backend->commitText(e->commitString()); e->accept(); }
QVariant WebEngineView::inputMethodQuery(Qt::InputMethodQuery query) const
{
    if (query == Qt::ImEnabled) return true;
    if (query == Qt::ImCursorRectangle) return QRectF(0, height() - 24, 1, 24);
    return QQuickPaintedItem::inputMethodQuery(query);
}
