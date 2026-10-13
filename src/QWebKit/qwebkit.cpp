#include "qwebkit.h"
#include "api/core/mb.h"
#include "qlitebridge.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QLockFile>
#include <QSaveFile>
#include <QStandardPaths>
#include <QThread>
#include <QtQml/qqml.h>
#include <windows.h>
#include <delayimp.h>

namespace {
QByteArray lastError;
HMODULE liteviewModule = nullptr;
bool initialized = false;
bool fail(const QString& message) { lastError=message.toUtf8(); return false; }

QString prepareRuntime()
{
#ifdef QWEBKIT_EMBED_RUNTIME
    QFile manifestFile(":/qliteview/runtime/manifest.json");
    if(!manifestFile.open(QIODevice::ReadOnly)) { fail("Embedded runtime manifest is missing"); return {}; }
    auto manifestBytes=manifestFile.readAll();
    auto manifest=QJsonDocument::fromJson(manifestBytes).object();
    auto digest=QCryptographicHash::hash(manifestBytes,QCryptographicHash::Sha256).toHex();
    QString directory=QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation)
        +"/QLiteView/"+QString::fromLatin1(digest);
    if(!QDir().mkpath(directory)) { fail("Cannot create runtime cache: "+directory); return {}; }
    QLockFile lock(directory+"/.extract.lock");
    if(!lock.tryLock(30000)) { fail("Could not lock runtime cache: "+directory); return {}; }
    for(const auto& value:manifest.value("files").toArray()) {
        auto entry=value.toObject();
        QString name=entry.value("path").toString();
        if(name.isEmpty()||QDir::isAbsolutePath(name)||name.contains("..")||name.contains(':')) {
            fail("Invalid embedded runtime path"); return {};
        }
        QString destination=directory+'/'+name;
        QByteArray expected=entry.value("sha256").toString().toLatin1();
        QFile existing(destination);
        if(existing.open(QIODevice::ReadOnly)) {
            QCryptographicHash hash(QCryptographicHash::Sha256);
            hash.addData(&existing);
            if(hash.result().toHex()==expected)continue;
        }
        existing.close();
        QFile source(":/qliteview/runtime/"+name);
        if(!source.open(QIODevice::ReadOnly)) { fail("Missing embedded runtime file: "+name); return {}; }
        QByteArray bytes=source.readAll();
        if(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex()!=expected) {
            fail("Embedded runtime checksum mismatch: "+name); return {};
        }
        QDir().mkpath(QFileInfo(destination).absolutePath());
        QSaveFile target(destination);
        if(!target.open(QIODevice::WriteOnly)||target.write(bytes)!=bytes.size()||!target.commit()) {
            fail("Cannot extract runtime file: "+destination); return {};
        }
    }
    return directory;
#else
    return QString::fromUtf8(QWEBKIT_DEV_RUNTIME_DIR);
#endif
}

FARPROC WINAPI delayLoadHook(unsigned event,PDelayLoadInfo info)
{
    if(event==dliNotePreLoadLibrary && qstrcmp(info->szDll,"livicore.dll")==0)
        return reinterpret_cast<FARPROC>(liteviewModule);
    return nullptr;
}
}

extern "C" const PfnDliHook __pfnDliNotifyHook2 = delayLoadHook;

bool qwebkit_initialize()
{
    if(initialized)return true;
    if(!QCoreApplication::instance()||QThread::currentThread()!=QCoreApplication::instance()->thread())
        return fail("qwebkit_initialize must run on the GUI thread after QGuiApplication construction");
    QString directory=prepareRuntime();
    if(directory.isEmpty())return false;
    QString dll=QDir::toNativeSeparators(directory+"/livicore.dll");
    liteviewModule=LoadLibraryExW(reinterpret_cast<LPCWSTR>(dll.utf16()),nullptr,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if(!liteviewModule)return fail(QStringLiteral("Cannot load %1 (Windows error %2)").arg(dll).arg(GetLastError()));
    g_hMiniblinkMod = liteviewModule;
    mbFillFuncPtr();
    if (!mbCreateWebView || !mbDestroyWebView || !mbRunJs || !mbOnPaintBitUpdated)
        return fail("The bundled LiteView runtime is missing required API exports");
    qlv_start_runtime();
    // C++-only QML types are registered by the DLL's generated module registration.
    qmlRegisterModule("QWebKit",1,0);
    initialized = true;
    lastError.clear();
    return true;
}

const char* qwebkit_last_error() { return lastError.constData(); }
