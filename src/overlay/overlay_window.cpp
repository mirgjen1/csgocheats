#include "overlay/overlay_window.hpp"

#ifdef _WIN32

#include <unordered_map>

// Global map to store window instances for callback routing
static std::unordered_map<HWND, OverlayWindow*> g_window_instances;

OverlayWindow::OverlayWindow() {
}

OverlayWindow::~OverlayWindow() {
    if (window_handle != nullptr) {
        g_window_instances.erase(window_handle);
        DestroyWindow(window_handle);
    }
}

bool OverlayWindow::create(uint32_t w, uint32_t h, const char* title) {
    width = w;
    height = h;
    
    // Register window class
    WNDCLASSA wnd_class = {};
    wnd_class.lpfnWndProc = window_proc_static;
    wnd_class.hInstance = GetModuleHandleA(nullptr);
    wnd_class.lpszClassName = "OverlayWindowClass";
    wnd_class.hCursor = LoadCursorA(nullptr, IDC_ARROW);
    
    if (!RegisterClassA(&wnd_class)) {
        // Class might already be registered, continue anyway
    }
    
    // Create window with transparency support
    window_handle = CreateWindowExA(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
        "OverlayWindowClass",
        title,
        WS_POPUP,
        0, 0, width, height,
        nullptr, nullptr,
        GetModuleHandleA(nullptr),
        nullptr
    );
    
    if (window_handle == nullptr) {
        return false;
    }
    
    // Store instance pointer for callback routing
    g_window_instances[window_handle] = this;
    
    // Setup overlay mode
    if (!setup_overlay_mode()) {
        DestroyWindow(window_handle);
        g_window_instances.erase(window_handle);
        window_handle = nullptr;
        return false;
    }
    
    active = true;
    show();
    return true;
}

bool OverlayWindow::setup_overlay_mode() {
    if (window_handle == nullptr) return false;
    
    // Set window to be layered and transparent
    COLORREF transparent_color = RGB(0, 0, 0);
    
    // Use color key transparency - make black pixels transparent
    if (!SetLayeredWindowAttributes(window_handle, transparent_color, 255, LWA_COLORKEY)) {
        return false;
    }
    
    // Make window click-through (transparent to mouse)
    SetWindowLongA(window_handle, GWL_EXSTYLE, 
                   GetWindowLongA(window_handle, GWL_EXSTYLE) | WS_EX_TRANSPARENT);
    
    return true;
}

HWND OverlayWindow::get_handle() const {
    return window_handle;
}

void OverlayWindow::process_messages() {
    MSG msg = {};
    while (PeekMessageA(&msg, window_handle, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
        
        if (msg.message == WM_DESTROY) {
            active = false;
        }
    }
}

bool OverlayWindow::is_active() const {
    return active && (window_handle != nullptr) && IsWindow(window_handle);
}

void OverlayWindow::resize(uint32_t w, uint32_t h) {
    if (window_handle == nullptr) return;
    
    width = w;
    height = h;
    SetWindowPos(window_handle, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
}

void OverlayWindow::move(int x, int y) {
    if (window_handle == nullptr) return;
    
    SetWindowPos(window_handle, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void OverlayWindow::show() {
    if (window_handle == nullptr) return;
    ShowWindow(window_handle, SW_SHOW);
}

void OverlayWindow::hide() {
    if (window_handle == nullptr) return;
    ShowWindow(window_handle, SW_HIDE);
}

void OverlayWindow::get_dimensions(uint32_t& w, uint32_t& h) const {
    w = width;
    h = height;
}

LRESULT CALLBACK OverlayWindow::window_proc_static(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    // Route message to instance
    auto it = g_window_instances.find(hwnd);
    if (it != g_window_instances.end()) {
        return it->second->window_proc(msg, wparam, lparam);
    }
    
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

LRESULT OverlayWindow::window_proc(UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_DESTROY:
            active = false;
            PostQuitMessage(0);
            return 0;
            
        case WM_CLOSE:
            active = false;
            return 0;
            
        case WM_PAINT:
            // Let DirectX handle painting
            return 0;
            
        case WM_SIZING:
        case WM_SIZE:
            // Handle resize
            return 0;
            
        default:
            return DefWindowProcA(window_handle, msg, wparam, lparam);
    }
}

#endif // _WIN32
