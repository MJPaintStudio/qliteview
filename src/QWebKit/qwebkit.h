#pragma once

#if defined(QWEBKIT_BUILD)
#define QWEBKIT_EXPORT __declspec(dllexport)
#else
#define QWEBKIT_EXPORT __declspec(dllimport)
#endif

// Call on the GUI thread after constructing QGuiApplication, before loading QML.
// A host may use QLibrary/GetProcAddress and this C entry point without a .lib.
extern "C" QWEBKIT_EXPORT bool qwebkit_initialize();
extern "C" QWEBKIT_EXPORT const char* qwebkit_last_error();
