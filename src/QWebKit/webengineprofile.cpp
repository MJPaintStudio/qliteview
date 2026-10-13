#include "webengineprofile.h"
#include "qlitebridge.h"
#include <QCoreApplication>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QDir>
#include <QPointer>
#include <QDebug>
#include <QUuid>

struct WebEngineProfile::Private {
    QString storage;
    QString cookies;
};
WebEngineProfile::WebEngineProfile(QObject* parent):QObject(parent) { }
WebEngineProfile::~WebEngineProfile()=default;
WebEngineProfile* WebEngineProfile::defaultProfile()
{
    static QPointer<WebEngineProfile> profile;
    if(!profile)profile=new WebEngineProfile(QCoreApplication::instance());
    return profile;
}
bool WebEngineProfile::canReconfigure() const
{
    if(!d)return true;
    qWarning("QWebKit: configure profile storage before attaching a WebEngineView");
    return false;
}
void WebEngineProfile::setStorageName(const QString& value)
{
    if(m_name==value||!canReconfigure())return;
    m_name=value;
    if(!m_privateExplicit&&m_private==!value.isEmpty()) { m_private=value.isEmpty(); emit offTheRecordChanged(); }
    emit storageNameChanged();
    emit persistentStoragePathChanged();
    emit cachePathChanged();
}
void WebEngineProfile::setOffTheRecord(bool value)
{
    if(!canReconfigure())return;
    m_privateExplicit=true;
    if(m_private==value)return;
    m_private=value;
    emit offTheRecordChanged();
}
QString WebEngineProfile::persistentStoragePath() const
{
    if(!m_storage.isEmpty())return m_storage;
    QString key=QString::fromLatin1(QCryptographicHash::hash(m_name.toUtf8(),QCryptographicHash::Sha256).toHex());
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)+"/QWebKit/profiles/"+key;
}
QString WebEngineProfile::cachePath() const { return m_cache.isEmpty()?persistentStoragePath()+"/cache":m_cache; }
void WebEngineProfile::setPersistentStoragePath(const QString& value)
{ if(m_storage!=value&&canReconfigure()) { m_storage=value; emit persistentStoragePathChanged(); } }
void WebEngineProfile::setCachePath(const QString& value)
{ if(m_cache!=value&&canReconfigure()) { m_cache=value; emit cachePathChanged(); } }
void WebEngineProfile::setHttpUserAgent(const QString& value)
{ if(m_userAgent!=value) { m_userAgent=value; emit httpUserAgentChanged(); } }

void WebEngineProfile::initialize()
{
    if (d) return;
    d = std::make_unique<Private>();
    if (m_private) {
        d->storage = "qliteview-memory:" + QUuid::createUuid().toString(QUuid::WithoutBraces);
        d->cookies = d->storage;
    } else {
        d->storage = persistentStoragePath() + "/localstorage";
        d->cookies = persistentStoragePath() + "/cookies.dat";
        QDir().mkpath(d->storage);
        QDir().mkpath(cachePath());
    }
}
QString WebEngineProfile::runtimeStoragePath() { initialize(); return d->storage; }
QString WebEngineProfile::runtimeCookiePath() { initialize(); return d->cookies; }
void WebEngineProfile::clearHttpCache()
{
    if (!d) { emit clearHttpCacheCompleted(); return; }
    auto* guard = new QPointer<WebEngineProfile>(this);
    qlv_clear_cache([](int64_t, void* user) {
        std::unique_ptr<QPointer<WebEngineProfile>> guard(static_cast<QPointer<WebEngineProfile>*>(user));
        if (*guard) emit (*guard)->clearHttpCacheCompleted();
    }, guard);
}
