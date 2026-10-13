#pragma once
#include "webenginesettings.h"
#include "webenginetypes.h"
#include "webengineprofile.h"
#include <QQuickPaintedItem>
#include <QImage>
#include <QJSValue>
#include <QPointer>
#include <QWebChannel>
#include <QTimer>
#include <memory>

class LiteViewBackend;

class WebEngineView : public QQuickPaintedItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged)
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingStateChanged)
    Q_PROPERTY(int loadProgress READ loadProgress NOTIFY loadProgressChanged)
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY canGoBackChanged)
    Q_PROPERTY(bool canGoForward READ canGoForward NOTIFY canGoForwardChanged)
    Q_PROPERTY(qreal zoomFactor READ zoomFactor WRITE setZoomFactor NOTIFY zoomFactorChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)
    Q_PROPERTY(WebEngineSettings* settings READ settings CONSTANT)
    Q_PROPERTY(WebEngineProfile* profile READ profile WRITE setProfile NOTIFY profileChanged)
    Q_PROPERTY(QWebChannel* webChannel READ webChannel WRITE setWebChannel NOTIFY webChannelChanged)
    Q_PROPERTY(WebEngineView* devToolsView READ devToolsView WRITE setDevToolsView NOTIFY devToolsViewChanged)
    Q_PROPERTY(QString backendName READ backendName CONSTANT)
    Q_PROPERTY(QStringList unsupportedFeatures READ unsupportedFeatures CONSTANT)
public:
    explicit WebEngineView(QQuickItem* parent = nullptr);
    ~WebEngineView() override;
    enum WebAction {
        NoWebAction = -1, Back, Forward, Stop, Reload, Cut, Copy, Paste, Undo, Redo,
        SelectAll, ReloadAndBypassCache, PasteAndMatchStyle, OpenLinkInThisWindow,
        OpenLinkInNewWindow, OpenLinkInNewTab, CopyLinkToClipboard, DownloadLinkToDisk,
        CopyImageToClipboard, CopyImageUrlToClipboard, DownloadImageToDisk,
        CopyMediaUrlToClipboard, ToggleMediaControls, ToggleMediaLoop, ToggleMediaPlayPause,
        ToggleMediaMute, DownloadMediaToDisk, InspectElement, ExitFullScreen, RequestClose,
        Unselect, SavePage, OpenLinkInNewBackgroundTab, ViewSource, ToggleBold, ToggleItalic,
        ToggleUnderline, ToggleStrikethrough, AlignLeft, AlignCenter, AlignRight, AlignJustified,
        Indent, Outdent, InsertOrderedList, InsertUnorderedList, ChangeTextDirectionLTR,
        ChangeTextDirectionRTL, WebActionCount
    };
    Q_ENUM(WebAction)
    enum LoadStatus { LoadStartedStatus, LoadStoppedStatus, LoadSucceededStatus, LoadFailedStatus };
    Q_ENUM(LoadStatus)
    enum NavigationType { LinkClickedNavigation, TypedNavigation, FormSubmittedNavigation,
        BackForwardNavigation, ReloadNavigation, OtherNavigation, RedirectNavigation };
    Q_ENUM(NavigationType)
    enum NavigationRequestAction { AcceptRequest = 0, IgnoreRequest = 255 };
    Q_ENUM(NavigationRequestAction)
    enum JavaScriptConsoleMessageLevel { InfoMessageLevel, WarningMessageLevel, ErrorMessageLevel };
    Q_ENUM(JavaScriptConsoleMessageLevel)

    QUrl url() const { return m_url; }
    void setUrl(const QUrl& url);
    QString title() const { return m_title; }
    bool loading() const { return m_loading; }
    int loadProgress() const { return m_progress; }
    bool canGoBack() const;
    bool canGoForward() const;
    qreal zoomFactor() const { return m_zoom; }
    void setZoomFactor(qreal value);
    QColor backgroundColor() const { return m_background; }
    void setBackgroundColor(const QColor& color);
    WebEngineSettings* settings() { return &m_settings; }
    WebEngineProfile* profile() const;
    void setProfile(WebEngineProfile* profile);
    QWebChannel* webChannel() const { return m_channel; }
    void setWebChannel(QWebChannel* channel);
    WebEngineView* devToolsView() const { return m_devTools; }
    void setDevToolsView(WebEngineView* view);
    QString backendName() const { return QStringLiteral("LiteView (Windows x64)"); }
    QStringList unsupportedFeatures() const;

    Q_INVOKABLE void loadHtml(const QString& html, const QUrl& baseUrl = QUrl("about:blank"));
    Q_INVOKABLE void runJavaScript(const QString& script, const QJSValue& callback = QJSValue());
    Q_INVOKABLE void goBack();
    Q_INVOKABLE void goForward();
    Q_INVOKABLE void reload();
    Q_INVOKABLE void reloadAndBypassCache();
    Q_INVOKABLE void stop();
    Q_INVOKABLE WebEngineAction* action(WebEngineView::WebAction action);
    Q_INVOKABLE void triggerWebAction(WebEngineView::WebAction action);
    Q_INVOKABLE void showInspector();
    Q_INVOKABLE void saveHtml(const QUrl& destination);
    void paint(QPainter* painter) override;

    // UI-thread callbacks from the WebKit C API.
    void didChangeUrl(const QUrl& url);
    void didChangeTitle(const QString& title);
    void didChangeProgress(int progress);
    void didLoad(LoadStatus status, const QString& error = {}, int code = 0);
    void didChangeHistory();
    void didCapture(QImage image);
    void reportUnsupported(const QString& feature);
signals:
    void urlChanged();
    void titleChanged();
    void loadingStateChanged();
    void loadProgressChanged();
    void canGoBackChanged();
    void canGoForwardChanged();
    void zoomFactorChanged();
    void backgroundColorChanged();
    void webChannelChanged();
    void profileChanged();
    void devToolsViewChanged();
    void loadingChanged(WebEngineLoadRequest loadRequest);
    void navigationRequested(WebEngineNavigationRequest* request);
    void contextMenuRequested(WebEngineContextMenuRequest* request);
    void javaScriptConsoleMessage(WebEngineView::JavaScriptConsoleMessageLevel level, const QString& message,
        int lineNumber, const QString& sourceID);
    void windowCloseRequested();
    void unsupportedFeatureRequested(const QString& feature);
    void htmlSaved(const QUrl& destination, bool success);
protected:
    void componentComplete() override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void hoverMoveEvent(QHoverEvent* event) override;
    void hoverLeaveEvent(QHoverEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void inputMethodEvent(QInputMethodEvent* event) override;
    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;
private:
    std::unique_ptr<LiteViewBackend> m_backend;
    WebEngineSettings m_settings;
    QPointer<QWebChannel> m_channel;
    QPointer<WebEngineProfile> m_profile;
    QPointer<WebEngineView> m_devTools;
    QHash<int, WebEngineAction*> m_actions;
    QSet<QString> m_reported;
    QTimer m_captureTimer;
    QImage m_image;
    QUrl m_url;
    QString m_title;
    QString m_pendingHtml;
    QUrl m_pendingBase;
    bool m_hasPendingHtml = false;
    bool m_loading = false;
    int m_progress = 0;
    qreal m_zoom = 1.0;
    QColor m_background = Qt::white;
};
