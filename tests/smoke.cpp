#include "smoke.h"
#include <QtTest>
#include <QGuiApplication>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QLibrary>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTemporaryDir>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFile>
#include <QResource>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QInputMethodEvent>
#include <QProcess>
#include <QProcessEnvironment>
#include <memory>
#include <windows.h>

QString Smoke::evaluate(const QString& source) {
        if (!QMetaObject::invokeMethod(root,"evaluate",Q_ARG(QString,source)))return {};
        QElapsedTimer timeout;
        timeout.start();
        while(root->property("result").toString()=="pending"&&timeout.elapsed()<10000)QTest::qWait(25);
        return root->property("result").toString();
    }
void Smoke::initTestCase() {
        module.setFileName(QCoreApplication::applicationDirPath()+"/qliteview.dll");
        module.setLoadHints(QLibrary::PreventUnloadHint);
        auto initialize=reinterpret_cast<bool(*)()>(module.resolve("qwebkit_initialize"));
        auto error=reinterpret_cast<const char*(*)()>(module.resolve("qwebkit_last_error"));
        QVERIFY2(initialize,qPrintable(module.errorString()));
        QVERIFY2(initialize(),error?error():"initialize failed");
        if(qEnvironmentVariableIsSet("QWEBKIT_REQUIRE_EMBEDDED")) {
            QFile manifest(":/qliteview/runtime/manifest.json");
            QVERIFY2(manifest.open(QIODevice::ReadOnly),"This DLL does not embed its runtime");
            auto hash=QCryptographicHash::hash(manifest.readAll(),QCryptographicHash::Sha256).toHex();
            QString expected=QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation)
                +"/QLiteView/"+QString::fromLatin1(hash)+"/livicore.dll";
            wchar_t loaded[32768]{};
            QVERIFY(GetModuleFileNameW(GetModuleHandleW(L"livicore.dll"),loaded,32768));
            QCOMPARE(QDir::fromNativeSeparators(QString::fromWCharArray(loaded)).toLower(),expected.toLower());
            qInfo().noquote()<<"Embedded LiteView loaded from"<<expected;
        }
        QQmlComponent component(&engine);
        component.setData(R"QML(
import QtQuick
import QtWebChannel
import QWebKit
Item {
    id: root
    width: 640; height: 480
    property string result: ""
    property string received: ""
    property string openedUrl: ""
    property bool blocked: false
    property int status: -1
    property string error: ""
    function evaluate(code: string) {
        result = "pending"
        web.runJavaScript(code, value => root.result = JSON.stringify(value) ?? "undefined")
    }
    function navigateBlocked() { web.url = "qwebkit-blocked:test" }
    function html(content: string) { web.loadHtml(content, "qrc:/tests/") }
    function htmlWithoutBase(content: string) { web.loadHtml(content) }
    function paymentHtml(content: string) { bridge.onZfbQrcodeChanged(content) }
    QtObject {
        id: bridge
        WebChannel.id: "hsjpro"
        function echo(message: string): string { root.received = message; return "reply:" + message }
        function openUrlExternally(url) { root.openedUrl = url; return true }
        function getDarkMode() { return false }
        function getAppInfo() { return {name: "compatibility-test", version: 1} }
        function getRuntimeSystemInfo() { return {cpuUsagePercent: 12, memory: {usedBytes: 1024, totalBytes: 2048}} }
        signal payloadReceived(var payload)
        signal onZfbQrcodeChanged(var data)
        function onReady() { root.received = "captcha-ready" }
        function onCaptchaError(message) { root.received = "captcha-error" }
        function onCaptchaResult(data) { root.received = "captcha-result" }
        function echoPayload(payload) { payloadReceived(payload); return payload }
    }
    WebChannel { id: channel; registeredObjects: [bridge] }
    WebEngineView {
        id: web
        objectName: "web"
        anchors.fill: parent
        webChannel: channel
        settings.localStorageEnabled: true
        settings.javascriptEnabled: true
        settings.accelerated2dCanvasEnabled: true
        settings.allowGeolocationOnInsecureOrigins: true
        settings.allowRunningInsecureContent: true
        settings.allowWindowActivationFromJavaScript: true
        settings.autoLoadIconsForPage: true
        settings.autoLoadImages: true
        settings.defaultTextEncoding: "UTF-8"
        settings.dnsPrefetchEnabled: true
        settings.errorPageEnabled: true
        settings.focusOnNavigationEnabled: true
        settings.hyperlinkAuditingEnabled: true
        settings.javascriptCanAccessClipboard: true
        settings.javascriptCanOpenWindows: true
        settings.javascriptCanPaste: true
        settings.linksIncludedInFocusChain: true
        settings.localContentCanAccessFileUrls: true
        settings.localContentCanAccessRemoteUrls: true
        settings.pdfViewerEnabled: true
        settings.playbackRequiresUserGesture: true
        settings.pluginsEnabled: true
        settings.printElementBackgrounds: true
        settings.screenCaptureEnabled: true
        settings.showScrollBars: true
        settings.spatialNavigationEnabled: true
        settings.touchIconsEnabled: true
        settings.webGLEnabled: true
        settings.webRTCPublicInterfacesOnly: true
        onLoadingChanged: request => { root.status = request.status; root.error = request.errorString }
        onJavaScriptConsoleMessage: (level, message) => console.log("PAGE", level, message)
        onNavigationRequested: request => {
            if (request.url.toString().startsWith("qwebkit-blocked:")) { request.reject(); root.blocked = true }
        }
    }
    Rectangle { x: 500; y: 0; width: 100; height: 100; color: "#00ff00" }
})QML",QUrl("qrc:/tests/Fixture.qml"));
        QVERIFY2(component.isReady(),qPrintable(component.errorString()));
        root=qobject_cast<QQuickItem*>(component.create());
        QVERIFY2(root,qPrintable(component.errorString()));
        root->setParentItem(window.contentItem());
        window.resize(640,480);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        web=root->findChild<QObject*>("web");
        QVERIFY(web);
        QString html=QString::fromUtf8(R"HTML(<!doctype html><meta charset="utf-8"><title>WebKit smoke</title>
<style>html,body{margin:0;background:#ff0000}input{margin:110px 10px}</style>
<script src="qrc:///qtwebchannel/qwebchannel.js"></script><input id="input"><script>
window.bridgeResult='pending';
new QWebChannel(qt.webChannelTransport,c=>{window.bridge=c.objects.hsjpro;bridge.echo('hello',r=>window.bridgeResult=r)});
</script>)HTML");
        QVERIFY(QMetaObject::invokeMethod(root,"html",Q_ARG(QString,html)));
        QTRY_VERIFY_WITH_TIMEOUT(root->property("status").toInt()==2||root->property("status").toInt()==3,30000);
        QCOMPARE(root->property("error").toString(),QString());
        QCOMPARE(root->property("status").toInt(),2);
    }
