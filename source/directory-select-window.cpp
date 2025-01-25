#include "directory-select-window.h"

#include <chrono>

#ifdef _DEBUG
#include <print>
#endif

int DirectorySelectWindow::runMessageLoop() const {
    ::MSG message{};
    while (::GetMessage(&message, m_window.handle, 0, 0)) {
        ::TranslateMessage(&message);
        ::DispatchMessage(&message);
    }
    return static_cast<int>(message.wParam);
}

void DirectorySelectWindow::drawDirectories() const {
    d2d.renderTarget->BeginDraw();
    d2d.renderTarget->Clear(m_backgroundTintColor);

    ::D2D1_RECT_F drawableArea{ m_window.drawableArea };
    const auto textHeight{ m_navigator->getCurrentNode()->getLongestChildSize().height };
    for (const auto& [name, node] : m_navigator->getCurrentNode()->getChildren()) {
        ::ID2D1SolidColorBrush* brush{};
        if (m_window.hasFocus && m_navigator->getSelectedChild()->getName() == name) {
            brush = node.getChildCount() ? d2d.yellowBrush.Get() : d2d.orangeBrush.Get();
        } else {
            brush = d2d.whiteBrush.Get();
        }

        // TODO: Might want to switch to DrawTextLayout,
        // because this creates a text layout every time it's called.
        // We could reuse them if the user presses arrow down or up
        // Also consider just drawing the whole text with one draw call
        // by caching it in the parent node.
        d2d.renderTarget->DrawText(
            name.c_str(),
            static_cast<::UINT32>(name.size()),
            d2d.textFormat.Get(),
            drawableArea,
            brush
        );
        drawableArea.top += textHeight;
    }

    d2d.renderTarget->EndDraw();
}

void DirectorySelectWindow::cacheNodeSize(DirectoryNode* const node) const {
    wrl::ComPtr<::IDWriteTextLayout> textLayout{};
    d2d.writeFactory->CreateTextLayout(
        node->getLongestChildName().data(),
        static_cast<::UINT32>(node->getLongestChildName().size()),
        d2d.textFormat.Get(),
        0.f,
        0.f,
        &textLayout
    );

    ::DWRITE_TEXT_METRICS textMetrics{};
    textLayout->GetMetrics(&textMetrics);
    node->setLongestChildSize(textMetrics.width, std::ceil(textMetrics.height + m_styleConfig.gaps));
}

void DirectorySelectWindow::fitToContent() {
    const auto& currentNode{ m_navigator->getCurrentNode() };

    if (!currentNode->getLongestChildSize().width) {
        cacheNodeSize(currentNode);
    }

    const auto longestChildSize{ currentNode->getLongestChildSize() };

    m_window.width = static_cast<int>(
        longestChildSize.width + m_styleConfig.padding.horizontal * 2
    );

    m_window.height = static_cast<int>(
        longestChildSize.height * static_cast<float>(currentNode->getChildCount())
        + m_styleConfig.padding.vertical * 2
        - m_styleConfig.gaps
    );

    m_window.drawableArea = D2D1::RectF(
        m_styleConfig.padding.horizontal,
        m_styleConfig.padding.vertical,
        static_cast<float>(m_window.width) - m_styleConfig.padding.horizontal,
        static_cast<float>(m_window.height) - m_styleConfig.padding.vertical
    );

    // Prevents 1 frame flicking of text when the window is resized
    d2d.renderTarget->BeginDraw();
    d2d.renderTarget->Clear(m_backgroundTintColor);
    d2d.renderTarget->EndDraw();

    ::SetWindowPos(
        m_window.handle,
        HWND_TOPMOST,
        (::GetSystemMetrics(SM_CXSCREEN) - m_window.width) / 2,
        (::GetSystemMetrics(SM_CYSCREEN) - m_window.height) / 2,
        m_window.width,
        m_window.height,
        SWP_SHOWWINDOW | SWP_NOREDRAW
    );

    d2d.renderTarget->Resize(D2D1::SizeU(
        static_cast<unsigned int>(m_window.width),
        static_cast<unsigned int>(m_window.height)
    ));
}

void DirectorySelectWindow::setupDirect2D() {
    ::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2d.factory.GetAddressOf());

    d2d.factory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED)
        ),
        D2D1::HwndRenderTargetProperties(m_window.handle),
        &d2d.renderTarget
    );
    d2d.renderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

    d2d.renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::White), &d2d.whiteBrush
    );
    d2d.renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::Yellow), &d2d.yellowBrush
    );
    d2d.renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(1.f, .5f, 0.f, 1.f), &d2d.orangeBrush
    );

    ::DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(::IDWriteFactory),
        reinterpret_cast<::IUnknown**>(d2d.writeFactory.GetAddressOf())
    );

    d2d.writeFactory->CreateTextFormat(
        L"Arial",
        NULL,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        m_styleConfig.fontSize,
        L"pl-PL",
        &d2d.textFormat
    );
    d2d.textFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
}

