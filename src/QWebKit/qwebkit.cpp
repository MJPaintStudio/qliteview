#include "qwebkit.h"
#include "api/core/mb.h"
#include "qlitebridge.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QThread>
#include <QtQml/qqml.h>
#include <windows.h>
#include <delayimp.h>

namespace {
QByteArray lastError;
HMODULE liteviewModule = nullptr;
bool initialized = false;
bool fail(const QString& message) { lastError=message.toUtf8(); return false; }

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
    const QString dll = QDir::toNativeSeparators(
        QCoreApplication::applicationDirPath() + QStringLiteral("/livicore.dll"));
    if (!QFileInfo::exists(dll)) {
        return fail(QStringLiteral("Missing LiteView runtime: %1").arg(dll));
    }
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
