#pragma once

#define NOMINMAX
#include <Windows.h>
#include <Dwmapi.h>

#pragma comment (lib, "Dwmapi.lib")

namespace win32 {
namespace window {

inline bool enableBackdropBlur(const ::HWND window) noexcept {
    const ::MARGINS margins{ .cxLeftWidth{ -1 } };
    if (::DwmExtendFrameIntoClientArea(window, &margins) != S_OK)
        return false;

    const ::HMODULE user32Dll{ ::LoadLibraryA("user32.dll") };
    if (!user32Dll)
        return false;

    enum class WINDOWCOMPOSITIONATTRIB {
        WCA_ACCENT_POLICY = 19,
    };

    struct WINDOWCOMPOSITIONATTRIBDATA {
        WINDOWCOMPOSITIONATTRIB Attrib{};
        void* pvData{};
        ::UINT cbData{};
    };
    using t_SetWindowCompositionAttribute = ::BOOL(WINAPI*)(::HWND, const WINDOWCOMPOSITIONATTRIBDATA*);

    const void* funcAddress{ ::GetProcAddress(user32Dll, "SetWindowCompositionAttribute") };
    const auto SetWindowCompositionAttribute{ reinterpret_cast<t_SetWindowCompositionAttribute>(funcAddress) };
    if (!SetWindowCompositionAttribute) {
        ::FreeLibrary(user32Dll);
        return false;
    }

    struct ACCENT_POLICY {
        int nAccentState{};
        int nFlags{};
        int nColor{};
        int nAnimationId{};
    };
    constexpr int ACCENT_ENABLE_BLURBEHIND{ 3 };
    ACCENT_POLICY accentPolicy{
        .nAccentState{ ACCENT_ENABLE_BLURBEHIND },
        .nFlags{ 0 },
        .nColor{ 0 },
        .nAnimationId{ 0 },
    };

    const WINDOWCOMPOSITIONATTRIBDATA compositionData{
        .Attrib{ WINDOWCOMPOSITIONATTRIB::WCA_ACCENT_POLICY },
        .pvData{ &accentPolicy },
        .cbData{ sizeof(accentPolicy) },
    };

    const ::BOOL result{ SetWindowCompositionAttribute(window, &compositionData) };
    ::FreeLibrary(user32Dll);
    return static_cast<bool>(result);
}

}
}
