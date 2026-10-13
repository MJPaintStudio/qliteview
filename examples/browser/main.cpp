#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QLibrary>
#include <QDir>
#include <QTimer>
#include <QDebug>
#include <QQuickWindow>
#include <QImage>
#include <QFontDatabase>
#include <QQuickStyle>

int main(int argc,char* argv[])
{
    QGuiApplication app(argc,argv);
    QQuickStyle::setStyle("Basic");
    if (QGuiApplication::platformName() == "offscreen") {
        // The offscreen platform does not enumerate Windows' installed fonts.
        QFontDatabase::addApplicationFont(qEnvironmentVariable("WINDIR") + "/Fonts/msyh.ttc");
        QGuiApplication::setFont(QFont("Microsoft YaHei", 10));
    }
    QCoreApplication::setApplicationName("qliteview-example");
    QCoreApplication::setOrganizationName("QLiteView");
    QLibrary module(QDir(QCoreApplication::applicationDirPath()).filePath("qliteview.dll"));
    module.setLoadHints(QLibrary::PreventUnloadHint);
    using Initialize=bool(*)();
    using LastError=const char*(*)();
    auto initialize=reinterpret_cast<Initialize>(module.resolve("qwebkit_initialize"));
    auto error=reinterpret_cast<LastError>(module.resolve("qwebkit_last_error"));
    if(!initialize||!initialize()) {
        qCritical().noquote()<<(error?QString::fromUtf8(error()):module.errorString());
        return 1;
    }
    QQmlApplicationEngine engine;
    QObject::connect(&engine,&QQmlApplicationEngine::objectCreationFailed,&app,[]{QCoreApplication::exit(2);},Qt::QueuedConnection);
    engine.loadFromModule("QLiteViewExample","Main");
    const int urlArgument=app.arguments().indexOf("--url");
    if(urlArgument>=0 && urlArgument+1<app.arguments().size() && !engine.rootObjects().isEmpty()) {
        auto web=engine.rootObjects().first()->findChild<QObject*>("webView");
        if(web)web->setProperty("url",QUrl::fromUserInput(app.arguments().at(urlArgument+1)));
    }
    if(app.arguments().contains("--smoke-test")) {
        QTimer::singleShot(urlArgument>=0?20000:5000,&app,[&engine,urlArgument]{
            auto window=engine.rootObjects().isEmpty()?nullptr:qobject_cast<QQuickWindow*>(engine.rootObjects().first());
            auto web=window?window->findChild<QObject*>("webView"):nullptr;
            bool loaded=web&&(urlArgument>=0?!web->property("title").toString().isEmpty():web->property("title").toString().startsWith("QLiteView"))
                &&web->property("loadProgress").toInt()==100;
            if(web)qInfo()<<"Page:"<<web->property("url")<<web->property("title")<<"loaded:"<<loaded;
            bool saved=window&&window->grabWindow().save(QCoreApplication::applicationDirPath()+"/example-render.png");
            QCoreApplication::exit(loaded&&saved?0:3);
        });
    }
    return app.exec();
}
