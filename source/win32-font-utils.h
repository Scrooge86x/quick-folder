#pragma once

#define NOMINMAX
#include <Windows.h>

namespace win32 {
namespace font {

inline ::HFONT createFontFromMemory(void* const fontData, const ::DWORD dataLength) {
    ::DWORD numFonts{};

    // Shouldn't pNumFonts be marked as _Out_?
    ::HANDLE fontResource{ ::AddFontMemResourceEx(fontData, dataLength, 0, &numFonts) };
    if (!fontResource)
        return 0;

    ::LOGFONT logFont{
        .lfHeight{ 24 },
        .lfFaceName{ TEXT("Roboto") }
    };
    return ::CreateFontIndirect(&logFont);
}

}
}
