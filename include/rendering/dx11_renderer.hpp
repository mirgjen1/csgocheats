#pragma once

#ifdef _WIN32

#include <d3d11.h>
#include <wrl/client.h>
#include <dxgi.h>
#include "renderer.hpp"

using Microsoft::WRL::ComPtr;

/**
 * DirectX 11 overlay renderer
 * Renders to overlay window with transparent background
 */
class DirectX11Renderer : public Renderer {
public:
    DirectX11Renderer();
    ~DirectX11Renderer() override;
    
    bool initialize(uint32_t width, uint32_t height) override;
    void begin_frame() override;
    void end_frame() override;
    bool is_ready() const override;
    
    uint32_t get_width() const override;
    uint32_t get_height() const override;
    
    void draw_aabb(const AABB& box, const Color& color, float thickness = 2.0f) override;
    void draw_box_2d(const Rect2D& box, const Color& color, float thickness = 2.0f) override;
    void draw_line(const Vector2& from, const Vector2& to, const Color& color, float thickness = 1.0f) override;
    void draw_filled_rect(const Rect2D& rect, const Color& color) override;
    void draw_circle(const Vector2& center, float radius, const Color& color) override;
    void draw_text(const Vector2& pos, const char* text, const Color& color) override;
    
    bool world_to_screen(const Vector3& world_pos, Vector2& screen_pos) const override;
    
    /**
     * Get DirectX device for advanced rendering
     */
    ID3D11Device* get_device();
    ID3D11DeviceContext* get_context();
    
    /**
     * Set window handle for rendering target
     */
    void set_window_handle(HWND hwnd);
    
private:
    // Window and device
    HWND window_handle = nullptr;
    uint32_t screen_width = 0;
    uint32_t screen_height = 0;
    bool initialized = false;
    
    // DirectX interfaces
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<IDXGISwapChain> swap_chain;
    ComPtr<ID3D11RenderTargetView> rtv;
    ComPtr<ID3D11Texture2D> backbuffer;
    
    // Vertex/Index buffers for line rendering
    ComPtr<ID3D11Buffer> line_vertex_buffer;
    ComPtr<ID3D11Buffer> line_index_buffer;
    ComPtr<ID3D11InputLayout> input_layout;
    
    // Shaders
    ComPtr<ID3D11VertexShader> vertex_shader;
    ComPtr<ID3D11PixelShader> pixel_shader;
    
    // Rasterizer and blend states
    ComPtr<ID3D11RasterizerState> raster_state;
    ComPtr<ID3D11BlendState> blend_state;
    
    // Constant buffers
    ComPtr<ID3D11Buffer> constant_buffer;
    
    struct VertexData {
        float x, y;
        uint32_t color;  // ARGB
    };
    
    struct ConstantBuffer {
        float screen_width;
        float screen_height;
        float padding[2];
    };
    
    /**
     * Initialize DirectX device and swap chain
     */
    bool initialize_device();
    
    /**
     * Create shaders and input layout
     */
    bool create_shaders();
    
    /**
     * Create render states
     */
    bool create_states();
    
    /**
     * Create buffers for line rendering
     */
    bool create_buffers();
    
    /**
     * Clear render target with transparent background
     */
    void clear_screen();
    
    /**
     * Draw line using line primitive
     */
    void draw_line_internal(const Vector2& from, const Vector2& to, const Color& color, float thickness);
};

#endif // _WIN32
