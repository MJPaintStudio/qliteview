#pragma once
#include <windows.h>
#include <memory>

namespace content {
// Owns a native tooltip on LiteView's UI thread. Renderer notifications can
// arrive on the Blink thread; queued tasks retain State, never the renderer.
class ToolTip {
public:
    ToolTip(bool balloon, double delaySeconds);
    ~ToolTip();
    ToolTip(const ToolTip&) = delete;
    ToolTip& operator=(const ToolTip&) = delete;
    void show(const wchar_t* text, HWND owner);
private:
    struct State;
    std::shared_ptr<State> m_state;
};
}
