#include "content/ui/ToolTip.h"
#include "content/common/ThreadCall.h"
#include <commctrl.h>
#include <string>

namespace content {
struct ToolTip::State {
    HWND window = nullptr;
    TOOLINFOW tool{sizeof(TOOLINFOW)};
    std::wstring text;
    bool balloon = false;
    int delayMs = 0;
};

ToolTip::ToolTip(bool balloon, double delaySeconds) : m_state(std::make_shared<State>())
{
    m_state->balloon = balloon;
    m_state->delayMs = static_cast<int>(delaySeconds * 1000);
}

ToolTip::~ToolTip()
{
    ThreadCall::callUiThreadAsync(MB_FROM_HERE, [state = m_state] {
        if (state->window) {
            DestroyWindow(state->window);
            state->window = nullptr;
        }
    });
}

void ToolTip::show(const wchar_t* text, HWND owner)
{
    ThreadCall::callUiThreadAsync(MB_FROM_HERE,
        [state = m_state, text = std::wstring(text ? text : L""), owner] {
            if (text.empty()) {
                if (state->window)
                    SendMessageW(state->window, TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&state->tool));
                return;
            }
            state->text = text;
            if (!state->window) {
                INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_WIN95_CLASSES};
                InitCommonControlsEx(&controls);
                state->window = CreateWindowExW(WS_EX_TOPMOST | WS_EX_NOACTIVATE,
                    TOOLTIPS_CLASSW, nullptr,
                    WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX | (state->balloon ? TTS_BALLOON : 0),
                    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                    owner, nullptr, GetModuleHandleW(nullptr), nullptr);
                if (!state->window)
                    return;
                state->tool.uFlags = TTF_TRACK | TTF_ABSOLUTE;
                state->tool.hwnd = owner;
                state->tool.uId = 1;
                state->tool.lpszText = state->text.data();
                SendMessageW(state->window, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&state->tool));
                SendMessageW(state->window, TTM_SETMAXTIPWIDTH, 0, 600);
                SendMessageW(state->window, TTM_SETDELAYTIME, TTDT_INITIAL, state->delayMs);
            }
            state->tool.lpszText = state->text.data();
            SendMessageW(state->window, TTM_UPDATETIPTEXTW, 0, reinterpret_cast<LPARAM>(&state->tool));
            POINT cursor{};
            GetCursorPos(&cursor);
            SendMessageW(state->window, TTM_TRACKPOSITION, 0, MAKELPARAM(cursor.x + 12, cursor.y + 20));
            SendMessageW(state->window, TTM_TRACKACTIVATE, TRUE, reinterpret_cast<LPARAM>(&state->tool));
        });
}
}
