#pragma once
#include <stdint.h>
#include <string>
namespace blink { class LocalFrame; }
namespace qliteview {
std::string storageProfile(blink::LocalFrame* frame);
bool navigation(int64_t view, int type, const char* url, bool mainFrame);
void loadStarted(int64_t view);
void loadProgress(int64_t view, double progress);
bool requestClose(int64_t view);
std::string userAgent(int64_t view);
}
