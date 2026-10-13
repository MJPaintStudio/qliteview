#pragma once
#include <QObject>
#include <QLibrary>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QQuickItem>

class Smoke : public QObject {
    Q_OBJECT
private:
    QLibrary module;
    QQmlEngine engine;
    QQuickWindow window;
    QQuickItem* root = nullptr;
    QObject* web = nullptr;
    QString evaluate(const QString& source);
private slots:
    void initTestCase();
    void javascriptAndTitle();
    void channelAndQrc();
    void navigationRejection();
    void composition();
    void keyboardInput();
    void transparency();
    void httpNetworkProcess();
    void compressedResponses();
    void htmlWithoutBaseUrl();
    void remotePageChannel();
    void hostPages();
    void pixelScale();
    void lightweightCapabilities();
    void iframeSrcdoc();
    void profileIsolation();
    void profilePersistence();
    void failedNavigation();
    void untrustedCertificate();
    void httpsNavigation();
    void immediateDestruction();
    void cleanupTestCase();
};
