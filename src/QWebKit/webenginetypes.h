#pragma once
#include <QObject>
#include <QPoint>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

class WebEngineNavigationRequest : public QObject {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(QUrl url MEMBER url CONSTANT)
    Q_PROPERTY(bool isMainFrame MEMBER isMainFrame CONSTANT)
    Q_PROPERTY(int navigationType MEMBER navigationType CONSTANT)
    Q_PROPERTY(int action MEMBER action)
public:
    explicit WebEngineNavigationRequest(QObject* parent = nullptr) : QObject(parent) { }
    QUrl url;
    bool isMainFrame = true;
    int navigationType = 5;
    int action = 0;
    Q_INVOKABLE void accept() { action = 0; }
    Q_INVOKABLE void reject() { action = 255; }
};

class WebEngineContextMenuRequest : public QObject {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(bool accepted MEMBER accepted)
    Q_PROPERTY(QPoint position MEMBER position CONSTANT)
    Q_PROPERTY(QUrl linkUrl MEMBER linkUrl CONSTANT)
    Q_PROPERTY(QString selectedText MEMBER selectedText CONSTANT)
    Q_PROPERTY(bool isContentEditable MEMBER isContentEditable CONSTANT)
public:
    explicit WebEngineContextMenuRequest(QObject* parent = nullptr) : QObject(parent) { }
    bool accepted = false;
    QPoint position;
    QUrl linkUrl;
    QString selectedText;
    bool isContentEditable = false;
};

class WebEngineLoadRequest {
    Q_GADGET
    QML_VALUE_TYPE(webEngineLoadRequest)
    Q_PROPERTY(QUrl url MEMBER url)
    Q_PROPERTY(int status MEMBER status)
    Q_PROPERTY(QString errorString MEMBER errorString)
    Q_PROPERTY(int errorCode MEMBER errorCode)
    Q_PROPERTY(int errorDomain MEMBER errorDomain)
public:
    QUrl url;
    int status = 0;
    QString errorString;
    int errorCode = 0;
    int errorDomain = 0;
};
Q_DECLARE_METATYPE(WebEngineLoadRequest)

class WebEngineAction : public QObject {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(bool enabled READ isEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QString text MEMBER text CONSTANT)
    Q_PROPERTY(QString iconName MEMBER iconName CONSTANT)
public:
    explicit WebEngineAction(QObject* parent = nullptr) : QObject(parent) { }
    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled) {
        if (m_enabled == enabled) return;
        m_enabled = enabled;
        emit enabledChanged();
    }
    QString text;
    QString iconName;
    Q_INVOKABLE void trigger() { if (m_enabled) emit triggered(); }
signals:
    void enabledChanged();
    void triggered();
private:
    bool m_enabled = false;
};
