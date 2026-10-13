#pragma once
#include <QObject>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

class WebEngineSettings : public QObject {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(bool localStorageEnabled READ localStorageEnabled WRITE set_localStorageEnabled NOTIFY localStorageEnabledChanged)
    Q_PROPERTY(bool javascriptEnabled READ javascriptEnabled WRITE set_javascriptEnabled NOTIFY javascriptEnabledChanged)
    Q_PROPERTY(bool accelerated2dCanvasEnabled READ accelerated2dCanvasEnabled WRITE set_accelerated2dCanvasEnabled NOTIFY accelerated2dCanvasEnabledChanged)
    Q_PROPERTY(bool allowGeolocationOnInsecureOrigins READ allowGeolocationOnInsecureOrigins WRITE set_allowGeolocationOnInsecureOrigins NOTIFY allowGeolocationOnInsecureOriginsChanged)
    Q_PROPERTY(bool allowRunningInsecureContent READ allowRunningInsecureContent WRITE set_allowRunningInsecureContent NOTIFY allowRunningInsecureContentChanged)
    Q_PROPERTY(bool allowWindowActivationFromJavaScript READ allowWindowActivationFromJavaScript WRITE set_allowWindowActivationFromJavaScript NOTIFY allowWindowActivationFromJavaScriptChanged)
    Q_PROPERTY(bool autoLoadIconsForPage READ autoLoadIconsForPage WRITE set_autoLoadIconsForPage NOTIFY autoLoadIconsForPageChanged)
    Q_PROPERTY(bool autoLoadImages READ autoLoadImages WRITE set_autoLoadImages NOTIFY autoLoadImagesChanged)
    Q_PROPERTY(bool dnsPrefetchEnabled READ dnsPrefetchEnabled WRITE set_dnsPrefetchEnabled NOTIFY dnsPrefetchEnabledChanged)
    Q_PROPERTY(bool errorPageEnabled READ errorPageEnabled WRITE set_errorPageEnabled NOTIFY errorPageEnabledChanged)
    Q_PROPERTY(bool focusOnNavigationEnabled READ focusOnNavigationEnabled WRITE set_focusOnNavigationEnabled NOTIFY focusOnNavigationEnabledChanged)
    Q_PROPERTY(bool hyperlinkAuditingEnabled READ hyperlinkAuditingEnabled WRITE set_hyperlinkAuditingEnabled NOTIFY hyperlinkAuditingEnabledChanged)
    Q_PROPERTY(bool javascriptCanAccessClipboard READ javascriptCanAccessClipboard WRITE set_javascriptCanAccessClipboard NOTIFY javascriptCanAccessClipboardChanged)
    Q_PROPERTY(bool javascriptCanOpenWindows READ javascriptCanOpenWindows WRITE set_javascriptCanOpenWindows NOTIFY javascriptCanOpenWindowsChanged)
    Q_PROPERTY(bool javascriptCanPaste READ javascriptCanPaste WRITE set_javascriptCanPaste NOTIFY javascriptCanPasteChanged)
    Q_PROPERTY(bool linksIncludedInFocusChain READ linksIncludedInFocusChain WRITE set_linksIncludedInFocusChain NOTIFY linksIncludedInFocusChainChanged)
    Q_PROPERTY(bool localContentCanAccessFileUrls READ localContentCanAccessFileUrls WRITE set_localContentCanAccessFileUrls NOTIFY localContentCanAccessFileUrlsChanged)
    Q_PROPERTY(bool localContentCanAccessRemoteUrls READ localContentCanAccessRemoteUrls WRITE set_localContentCanAccessRemoteUrls NOTIFY localContentCanAccessRemoteUrlsChanged)
    Q_PROPERTY(bool pdfViewerEnabled READ pdfViewerEnabled WRITE set_pdfViewerEnabled NOTIFY pdfViewerEnabledChanged)
    Q_PROPERTY(bool playbackRequiresUserGesture READ playbackRequiresUserGesture WRITE set_playbackRequiresUserGesture NOTIFY playbackRequiresUserGestureChanged)
    Q_PROPERTY(bool pluginsEnabled READ pluginsEnabled WRITE set_pluginsEnabled NOTIFY pluginsEnabledChanged)
    Q_PROPERTY(bool printElementBackgrounds READ printElementBackgrounds WRITE set_printElementBackgrounds NOTIFY printElementBackgroundsChanged)
    Q_PROPERTY(bool screenCaptureEnabled READ screenCaptureEnabled WRITE set_screenCaptureEnabled NOTIFY screenCaptureEnabledChanged)
    Q_PROPERTY(bool showScrollBars READ showScrollBars WRITE set_showScrollBars NOTIFY showScrollBarsChanged)
    Q_PROPERTY(bool spatialNavigationEnabled READ spatialNavigationEnabled WRITE set_spatialNavigationEnabled NOTIFY spatialNavigationEnabledChanged)
    Q_PROPERTY(bool touchIconsEnabled READ touchIconsEnabled WRITE set_touchIconsEnabled NOTIFY touchIconsEnabledChanged)
    Q_PROPERTY(bool webGLEnabled READ webGLEnabled WRITE set_webGLEnabled NOTIFY webGLEnabledChanged)
    Q_PROPERTY(bool webRTCPublicInterfacesOnly READ webRTCPublicInterfacesOnly WRITE set_webRTCPublicInterfacesOnly NOTIFY webRTCPublicInterfacesOnlyChanged)
    Q_PROPERTY(QString defaultTextEncoding READ defaultTextEncoding WRITE setDefaultTextEncoding NOTIFY defaultTextEncodingChanged)
public:
    explicit WebEngineSettings(QObject* parent = nullptr) : QObject(parent) { }
    bool localStorageEnabled() const { return m_localStorageEnabled; }
    void set_localStorageEnabled(bool value) {
        if (m_localStorageEnabled == value) return;
        m_localStorageEnabled = value;
        emit localStorageEnabledChanged();
        emit settingChanged(QStringLiteral("localStorageEnabled"));
    }
    bool javascriptEnabled() const { return m_javascriptEnabled; }
    void set_javascriptEnabled(bool value) {
        if (m_javascriptEnabled == value) return;
        m_javascriptEnabled = value;
        emit javascriptEnabledChanged();
        emit settingChanged(QStringLiteral("javascriptEnabled"));
    }
    bool accelerated2dCanvasEnabled() const { return m_accelerated2dCanvasEnabled; }
    void set_accelerated2dCanvasEnabled(bool value) {
        if (m_accelerated2dCanvasEnabled == value) return;
        m_accelerated2dCanvasEnabled = value;
        emit accelerated2dCanvasEnabledChanged();
        emit settingChanged(QStringLiteral("accelerated2dCanvasEnabled"));
    }
    bool allowGeolocationOnInsecureOrigins() const { return m_allowGeolocationOnInsecureOrigins; }
    void set_allowGeolocationOnInsecureOrigins(bool value) {
        if (m_allowGeolocationOnInsecureOrigins == value) return;
        m_allowGeolocationOnInsecureOrigins = value;
        emit allowGeolocationOnInsecureOriginsChanged();
        emit settingChanged(QStringLiteral("allowGeolocationOnInsecureOrigins"));
    }
    bool allowRunningInsecureContent() const { return m_allowRunningInsecureContent; }
    void set_allowRunningInsecureContent(bool value) {
        if (m_allowRunningInsecureContent == value) return;
        m_allowRunningInsecureContent = value;
        emit allowRunningInsecureContentChanged();
        emit settingChanged(QStringLiteral("allowRunningInsecureContent"));
    }
    bool allowWindowActivationFromJavaScript() const { return m_allowWindowActivationFromJavaScript; }
    void set_allowWindowActivationFromJavaScript(bool value) {
        if (m_allowWindowActivationFromJavaScript == value) return;
        m_allowWindowActivationFromJavaScript = value;
        emit allowWindowActivationFromJavaScriptChanged();
        emit settingChanged(QStringLiteral("allowWindowActivationFromJavaScript"));
    }
    bool autoLoadIconsForPage() const { return m_autoLoadIconsForPage; }
    void set_autoLoadIconsForPage(bool value) {
        if (m_autoLoadIconsForPage == value) return;
        m_autoLoadIconsForPage = value;
        emit autoLoadIconsForPageChanged();
        emit settingChanged(QStringLiteral("autoLoadIconsForPage"));
    }
    bool autoLoadImages() const { return m_autoLoadImages; }
    void set_autoLoadImages(bool value) {
        if (m_autoLoadImages == value) return;
        m_autoLoadImages = value;
        emit autoLoadImagesChanged();
        emit settingChanged(QStringLiteral("autoLoadImages"));
    }
    bool dnsPrefetchEnabled() const { return m_dnsPrefetchEnabled; }
    void set_dnsPrefetchEnabled(bool value) {
        if (m_dnsPrefetchEnabled == value) return;
        m_dnsPrefetchEnabled = value;
        emit dnsPrefetchEnabledChanged();
        emit settingChanged(QStringLiteral("dnsPrefetchEnabled"));
    }
    bool errorPageEnabled() const { return m_errorPageEnabled; }
    void set_errorPageEnabled(bool value) {
        if (m_errorPageEnabled == value) return;
        m_errorPageEnabled = value;
        emit errorPageEnabledChanged();
        emit settingChanged(QStringLiteral("errorPageEnabled"));
    }
    bool focusOnNavigationEnabled() const { return m_focusOnNavigationEnabled; }
    void set_focusOnNavigationEnabled(bool value) {
        if (m_focusOnNavigationEnabled == value) return;
        m_focusOnNavigationEnabled = value;
        emit focusOnNavigationEnabledChanged();
        emit settingChanged(QStringLiteral("focusOnNavigationEnabled"));
    }
    bool hyperlinkAuditingEnabled() const { return m_hyperlinkAuditingEnabled; }
    void set_hyperlinkAuditingEnabled(bool value) {
        if (m_hyperlinkAuditingEnabled == value) return;
        m_hyperlinkAuditingEnabled = value;
        emit hyperlinkAuditingEnabledChanged();
        emit settingChanged(QStringLiteral("hyperlinkAuditingEnabled"));
    }
    bool javascriptCanAccessClipboard() const { return m_javascriptCanAccessClipboard; }
    void set_javascriptCanAccessClipboard(bool value) {
        if (m_javascriptCanAccessClipboard == value) return;
        m_javascriptCanAccessClipboard = value;
        emit javascriptCanAccessClipboardChanged();
        emit settingChanged(QStringLiteral("javascriptCanAccessClipboard"));
    }
    bool javascriptCanOpenWindows() const { return m_javascriptCanOpenWindows; }
    void set_javascriptCanOpenWindows(bool value) {
        if (m_javascriptCanOpenWindows == value) return;
        m_javascriptCanOpenWindows = value;
        emit javascriptCanOpenWindowsChanged();
        emit settingChanged(QStringLiteral("javascriptCanOpenWindows"));
    }
    bool javascriptCanPaste() const { return m_javascriptCanPaste; }
    void set_javascriptCanPaste(bool value) {
        if (m_javascriptCanPaste == value) return;
        m_javascriptCanPaste = value;
        emit javascriptCanPasteChanged();
        emit settingChanged(QStringLiteral("javascriptCanPaste"));
    }
    bool linksIncludedInFocusChain() const { return m_linksIncludedInFocusChain; }
    void set_linksIncludedInFocusChain(bool value) {
        if (m_linksIncludedInFocusChain == value) return;
        m_linksIncludedInFocusChain = value;
        emit linksIncludedInFocusChainChanged();
        emit settingChanged(QStringLiteral("linksIncludedInFocusChain"));
    }
    bool localContentCanAccessFileUrls() const { return m_localContentCanAccessFileUrls; }
    void set_localContentCanAccessFileUrls(bool value) {
        if (m_localContentCanAccessFileUrls == value) return;
        m_localContentCanAccessFileUrls = value;
        emit localContentCanAccessFileUrlsChanged();
        emit settingChanged(QStringLiteral("localContentCanAccessFileUrls"));
    }
    bool localContentCanAccessRemoteUrls() const { return m_localContentCanAccessRemoteUrls; }
    void set_localContentCanAccessRemoteUrls(bool value) {
        if (m_localContentCanAccessRemoteUrls == value) return;
        m_localContentCanAccessRemoteUrls = value;
        emit localContentCanAccessRemoteUrlsChanged();
        emit settingChanged(QStringLiteral("localContentCanAccessRemoteUrls"));
    }
    bool pdfViewerEnabled() const { return m_pdfViewerEnabled; }
    void set_pdfViewerEnabled(bool value) {
        if (m_pdfViewerEnabled == value) return;
        m_pdfViewerEnabled = value;
        emit pdfViewerEnabledChanged();
        emit settingChanged(QStringLiteral("pdfViewerEnabled"));
    }
    bool playbackRequiresUserGesture() const { return m_playbackRequiresUserGesture; }
    void set_playbackRequiresUserGesture(bool value) {
        if (m_playbackRequiresUserGesture == value) return;
        m_playbackRequiresUserGesture = value;
        emit playbackRequiresUserGestureChanged();
        emit settingChanged(QStringLiteral("playbackRequiresUserGesture"));
    }
    bool pluginsEnabled() const { return m_pluginsEnabled; }
    void set_pluginsEnabled(bool value) {
        if (m_pluginsEnabled == value) return;
        m_pluginsEnabled = value;
        emit pluginsEnabledChanged();
        emit settingChanged(QStringLiteral("pluginsEnabled"));
    }
    bool printElementBackgrounds() const { return m_printElementBackgrounds; }
    void set_printElementBackgrounds(bool value) {
        if (m_printElementBackgrounds == value) return;
        m_printElementBackgrounds = value;
        emit printElementBackgroundsChanged();
        emit settingChanged(QStringLiteral("printElementBackgrounds"));
    }
    bool screenCaptureEnabled() const { return m_screenCaptureEnabled; }
    void set_screenCaptureEnabled(bool value) {
        if (m_screenCaptureEnabled == value) return;
        m_screenCaptureEnabled = value;
        emit screenCaptureEnabledChanged();
        emit settingChanged(QStringLiteral("screenCaptureEnabled"));
    }
    bool showScrollBars() const { return m_showScrollBars; }
    void set_showScrollBars(bool value) {
        if (m_showScrollBars == value) return;
        m_showScrollBars = value;
        emit showScrollBarsChanged();
        emit settingChanged(QStringLiteral("showScrollBars"));
    }
    bool spatialNavigationEnabled() const { return m_spatialNavigationEnabled; }
    void set_spatialNavigationEnabled(bool value) {
        if (m_spatialNavigationEnabled == value) return;
        m_spatialNavigationEnabled = value;
        emit spatialNavigationEnabledChanged();
        emit settingChanged(QStringLiteral("spatialNavigationEnabled"));
    }
    bool touchIconsEnabled() const { return m_touchIconsEnabled; }
    void set_touchIconsEnabled(bool value) {
        if (m_touchIconsEnabled == value) return;
        m_touchIconsEnabled = value;
        emit touchIconsEnabledChanged();
        emit settingChanged(QStringLiteral("touchIconsEnabled"));
    }
    bool webGLEnabled() const { return m_webGLEnabled; }
    void set_webGLEnabled(bool value) {
        if (m_webGLEnabled == value) return;
        m_webGLEnabled = value;
        emit webGLEnabledChanged();
        emit settingChanged(QStringLiteral("webGLEnabled"));
    }
    bool webRTCPublicInterfacesOnly() const { return m_webRTCPublicInterfacesOnly; }
    void set_webRTCPublicInterfacesOnly(bool value) {
        if (m_webRTCPublicInterfacesOnly == value) return;
        m_webRTCPublicInterfacesOnly = value;
        emit webRTCPublicInterfacesOnlyChanged();
        emit settingChanged(QStringLiteral("webRTCPublicInterfacesOnly"));
    }
    QString defaultTextEncoding() const { return m_encoding; }
    void setDefaultTextEncoding(const QString& value) {
        if (m_encoding == value) return;
        m_encoding = value;
        emit defaultTextEncodingChanged();
        emit settingChanged(QStringLiteral("defaultTextEncoding"));
    }
signals:
    void settingChanged(const QString& name);
    void defaultTextEncodingChanged();
    void localStorageEnabledChanged();
    void javascriptEnabledChanged();
    void accelerated2dCanvasEnabledChanged();
    void allowGeolocationOnInsecureOriginsChanged();
    void allowRunningInsecureContentChanged();
    void allowWindowActivationFromJavaScriptChanged();
    void autoLoadIconsForPageChanged();
    void autoLoadImagesChanged();
    void dnsPrefetchEnabledChanged();
    void errorPageEnabledChanged();
    void focusOnNavigationEnabledChanged();
    void hyperlinkAuditingEnabledChanged();
    void javascriptCanAccessClipboardChanged();
    void javascriptCanOpenWindowsChanged();
    void javascriptCanPasteChanged();
    void linksIncludedInFocusChainChanged();
    void localContentCanAccessFileUrlsChanged();
    void localContentCanAccessRemoteUrlsChanged();
    void pdfViewerEnabledChanged();
    void playbackRequiresUserGestureChanged();
    void pluginsEnabledChanged();
    void printElementBackgroundsChanged();
    void screenCaptureEnabledChanged();
    void showScrollBarsChanged();
    void spatialNavigationEnabledChanged();
    void touchIconsEnabledChanged();
    void webGLEnabledChanged();
    void webRTCPublicInterfacesOnlyChanged();
private:
    QString m_encoding = QStringLiteral("UTF-8");
    bool m_localStorageEnabled = true;
    bool m_javascriptEnabled = true;
    bool m_accelerated2dCanvasEnabled = true;
    bool m_allowGeolocationOnInsecureOrigins = false;
    bool m_allowRunningInsecureContent = false;
    bool m_allowWindowActivationFromJavaScript = false;
    bool m_autoLoadIconsForPage = true;
    bool m_autoLoadImages = true;
    bool m_dnsPrefetchEnabled = false;
    bool m_errorPageEnabled = true;
    bool m_focusOnNavigationEnabled = true;
    bool m_hyperlinkAuditingEnabled = false;
    bool m_javascriptCanAccessClipboard = false;
    bool m_javascriptCanOpenWindows = true;
    bool m_javascriptCanPaste = false;
    bool m_linksIncludedInFocusChain = true;
    bool m_localContentCanAccessFileUrls = true;
    bool m_localContentCanAccessRemoteUrls = false;
    bool m_pdfViewerEnabled = true;
    bool m_playbackRequiresUserGesture = true;
    bool m_pluginsEnabled = false;
    bool m_printElementBackgrounds = true;
    bool m_screenCaptureEnabled = false;
    bool m_showScrollBars = true;
    bool m_spatialNavigationEnabled = false;
    bool m_touchIconsEnabled = false;
    bool m_webGLEnabled = true;
    bool m_webRTCPublicInterfacesOnly = false;
};

