#include "win32-font-utils.h"
#include "directory-select-window.h"
#include "win32-resource-utils.h"
#include "win32-window-utils.h"
#include "directory-utils.h"
#include "resources.h"

#include <fstream>
#include <locale>
#include <string_view>
#include <winnt.h>

static inline int exitMessage(const std::wstring_view message) {
    ::MessageBox(NULL, message.data(), TEXT("Error."), MB_OK | MB_ICONERROR);
    return -1;
}

int main() {
    std::wifstream file{ "config.txt" };
    file.imbue(std::locale{ "en_US.UTF-8" });
    const std::wstring fileContents{ std::istreambuf_iterator{ file }, {} };
    file.close();

    DirectoryNode root{};
    if (!root.readFromString(fileContents))
        return exitMessage(TEXT("Loading config failed."));

    DirectoryNavigator navigator{ &root };

    DirectorySelectWindow window{ TEXT("Quick Folder"), &navigator };

    win32::window::enableBackdropBlur(window.getSystemHandle());
    
    return window.runMessageLoop();
}

int WINAPI WinMain(
    [[maybe_unused]] _In_ ::HINSTANCE hInstance,
    [[maybe_unused]] _In_opt_ ::HINSTANCE hPrevInstance,
    [[maybe_unused]] _In_ ::LPSTR lpCmdLine,
    [[maybe_unused]] _In_ int nCmdShow
) {
    return main();
}