void Smoke::javascriptAndTitle() {
        QTRY_COMPARE_WITH_TIMEOUT(web->property("title").toString(),QStringLiteral("WebKit smoke"),10000);
        QCOMPARE(evaluate("6*7"),QStringLiteral("42"));
        QCOMPARE(evaluate("({engine:navigator.userAgent.includes('AppleWebKit'),value:[1,true,'中文']})"),
            QString::fromUtf8("{\"engine\":true,\"value\":[1,true,\"中文\"]}"));
        QCOMPARE(evaluate("document.createElement('canvas').getContext('webgl') === null"),QStringLiteral("true"));
    }
void Smoke::channelAndQrc() {
        QTRY_COMPARE_WITH_TIMEOUT(root->property("received").toString(),QStringLiteral("hello"),10000);
        QCOMPARE(evaluate("window.bridgeResult"),QStringLiteral("\"reply:hello\""));
    }
void Smoke::navigationRejection() {
        QVERIFY(QMetaObject::invokeMethod(root,"navigateBlocked"));
        QTRY_VERIFY_WITH_TIMEOUT(root->property("blocked").toBool(),10000);
        QCOMPARE(evaluate("document.title"),QStringLiteral("\"WebKit smoke\""));
    }
void Smoke::composition() {
        evaluate("setTimeout(()=>window.timerSeen=true,20);requestAnimationFrame(()=>window.rafSeen=true)");
        QTest::qWait(500);
        QCOMPARE(evaluate("!!window.timerSeen && !!window.rafSeen"),QStringLiteral("true"));
        auto image=window.grabWindow();
        QVERIFY(!image.isNull());
        image.save(QCoreApplication::applicationDirPath()+"/smoke-render.png");
        double scale=image.width()/640.0;
        QColor page=image.pixelColor(qRound(40*scale),qRound(40*scale));
        QColor overlay=image.pixelColor(qRound(550*scale),qRound(40*scale));
        QVERIFY2(page.red()>220&&page.green()<40,qPrintable(page.name(QColor::HexArgb)));
        QVERIFY2(overlay.green()>220&&overlay.red()<40,qPrintable(overlay.name(QColor::HexArgb)));
    }
void Smoke::keyboardInput() {
        QTest::mouseClick(&window,Qt::LeftButton,Qt::NoModifier,QPoint(80,120));
        for(char c:QByteArray("webkit"))QTest::keyClick(&window,Qt::Key(Qt::Key_A+c-'a'));
        QTRY_COMPARE_WITH_TIMEOUT(evaluate("document.getElementById('input').value"),QStringLiteral("\"webkit\""),5000);
        QInputMethodEvent commit;
        commit.setCommitString(QStringLiteral("中文"));
        QCoreApplication::sendEvent(web,&commit);
        QTRY_COMPARE_WITH_TIMEOUT(evaluate("document.getElementById('input').value"),QStringLiteral("\"webkit中文\""),5000);
    }