void DirectorySelectWindow::setupWindow() {
    const auto windowClassName{ TEXT("quick-folder-window") };

    ::HMODULE thisModule{ ::GetModuleHandle(NULL) };
    ::WNDCLASS windowClass{
        .style{ CS_CLASSDC | CS_DROPSHADOW },
        .lpfnWndProc{ &DirectorySelectWindow::WindowProc },
        .cbClsExtra{},
        .cbWndExtra{},
        .hInstance{ thisModule },
        .hIcon{},
        .hCursor{ ::LoadCursor(NULL, IDC_ARROW) },
        .hbrBackground{ reinterpret_cast<::HBRUSH>(COLOR_WINDOW + 1) },
        .lpszMenuName{},
        .lpszClassName{ windowClassName },
    };
    if (!::RegisterClass(&windowClass)) {
        exit(1);
    }

    m_window.handle = ::CreateWindow(
        windowClassName,
        m_title.c_str(),
        WS_VISIBLE | WS_POPUP,
        0, 0, 0, 0,
        NULL,
        NULL,
        thisModule,
        this
    );
    if (!m_window.handle) {
        exit(2);
    }
}

static void openExplorerWindow(
    const std::wstring_view path,
    const ::HWND windowToClose
) {
    if (!windowToClose) {
        ::ShellExecute(
            NULL, TEXT("explore"), path.data(),
            NULL, NULL, SW_SHOWNOACTIVATE
        );
        return;
    }
    
    ::ShellExecute(
        NULL, TEXT("explore"), path.data(),
        NULL, NULL, SW_SHOWDEFAULT
    );
    ::PostMessage(windowToClose, WM_QUIT, 0, 0);
}

void DirectorySelectWindow::handleKeyPress(
    const ::WPARAM keyCode,
    const ::LPARAM lParam,
    const bool isLeftShiftDown
) {
    static std::chrono::high_resolution_clock::time_point lastRepeatedChar{};
    if (HIWORD(lParam) & KF_REPEAT) {
        using namespace std::chrono_literals;
        if (std::chrono::high_resolution_clock::now() - lastRepeatedChar < 100ms)
            return;

        lastRepeatedChar = std::chrono::high_resolution_clock::now();
    }

    switch (keyCode) {
    case VK_LSHIFT:
    case VK_LEFT:
    case 'A':
    case 'a':
        if (m_navigator->enterParent()) {
            fitToContent();
        }
        drawDirectories();
        break;
    case VK_RIGHT:
    case 'D':
    case 'd':
        if (m_navigator->enterSelected()) {
            fitToContent();
        }
        drawDirectories();
        break;
    case VK_UP:
    case 'W':
    case 'w':
        m_navigator->selectionUp();
        drawDirectories();
        break;
    case VK_DOWN:
    case 'S':
    case 's':
        m_navigator->selectionDown();
        drawDirectories();
        break;
    case 'E':
    case 'e':
        m_window.hasFocus = false;
        openExplorerWindow(
            m_navigator->getCurrentNode()->getFullPath(),
            isLeftShiftDown ? 0 : m_window.handle
        );
        break;
    case VK_RETURN:
    case VK_SPACE:
        openExplorerWindow(
            m_navigator->getCurrentNode()->getFullPath() + m_navigator->getSelectedChild()->getName(),
            isLeftShiftDown ? 0 : m_window.handle
        );
        break;
    case VK_ESCAPE:
    case 'Q':
    case 'q':
        ::PostMessage(m_window.handle, WM_QUIT, 0, 0);
        break;
    default:
        break;
    }
}

::LRESULT CALLBACK DirectorySelectWindow::WindowProc(
    ::HWND hwnd,
    ::UINT msg,
    ::WPARAM wParam,
    ::LPARAM lParam
) {
    if (msg == WM_CREATE) {
        const auto createParams{ reinterpret_cast<::CREATESTRUCT*>(lParam)->lpCreateParams };
        if (!lParam || !createParams)
            return -1;

        ::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<::LONG_PTR>(createParams));
    }

    static bool isLeftShiftDown{};

    // Only retrieve thisptr for specific messages
    switch (msg) {
    case WM_KEYDOWN: {
        if (wParam == VK_SHIFT) {
            isLeftShiftDown = true;
        }

        const auto thisptr{ reinterpret_cast<DirectorySelectWindow*>(
            ::GetWindowLongPtr(hwnd, GWLP_USERDATA)
        ) };
        thisptr->handleKeyPress(wParam, lParam, isLeftShiftDown);
    } return 0;
    case WM_KEYUP:
        if (wParam == VK_SHIFT) {
            isLeftShiftDown = false;
        }
        return 0;
    case WM_SETFOCUS: {
        const auto thisptr{ reinterpret_cast<DirectorySelectWindow*>(
            ::GetWindowLongPtr(hwnd, GWLP_USERDATA)
        ) };
        thisptr->m_window.hasFocus = true;
        if (thisptr->d2d.renderTarget) {
            thisptr->drawDirectories();
        }
    } return 0;
    case WM_KILLFOCUS: {
        const auto thisptr{ reinterpret_cast<DirectorySelectWindow*>(
            ::GetWindowLongPtr(hwnd, GWLP_USERDATA)
        ) };

        static bool justChanged{};
        if (justChanged) {
            ::SetForegroundWindow(thisptr->m_window.handle);
            justChanged = false;
            return 0;
        }

        if (!thisptr->m_window.hasFocus) {
            ::SetForegroundWindow(thisptr->m_window.handle);
            justChanged = true;
        } else {
            thisptr->m_window.hasFocus = false;
            thisptr->drawDirectories();
        }
    } return 0;
    case WM_CLOSE:
        ::PostQuitMessage(0);
        return 0;
    default:
        return ::DefWindowProc(hwnd, msg, wParam, lParam);
    }
}
