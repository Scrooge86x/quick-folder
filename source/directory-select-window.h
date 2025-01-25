#pragma once

#include "directory-utils.h"
#include "resources.h"

#include <d2d1.h>
#include <dwrite.h>
#include <wrl.h>

namespace wrl = Microsoft::WRL;

class DirectorySelectWindow {
public:
    struct StyleConfig {
        struct {
            float horizontal{ 25 };
            float vertical{ 15 };
        } padding{};
        float gaps{ 5 };
        float fontSize{ 25 };
        float backgroundTint{ .4f };
    };

    DirectorySelectWindow(
        const std::wstring& title,
        DirectoryNavigator* const navigator,
        const StyleConfig styleConfig = {}
    )
        : m_backgroundTintColor{ D2D1::ColorF(0.f, 0.f, 0.f, styleConfig.backgroundTint) }
        , m_styleConfig{ styleConfig }
        , m_title{ title }
        , m_navigator{ navigator }
    {
        setupWindow();
        setupDirect2D();
        
        fitToContent();
        drawDirectories();
    }

    DirectorySelectWindow(DirectorySelectWindow&) = delete;
    DirectorySelectWindow(DirectorySelectWindow&&) = delete;
    DirectorySelectWindow& operator=(DirectorySelectWindow&) = delete;

    inline ::HWND getSystemHandle() const {
        return m_window.handle;
    }

    int runMessageLoop() const;

private:

    // TODO: could redraw only the current child and the previous one
    // or could even generate one bitmap that is stored in the wrapper
    // then also add DirectoryNode::m_longestChildWidth to the wrapper
    void drawDirectories() const;

    void cacheNodeSize(DirectoryNode* const node) const;

    void fitToContent();

    void setupDirect2D();

    void setupWindow();

    void handleKeyPress(
        const ::WPARAM keyCode,
        const ::LPARAM lParam,
        const bool isLeftShiftDown
    );

    static ::LRESULT CALLBACK WindowProc(
        ::HWND hwnd,
        ::UINT msg,
        ::WPARAM wParam,
        ::LPARAM lParam
    );

    const ::D2D1_COLOR_F m_backgroundTintColor{};

    struct {
        int width{};
        int height{};
        bool hasFocus{};
        ::HWND handle{};
        ::D2D1_RECT_F drawableArea{};
    } m_window{};

    const StyleConfig m_styleConfig{};
    const std::wstring m_title{};

    DirectoryNavigator* const m_navigator;

    struct {
        wrl::ComPtr<::ID2D1Factory> factory{};
        wrl::ComPtr<::ID2D1HwndRenderTarget> renderTarget{};
        wrl::ComPtr<::ID2D1SolidColorBrush> whiteBrush{};
        wrl::ComPtr<::ID2D1SolidColorBrush> yellowBrush{};
        wrl::ComPtr<::ID2D1SolidColorBrush> orangeBrush{};
        wrl::ComPtr<::IDWriteFactory> writeFactory{};
        wrl::ComPtr<::IDWriteTextFormat> textFormat{};
    } d2d;
};
