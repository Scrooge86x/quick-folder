#pragma once

#define NOMINMAX
#include <Windows.h>
#include <utility>

namespace win32 {
namespace resource {

inline std::pair<void* const, const ::DWORD> loadResourceToMemory(int resourceId, ::TCHAR* resourceType) {
    const ::HMODULE module{ ::GetModuleHandle(NULL) };
    const ::HRSRC resource{ ::FindResource(module, MAKEINTRESOURCE(resourceId), resourceType) };
    if (!resource)
        return { nullptr, 0 };

    const ::HGLOBAL memory{ ::LoadResource(module, resource) };
    if (!memory)
        return { nullptr, 0 };

    void* const data{ ::LockResource(memory) };
    if (!data)
        return { nullptr, 0 };

    const ::DWORD size{ ::SizeofResource(module, resource) };
    if (!size)
        return { nullptr, 0 };

    return { data, size };
}

}
}