void Smoke::transparency() {
        window.setColor(Qt::blue);
        QVERIFY(web->setProperty("backgroundColor",QColor(Qt::transparent)));
        QVERIFY(QMetaObject::invokeMethod(root,"html",Q_ARG(QString,QStringLiteral(
            "<!doctype html><title>Transparent</title><style>html,body{background:transparent;margin:0}</style>"
            "<div style='width:50px;height:50px;background:rgba(255,0,0,.5)'></div>"))));
        QTRY_COMPARE_WITH_TIMEOUT(web->property("title").toString(),QStringLiteral("Transparent"),10000);
        QTest::qWait(300);
        auto image=window.grabWindow();
        double scale=image.width()/640.0;
        auto color=image.pixelColor(qRound(80*scale),qRound(80*scale));
        QVERIFY2(color.blue()>220&&color.red()<40,qPrintable(color.name(QColor::HexArgb)));
        auto mixed=image.pixelColor(qRound(20*scale),qRound(20*scale));
        QVERIFY2(mixed.red()>100&&mixed.red()<155&&mixed.blue()>100&&mixed.blue()<155&&mixed.green()<20,
            qPrintable(mixed.name(QColor::HexArgb)));
        QVERIFY(web->setProperty("backgroundColor",QColor(Qt::white)));
        window.setColor(Qt::white);
    }
void Smoke::httpNetworkProcess() {
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        connect(&server,&QTcpServer::newConnection,&server,[&server]{
            while(auto socket=server.nextPendingConnection()) {
                connect(socket,&QTcpSocket::readyRead,socket,[socket]{
                    auto request=socket->property("request").toByteArray()+socket->readAll();
                    socket->setProperty("request",request);
                    if(!request.contains("\r\n\r\n"))return;
                    QByteArray title=request.startsWith("GET /second ")?"HTTP second":"HTTP smoke";
                    QByteArray body="<!doctype html><title>"+title+"</title><p>network ok</p>";
                    socket->write("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\nContent-Length: "
                        +QByteArray::number(body.size())+"\r\n\r\n"+body);
                    socket->disconnectFromHost();
                });
                connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
            }
        });
        QUrl url(QStringLiteral("http://127.0.0.1:%1/").arg(server.serverPort()));
        QVERIFY(web->setProperty("url",url));
        QTRY_COMPARE_WITH_TIMEOUT(web->property("title").toString(),QStringLiteral("HTTP smoke"),15000);
        QCOMPARE(evaluate("document.body.innerText.trim()"),QStringLiteral("\"network ok\""));
        QCOMPARE(evaluate("localStorage.setItem('qliteview-smoke','stored');localStorage.getItem('qliteview-smoke')"),
            QStringLiteral("\"stored\""));
        url.setPath("/second");
        QVERIFY(web->setProperty("url",url));
        QTRY_COMPARE_WITH_TIMEOUT(web->property("title").toString(),QStringLiteral("HTTP second"),10000);
        QCOMPARE(evaluate("localStorage.getItem('qliteview-smoke')"),QStringLiteral("\"stored\""));
        QVERIFY(web->property("canGoBack").toBool());
        QVERIFY(QMetaObject::invokeMethod(web,"goBack"));
        QTRY_COMPARE_WITH_TIMEOUT(web->property("title").toString(),QStringLiteral("HTTP smoke"),10000);
        QVERIFY(web->property("canGoForward").toBool());
        QVERIFY(QMetaObject::invokeMethod(web,"goForward"));
        QTRY_COMPARE_WITH_TIMEOUT(web->property("title").toString(),QStringLiteral("HTTP second"),10000);
    }
