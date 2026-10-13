#pragma once
#include <QObject>
#include <QtQml/qqmlregistration.h>
#include <QString>
#include <memory>

class WebEngineProfile : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString storageName READ storageName WRITE setStorageName NOTIFY storageNameChanged)
    Q_PROPERTY(bool offTheRecord READ offTheRecord WRITE setOffTheRecord NOTIFY offTheRecordChanged)
    Q_PROPERTY(QString persistentStoragePath READ persistentStoragePath WRITE setPersistentStoragePath NOTIFY persistentStoragePathChanged)
    Q_PROPERTY(QString cachePath READ cachePath WRITE setCachePath NOTIFY cachePathChanged)
    Q_PROPERTY(QString httpUserAgent READ httpUserAgent WRITE setHttpUserAgent NOTIFY httpUserAgentChanged)
public:
    explicit WebEngineProfile(QObject* parent=nullptr);
    ~WebEngineProfile() override;
    static WebEngineProfile* defaultProfile();
    QString storageName() const { return m_name; }
    void setStorageName(const QString& value);
    bool offTheRecord() const { return m_private; }
    void setOffTheRecord(bool value);
    QString persistentStoragePath() const;
    void setPersistentStoragePath(const QString& value);
    QString cachePath() const;
    void setCachePath(const QString& value);
    QString httpUserAgent() const { return m_userAgent; }
    void setHttpUserAgent(const QString& value);
    Q_INVOKABLE void clearHttpCache();
    QString runtimeStoragePath();
    QString runtimeCookiePath();
signals:
    void storageNameChanged();
    void offTheRecordChanged();
    void persistentStoragePathChanged();
    void cachePathChanged();
    void httpUserAgentChanged();
    void clearHttpCacheCompleted();
private:
    void initialize();
    bool canReconfigure() const;
    struct Private;
    std::unique_ptr<Private> d;
    QString m_name;
    QString m_storage;
    QString m_cache;
    QString m_userAgent;
    bool m_private=true;
    bool m_privateExplicit=false;
};
