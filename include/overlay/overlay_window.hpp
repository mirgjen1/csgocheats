#pragma once

#ifdef _WIN32

#include <windows.h>
#include <memory>

/**
 * Overlay window manager for creating and maintaining transparent overlay window
 */
class OverlayWindow {
public:
    OverlayWindow();
    ~OverlayWindow();
    
    /**
     * Create overlay window
     */
    bool create(uint32_t width, uint32_t height, const char* title = "Overlay");
    
    /**
     * Get window handle
     */
    HWND get_handle() const;
    
    /**
     * Process window messages (call regularly)
     */
    void process_messages();
    
    /**
     * Check if window is still active
     */
    bool is_active() const;
    
    /**
     * Set window size
     */
    void resize(uint32_t width, uint32_t height);
    
    /**
     * Move window
     */
    void move(int x, int y);
    
    /**
     * Show/hide window
     */
    void show();
    void hide();
    
    /**
     * Get window client area dimensions
     */
    void get_dimensions(uint32_t& width, uint32_t& height) const;
    
private:
    HWND window_handle = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
    bool active = false;
    
    /**
     * Static window procedure (callback)
     */
    static LRESULT CALLBACK window_proc_static(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    
    /**
     * Instance window procedure
     */
    LRESULT window_proc(UINT msg, WPARAM wparam, LPARAM lparam);
    
    /**
     * Set window to transparent overlay mode
     */
    bool setup_overlay_mode();
};

using OverlayWindowPtr = std::shared_ptr<OverlayWindow>;

#endif // _WIN32