void Smoke::compressedResponses() {
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    connect(&server, &QTcpServer::newConnection, &server, [&server] {
        while (auto socket = server.nextPendingConnection()) {
            connect(socket, &QTcpSocket::readyRead, socket, [socket] {
                auto request = socket->property("request").toByteArray() + socket->readAll();
                socket->setProperty("request", request);
                if (!request.contains("\r\n\r\n")) return;
                const bool script = request.startsWith("GET /script ");
                const bool frame = request.startsWith("GET /frame ");
                const QByteArray body = QByteArray::fromHex(script ? "789c2bcfcc4bc92fd74bcecf2d284a2d2e4e4d094e2eca2c28b13531b2060098eb0a4f" : frame ? "1b4500d09cc703000000f2d5e5f9c3a9841b7b499e9eb69aa1299f3770c881c397504b234828d880138d7c1e1f64e11786ada2404afeb0647e7d0c" : "1f8b080000000000020a2d8c310e02310c04bf62d2a3b4148e1b1a0ae8f84014fbb84817c5724cc1ef39c875b333d2e2897bf18f0aacde36c2269ea1acd986780a6f5fce9740e8d537a16b6f6a328630dc9e8f3bc6a951a1720a6d2ff92581584a67618c4a388a55751856528873ec770711d6c5729323fff957a7a52f1b84a6669b000000");
                socket->write("HTTP/1.1 200 OK\r\nContent-Type: " + QByteArray(script ? "application/javascript" : "text/html; charset=utf-8")
                    + "\r\nContent-Encoding: " + QByteArray(script ? "deflate" : frame ? "br" : "gzip")
                    + "\r\nConnection: close\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                socket->disconnectFromHost();
            });
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        }
    });
    QVERIFY(web->setProperty("url", QUrl(QStringLiteral("http://127.0.0.1:%1/").arg(server.serverPort()))));
    QTRY_COMPARE_WITH_TIMEOUT(web->property("title").toString(), QStringLiteral("Compressed HTML"), 10000);
    QTRY_COMPARE_WITH_TIMEOUT(evaluate("window.compressedScript"), QStringLiteral("42"), 10000);
    QTRY_COMPARE_WITH_TIMEOUT(evaluate("document.querySelector('iframe').contentWindow.compressedFrame"), QStringLiteral("43"), 10000);
}
void Smoke::htmlWithoutBaseUrl() {
        const QString html = "<!doctype html><title>No base URL</title>"
            "<script src='qrc:/qtwebchannel/qwebchannel.js'></script>"
            "<script>window.scriptLoaded = typeof QWebChannel === 'function';</script>";
        QVERIFY(QMetaObject::invokeMethod(root,"htmlWithoutBase",Q_ARG(QString,html)));
        QTRY_COMPARE_WITH_TIMEOUT(web->property("title").toString(),QStringLiteral("No base URL"),10000);
        QCOMPARE(evaluate("window.scriptLoaded"),QStringLiteral("true"));
    }
void Smoke::remotePageChannel() {
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        connect(&server,&QTcpServer::newConnection,&server,[&server]{
            while(auto socket=server.nextPendingConnection()) {
                connect(socket,&QTcpSocket::readyRead,socket,[socket]{
                    auto request=socket->property("request").toByteArray()+socket->readAll();
                    socket->setProperty("request",request);
                    if(!request.contains("\r\n\r\n"))return;
                    const QByteArray body="<!doctype html><title>Remote channel</title>"
                        "<script src='qrc:/qtwebchannel/qwebchannel.js'></script>"
                        "<script>new QWebChannel(qt.webChannelTransport,c=>{window.bridge=c.objects.hsjpro;"
                        "bridge.echo('remote',r=>window.reply=r);});</script>";
                    socket->write("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\nContent-Length: "
                        +QByteArray::number(body.size())+"\r\n\r\n"+body);
                    socket->disconnectFromHost();
                });
                connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
            }
        });
        QVERIFY(web->setProperty("url",QUrl(QStringLiteral("http://127.0.0.1:%1/").arg(server.serverPort()))));
        QTRY_COMPARE_WITH_TIMEOUT(web->property("title").toString(),QStringLiteral("Remote channel"),10000);
        QTRY_COMPARE_WITH_TIMEOUT(evaluate("window.reply"),QStringLiteral("\"reply:remote\""),5000);
        evaluate("bridge.payloadReceived.connect(p=>window.signalPayload=p);"
            "bridge.echoPayload({text:'中文',values:[1,true,null]}).then(p=>window.promisePayload=p);"
            "bridge.openUrlExternally('https://example.com/compatibility')");
        QTRY_COMPARE_WITH_TIMEOUT(root->property("openedUrl").toString(),QStringLiteral("https://example.com/compatibility"),5000);
        QTRY_COMPARE_WITH_TIMEOUT(evaluate("JSON.stringify(window.signalPayload)===JSON.stringify(window.promisePayload) && window.promisePayload.text==='中文'"),QStringLiteral("true"),5000);
    }
void Smoke::hostPages() {
        const auto resources=qEnvironmentVariable("QWEBKIT_HOST_RESOURCES");
        if(resources.isEmpty())QSKIP("Set QWEBKIT_HOST_RESOURCES to test the host's actual HTML/JS resources");
        QVERIFY(QResource::registerResource(resources));
        const auto desktopPage=qEnvironmentVariable("QWEBKIT_TEST_HTML");
        QVERIFY(web->setProperty("url",QUrl::fromLocalFile(desktopPage)));
        QTRY_COMPARE_WITH_TIMEOUT(evaluate("!!window.hsjpro"),QStringLiteral("true"),15000);
        evaluate("document.getElementById('openExternalButton').click();"
            "hsjpro.getAppInfo().then(v=>window.appInfo=v);"
            "document.getElementById('runtimeSystemInfoButton').click()");
        QTRY_COMPARE_WITH_TIMEOUT(root->property("openedUrl").toString(),QStringLiteral("https://www.huashijie.art/"),5000);
        QTRY_COMPARE_WITH_TIMEOUT(evaluate("window.appInfo && window.appInfo.name"),QStringLiteral("\"compatibility-test\""),5000);
        QTRY_COMPARE_WITH_TIMEOUT(evaluate("document.getElementById('runtimeSystemInfoResult').textContent.includes('12%')"),QStringLiteral("true"),5000);
        QVERIFY(web->setProperty("url",QUrl("qrc:/html/zfbpay.html")));
        QTRY_COMPARE_WITH_TIMEOUT(evaluate("!!window.hsjpro && !!document.getElementById('paymentFrame')"),QStringLiteral("true"),10000);
        const QString payment="<!doctype html><p id='ready'>payment-frame-ready</p><script>window.executed=true;</script>";
        QVERIFY(QMetaObject::invokeMethod(root,"paymentHtml",Q_ARG(QString,payment)));
        QTRY_COMPARE_WITH_TIMEOUT(evaluate("document.getElementById('paymentFrame').contentWindow.executed"),QStringLiteral("true"),10000);
        if(qEnvironmentVariableIsSet("QWEBKIT_LIVE_CHECK")) {
            QVERIFY(web->setProperty("url",QUrl("qrc:/html/captcha.html")));
            QTRY_COMPARE_WITH_TIMEOUT(evaluate("typeof initSMCaptcha==='function' && !!window.hsjpro"),QStringLiteral("true"),20000);
            root->setProperty("status", -1);
            QVERIFY(web->setProperty("url",QUrl("https://staticpro.mjing.art/newmarket/")));
            QTRY_COMPARE_WITH_TIMEOUT(root->property("status").toInt(), 2, 45000);
            QTRY_COMPARE_WITH_TIMEOUT(evaluate("!!window.windowsHsjpro"),QStringLiteral("true"),45000);
            root->setProperty("status", -1);
            QVERIFY(web->setProperty("url",QUrl("https://staticpro.mjing.art/hsj/help/")));
            QTRY_COMPARE_WITH_TIMEOUT(root->property("status").toInt(), 2, 45000);
            QTRY_COMPARE_WITH_TIMEOUT(evaluate("!!window.windowsHsjpro"),QStringLiteral("true"),45000);
            QTRY_COMPARE_WITH_TIMEOUT(evaluate("location.pathname === '/hsj/help/' && !!document.querySelector('.problem') && document.querySelector('.problem').innerText.trim().length > 10"),QStringLiteral("true"),45000);
            // Wait for the Vue page and its CSS transition to paint, not just for DOM text.
            auto helpPainted = [&] {
                const auto shot = window.grabWindow();
                int ink = 0;
                for (int y = 0; y < shot.height(); y += 4)
                    for (int x = 0; x < shot.width()/2; x += 4)
                        if (shot.pixelColor(x,y).lightness() < 180) ++ink;
                return ink > 100;
            };
            QTRY_VERIFY_WITH_TIMEOUT(helpPainted(), 10000);
            window.grabWindow().save(QCoreApplication::applicationDirPath()+"/help-live.png");
            evaluate("windowsHsjpro.openUrlExternally('https://example.com/live-channel')");
            QTRY_COMPARE_WITH_TIMEOUT(root->property("openedUrl").toString(),QStringLiteral("https://example.com/live-channel"),5000);
            QFile htmlFile(":/html/wxqrcode.html");
            QVERIFY(htmlFile.open(QIODevice::ReadOnly));
            const QString html=QString::fromUtf8(htmlFile.readAll());
            QVERIFY(web->setProperty("zoomFactor",0.8));
            root->setPosition(QPointF(0.4/window.devicePixelRatio(),0.4/window.devicePixelRatio()));
            QVERIFY(QMetaObject::invokeMethod(root,"htmlWithoutBase",Q_ARG(QString,html)));
            QTRY_COMPARE_WITH_TIMEOUT(evaluate("typeof WxLogin"),QStringLiteral("\"function\""),10000);
            QTRY_COMPARE_WITH_TIMEOUT(evaluate("document.querySelectorAll('#login_container iframe').length"),QStringLiteral("1"),10000);
            // Cross-origin contents cannot be read from the host document. Check
            // the composed iframe image, then verify its image loads directly.
            QTest::qWait(12000);
            window.grabWindow().save(QCoreApplication::applicationDirPath()+"/wechat-iframe.png");
            auto frameUrl=QJsonDocument::fromJson(("["+evaluate("document.querySelector('iframe').src")+"]").toUtf8()).array().first().toString();
            QVERIFY(web->setProperty("url",QUrl(frameUrl)));
            QTRY_COMPARE_WITH_TIMEOUT(evaluate("Array.from(document.images).some(i=>i.src.includes('/connect/qrcode/') && i.complete && i.naturalWidth>100)"),QStringLiteral("true"),45000);
            window.grabWindow().save(QCoreApplication::applicationDirPath()+"/wechat-direct.png");
            root->setPosition(QPointF());
            QVERIFY(web->setProperty("zoomFactor",1.0));
        }
        QResource::unregisterResource(resources);
    }
void Smoke::pixelScale() {
        // Centered dialogs can land between physical pixels at fractional DPI.
        // WebKit's native-resolution bitmap must not be filtered a second time.
        root->setPosition(QPointF(0.4/window.devicePixelRatio(),0.4/window.devicePixelRatio()));
        QVERIFY(web->setProperty("zoomFactor",0.8));
        const QString html=QString::fromUtf8(R"HTML(<!doctype html><meta charset="utf-8">
<style>html,body{margin:0;background:#323232;color:white;font-family:'Microsoft YaHei';}p{margin:20px;font-size:20px}</style>
<canvas id="pattern" width="240" height="80"></canvas><p>微信登录 Clear text 0123456789</p>
<p style="font-size:13px">使用微信扫一扫登录 “画世界Pro”</p><script>
const c=document.getElementById('pattern');c.style.width=240/devicePixelRatio+'px';c.style.height=80/devicePixelRatio+'px';
const ctx=c.getContext('2d');for(let x=0;x<240;x++){ctx.fillStyle=x%2?'white':'black';ctx.fillRect(x,0,1,80)}
window.patternReady=true;
</script>)HTML");
        QVERIFY(QMetaObject::invokeMethod(root,"html",Q_ARG(QString,html)));
        QTRY_COMPARE_WITH_TIMEOUT(evaluate("window.patternReady"),QStringLiteral("true"),10000);
        QTest::qWait(500);
        auto shot=window.grabWindow();
        const auto scale=window.devicePixelRatio();
        shot.save(QCoreApplication::applicationDirPath()+QString("/pixel-scale-%1.png").arg(scale));
        int crisp=0;
        for(int x=10;x<230;++x) {
            int value=shot.pixelColor(x,20).red();
            if(value<8||value>247)++crisp;
        }
        qInfo()<<"DPR"<<scale<<"JS DPR"<<evaluate("devicePixelRatio")<<"crisp pixels"<<crisp<<"/220";
        QVERIFY2(qAbs(evaluate("devicePixelRatio").toDouble() - scale*0.8) < 0.001,
            "Screen updates must preserve Qt DPR and the requested page zoom");
        QVERIFY2(crisp>210,"Physical 1px stripes were resampled in the rendering pipeline");
        // CSS transforms/iframes can switch WebKit to its composited path.
        evaluate("document.body.style.transform='translateZ(0)'");
        QTest::qWait(700);
        shot=window.grabWindow();
        shot.save(QCoreApplication::applicationDirPath()+QString("/pixel-composited-%1.png").arg(scale));
        crisp=0;
        for(int x=10;x<230;++x) {
            int value=shot.pixelColor(x,20).red();
            if(value<8||value>247)++crisp;
        }
        qInfo()<<"Composited DPR"<<scale<<"crisp pixels"<<crisp<<"/220";
        QVERIFY2(crisp>210,"Composited WebKit pixels were scaled twice");
        root->setPosition(QPointF());
        QVERIFY(web->setProperty("zoomFactor",1.0));
    }
void Smoke::lightweightCapabilities() {
        QCOMPARE(evaluate("new WebAssembly.Instance(new WebAssembly.Module(new Uint8Array([0,97,115,109,1,0,0,0,1,5,1,96,0,1,127,3,2,1,0,7,10,1,6,97,110,115,119,101,114,0,0,10,6,1,4,0,65,42,11]))).exports.answer()"),QStringLiteral("42"));
        QCOMPARE(evaluate("Intl.DateTimeFormat.supportedLocalesOf(['zh-CN','en-US','ja-JP','ar']).length"),QStringLiteral("4"));
        QCOMPARE(evaluate("new Intl.NumberFormat('en-US').format(1234567.5)"),QStringLiteral("\"1,234,567.5\""));
        QCOMPARE(evaluate("typeof indexedDB==='object' && typeof Worker==='function' && typeof WebSocket==='function' && typeof fetch==='function' && typeof crypto.subtle==='object'"),QStringLiteral("true"));
        QCOMPARE(evaluate("(()=>{const c=document.createElement('canvas');const g=c.getContext('2d');g.fillStyle='#12ab34';g.fillRect(0,0,1,1);return Array.from(g.getImageData(0,0,1,1).data)})()"),QStringLiteral("[18,171,52,255]"));
        // Exercise optimizing JS tiers without depending on timing thresholds.
        QCOMPARE(evaluate("(()=>{function f(n){let sum=0;for(let i=0;i<n;i++)sum+=i;return sum}let r;for(let j=0;j<500;j++)r=f(10000);return r})()"),QStringLiteral("49995000"));
    }
void Smoke::iframeSrcdoc() {
    QVERIFY(QMetaObject::invokeMethod(root,"html",Q_ARG(QString,QStringLiteral(
        "<!doctype html><title>srcdoc test</title>"
        "<iframe id='allowed' sandbox='allow-scripts allow-same-origin'></iframe>"
        "<iframe id='blocked' sandbox='allow-same-origin'></iframe>"))));
    QTRY_COMPARE_WITH_TIMEOUT(web->property("title").toString(),QStringLiteral("srcdoc test"),10000);
    evaluate("const html='<p id=content>inline</p><script>window.executed=true;</script>';"
        "document.getElementById('allowed').srcdoc=html;document.getElementById('blocked').srcdoc=html");
    QTRY_COMPARE_WITH_TIMEOUT(evaluate("document.getElementById('allowed').contentWindow.executed"),QStringLiteral("true"),10000);
    QTRY_COMPARE_WITH_TIMEOUT(evaluate("document.getElementById('blocked').contentDocument.body.innerText.trim()"),QStringLiteral("\"inline\""),10000);
    QCOMPARE(evaluate("!!document.getElementById('blocked').contentWindow.executed"),QStringLiteral("false"));
    QCOMPARE(evaluate("document.getElementById('allowed').contentDocument.baseURI"),QStringLiteral("\"qrc:/tests/\""));
    evaluate("document.getElementById('allowed').srcdoc='<p>updated</p><script>window.executed=42;</script>'");
    QTRY_COMPARE_WITH_TIMEOUT(evaluate("document.getElementById('allowed').contentWindow.executed"),QStringLiteral("42"),10000);
}
namespace {
std::unique_ptr<QObject> profileView(QQmlEngine& engine, const QVariantMap& properties = {}) {
    QQmlComponent component(&engine);
    component.setData(R"QML(
import QtQuick
import QWebKit
WebEngineView {
    id: view
    width: 320; height: 200
    property string storageDirectory: ""
    property bool privateProfile: true
    property string result: ""
    property int status: -1
    profile: WebEngineProfile {
        offTheRecord: view.privateProfile
        persistentStoragePath: view.storageDirectory
        httpUserAgent: "QLiteView profile test"
    }
    onLoadingChanged: request => status = request.status
    function evaluate(code: string) {
        result = "pending"
        runJavaScript(code, value => result = JSON.stringify(value) ?? "undefined")
    }
    Component.onCompleted: loadHtml("<!doctype html><title>Profile</title>", "http://qliteview.test/")
})QML", QUrl("qrc:/tests/Profile.qml"));
    if (!component.isReady()) qWarning().noquote() << component.errorString();
    return std::unique_ptr<QObject>(component.createWithInitialProperties(properties));
}
QString evaluateView(QObject* view, const QString& source) {
    if (!QMetaObject::invokeMethod(view, "evaluate", Q_ARG(QString, source))) return {};
    QElapsedTimer timer;
    timer.start();
    while (view->property("result").toString() == "pending" && timer.elapsed() < 10000)
        QTest::qWait(10);
    return view->property("result").toString();
}
}
void Smoke::profileIsolation() {
    auto a = profileView(engine);
    auto b = profileView(engine);
    QVERIFY(a && b);
    QTRY_COMPARE_WITH_TIMEOUT(a->property("status").toInt(), 2, 10000);
    QTRY_COMPARE_WITH_TIMEOUT(b->property("status").toInt(), 2, 10000);
    QCOMPARE(evaluateView(a.get(), "navigator.userAgent"), QStringLiteral("\"QLiteView profile test\""));
    QCOMPARE(evaluateView(a.get(), "localStorage.setItem('isolated','A');sessionStorage.setItem('session','A');document.cookie='profile=A;path=/';localStorage.getItem('isolated')"), QStringLiteral("\"A\""));
    QCOMPARE(evaluateView(b.get(), "localStorage.getItem('isolated')"), QStringLiteral("null"));
    QCOMPARE(evaluateView(b.get(), "sessionStorage.getItem('session')"), QStringLiteral("null"));
    QCOMPARE(evaluateView(b.get(), "document.cookie"), QStringLiteral("\"\""));
    QCOMPARE(evaluateView(a.get(), "document.cookie"), QStringLiteral("\"profile=A\""));
    a.reset();
    auto fresh = profileView(engine);
    QVERIFY(fresh);
    QTRY_COMPARE_WITH_TIMEOUT(fresh->property("status").toInt(), 2, 10000);
    QCOMPARE(evaluateView(fresh.get(), "localStorage.getItem('isolated')"), QStringLiteral("null"));
}
void Smoke::profilePersistence() {
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const auto inheritedPath = qEnvironmentVariable("QLITEVIEW_STORAGE_TEST_PATH");
    auto view = profileView(engine, {{"storageDirectory", inheritedPath.isEmpty() ? temporary.path() : inheritedPath},
                                    {"privateProfile", false}});
    QVERIFY(view);
    QTRY_COMPARE_WITH_TIMEOUT(view->property("status").toInt(), 2, 10000);
    if (!inheritedPath.isEmpty()) {
        QCOMPARE(evaluateView(view.get(), "localStorage.getItem('persistent')"), QStringLiteral("\"saved\""));
        QCOMPARE(evaluateView(view.get(), "document.cookie"), QStringLiteral("\"persistent=saved\""));
        return;
    }
    QCOMPARE(evaluateView(view.get(), "localStorage.setItem('persistent','saved');document.cookie='persistent=saved;max-age=3600;path=/';localStorage.getItem('persistent')"), QStringLiteral("\"saved\""));
    view.reset();
    QTest::qWait(150);
    QVERIFY(!QDir(temporary.path()+"/localstorage").entryList({"*.qlstorage"}, QDir::Files).isEmpty());
    QProcess child;
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert("QLITEVIEW_STORAGE_TEST_PATH", temporary.path());
    child.setProcessEnvironment(environment);
    const QString resultFile = temporary.path()+"/child-result.txt";
    child.start(QCoreApplication::applicationFilePath(), {"profilePersistence", "-o", resultFile+",txt"});
    QVERIFY(child.waitForStarted());
    QVERIFY(child.waitForFinished(30000));
    QFile result(resultFile);
    QVERIFY(result.open(QIODevice::ReadOnly));
    const QByteArray report = result.readAll();
    QVERIFY2(child.exitStatus() == QProcess::NormalExit && child.exitCode() == 0, report.constData());
}
void Smoke::failedNavigation() {
    QTcpServer unused;
    QVERIFY(unused.listen(QHostAddress::LocalHost));
    const auto port = unused.serverPort();
    unused.close();
    root->setProperty("status", -1);
    QVERIFY(web->setProperty("url", QUrl(QStringLiteral("http://127.0.0.1:%1/failure").arg(port))));
    QTRY_COMPARE_WITH_TIMEOUT(root->property("status").toInt(), 3, 10000);
    QVERIFY(!root->property("error").toString().isEmpty());
    QVERIFY(!web->property("loading").toBool());
}
void Smoke::immediateDestruction() {
    // Covers both pending engine creation and callbacks queued after deletion.
    for (int i = 0; i < 12; ++i) {
        auto view = profileView(engine);
        QVERIFY(view);
        if (i % 2) QTest::qWait(10);
    }
    QTest::qWait(250);
}
void Smoke::untrustedCertificate() {
    QProcess server;
    server.start("python", {QStringLiteral(QLITEVIEW_TEST_SOURCE_DIR "/tls_server.py")});
    QVERIFY(server.waitForStarted());
    QVERIFY(server.waitForReadyRead(5000));
    const auto port = server.readLine().trimmed().toUInt();
    QVERIFY(port > 0);
    root->setProperty("status", -1);
    QVERIFY(web->setProperty("url", QUrl(QStringLiteral("https://127.0.0.1:%1/").arg(port))));
    QTRY_COMPARE_WITH_TIMEOUT(root->property("status").toInt(), 3, 10000);
    QCOMPARE(root->property("error").toString(), QStringLiteral("-202"));
    server.kill();
    QVERIFY(server.waitForFinished(5000));
}
void Smoke::httpsNavigation() {
    const auto url = qEnvironmentVariable("QLITEVIEW_HTTPS_TEST_URL");
    if (url.isEmpty()) QSKIP("Set QLITEVIEW_HTTPS_TEST_URL for live HTTPS validation");
    root->setProperty("status", -1);
    QVERIFY(web->setProperty("url", QUrl(url)));
    QTRY_VERIFY_WITH_TIMEOUT(root->property("status").toInt() == 2 || root->property("status").toInt() == 3, 30000);
    QVERIFY2(root->property("status").toInt() == 2, qPrintable(root->property("error").toString()));
    QCOMPARE(evaluate("location.protocol"), QStringLiteral("\"https:\""));
}
void Smoke::cleanupTestCase() {
        delete root;
        root=nullptr;
        QTest::qWait(100);
    }

int main(int argc,char** argv)
{
    QGuiApplication app(argc,argv);
    QCoreApplication::setApplicationName("qliteview-smoke");
    Smoke test;
    return QTest::qExec(&test,argc,argv);
}
