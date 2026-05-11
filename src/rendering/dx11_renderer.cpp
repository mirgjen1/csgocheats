#include "rendering/dx11_renderer.hpp"

#ifdef _WIN32

#include <cmath>
#include <vector>

// Shader source code (HLSL)
const char* VERTEX_SHADER_SRC = R"(
cbuffer ConstantBuffer : register(b0) {
    float screen_width;
    float screen_height;
    float padding[2];
};

struct VS_INPUT {
    float2 pos : POSITION;
    uint color : COLOR;
};

struct VS_OUTPUT {
    float4 pos : SV_POSITION;
    uint color : COLOR;
};

VS_OUTPUT main(VS_INPUT input) {
    VS_OUTPUT output;
    
    // Convert screen coordinates to clip space
    float clip_x = (input.pos.x / (screen_width / 2.0f)) - 1.0f;
    float clip_y = 1.0f - (input.pos.y / (screen_height / 2.0f));
    
    output.pos = float4(clip_x, clip_y, 0.0f, 1.0f);
    output.color = input.color;
    
    return output;
}
)";

const char* PIXEL_SHADER_SRC = R"(
struct PS_INPUT {
    float4 pos : SV_POSITION;
    uint color : COLOR;
};

float4 main(PS_INPUT input) : SV_TARGET {
    // Extract ARGB color
    uint a = (input.color >> 24) & 0xFF;
    uint r = (input.color >> 16) & 0xFF;
    uint g = (input.color >> 8) & 0xFF;
    uint b = input.color & 0xFF;
    
    return float4(
        r / 255.0f,
        g / 255.0f,
        b / 255.0f,
        a / 255.0f
    );
}
)";

DirectX11Renderer::DirectX11Renderer() {
}

DirectX11Renderer::~DirectX11Renderer() {
    if (context) {
        context->ClearState();
    }
}

bool DirectX11Renderer::initialize(uint32_t width, uint32_t height) {
    screen_width = width;
    screen_height = height;
    
    if (!initialize_device()) {
        return false;
    }
    
    if (!create_shaders()) {
        return false;
    }
    
    if (!create_states()) {
        return false;
    }
    
    if (!create_buffers()) {
        return false;
    }
    
    initialized = true;
    return true;
}

bool DirectX11Renderer::initialize_device() {
    if (window_handle == nullptr) {
        // For overlay, would need actual window handle
        return false;
    }
    
    DXGI_SWAP_CHAIN_DESC swap_chain_desc = {};
    swap_chain_desc.BufferCount = 1;
    swap_chain_desc.BufferDesc.Width = screen_width;
    swap_chain_desc.BufferDesc.Height = screen_height;
    swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.OutputWindow = window_handle;
    swap_chain_desc.SampleDesc.Count = 1;
    swap_chain_desc.SampleDesc.Quality = 0;
    swap_chain_desc.Windowed = TRUE;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    
    D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;
    
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        &feature_level,
        1,
        D3D11_SDK_VERSION,
        &swap_chain_desc,
        swap_chain.GetAddressOf(),
        device.GetAddressOf(),
        nullptr,
        context.GetAddressOf()
    );
    
    if (FAILED(hr)) {
        return false;
    }
    
    // Get backbuffer and create RTV
    hr = swap_chain->GetBuffer(0, IID_PPV_ARGS(backbuffer.GetAddressOf()));
    if (FAILED(hr)) {
        return false;
    }
    
    hr = device->CreateRenderTargetView(backbuffer.Get(), nullptr, rtv.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }
    
    return true;
}

bool DirectX11Renderer::create_shaders() {
    // Compile vertex shader
    ComPtr<ID3DBlob> vs_blob;
    HRESULT hr = D3DCompile(
        VERTEX_SHADER_SRC,
        strlen(VERTEX_SHADER_SRC),
        nullptr,
        nullptr,
        nullptr,
        "main",
        "vs_4_0",
        0,
        0,
        vs_blob.GetAddressOf(),
        nullptr
    );
    
    if (FAILED(hr)) {
        return false;
    }
    
    hr = device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(),
                                    nullptr, vertex_shader.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }
    
    // Compile pixel shader
    ComPtr<ID3DBlob> ps_blob;
    hr = D3DCompile(
        PIXEL_SHADER_SRC,
        strlen(PIXEL_SHADER_SRC),
        nullptr,
        nullptr,
        nullptr,
        "main",
        "ps_4_0",
        0,
        0,
        ps_blob.GetAddressOf(),
        nullptr
    );
    
    if (FAILED(hr)) {
        return false;
    }
    
    hr = device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(),
                                   nullptr, pixel_shader.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }
    
    // Create input layout
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    
    hr = device->CreateInputLayout(layout, 2, vs_blob->GetBufferPointer(),
                                   vs_blob->GetBufferSize(), input_layout.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }
    
    return true;
}

bool DirectX11Renderer::create_states() {
    // Create rasterizer state
    D3D11_RASTERIZER_DESC raster_desc = {};
    raster_desc.FillMode = D3D11_FILL_SOLID;
    raster_desc.CullMode = D3D11_CULL_NONE;
    raster_desc.DepthBias = 0;
    raster_desc.DepthBiasClamp = 0.0f;
    raster_desc.SlopeScaledDepthBias = 0.0f;
    raster_desc.DepthClipEnable = TRUE;
    raster_desc.ScissorEnable = FALSE;
    raster_desc.MultisampleEnable = FALSE;
    raster_desc.AntialiasedLineEnable = TRUE;
    
    HRESULT hr = device->CreateRasterizerState(&raster_desc, raster_state.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }
    
    // Create blend state (for transparency)
    D3D11_BLEND_DESC blend_desc = {};
    blend_desc.AlphaToCoverageEnable = FALSE;
    blend_desc.IndependentBlendEnable = FALSE;
    blend_desc.RenderTarget[0].BlendEnable = TRUE;
    blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    
    hr = device->CreateBlendState(&blend_desc, blend_state.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }
    
    return true;
}

bool DirectX11Renderer::create_buffers() {
    // Create constant buffer
    D3D11_BUFFER_DESC cb_desc = {};
    cb_desc.Usage = D3D11_USAGE_DYNAMIC;
    cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cb_desc.ByteWidth = sizeof(ConstantBuffer);
    
    HRESULT hr = device->CreateBuffer(&cb_desc, nullptr, constant_buffer.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }
    
    // Update constant buffer
    D3D11_MAPPED_SUBRESOURCE mapped;
    hr = context->Map(constant_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr)) {
        return false;
    }
    
    ConstantBuffer* cb_data = static_cast<ConstantBuffer*>(mapped.pData);
    cb_data->screen_width = static_cast<float>(screen_width);
    cb_data->screen_height = static_cast<float>(screen_height);
    
    context->Unmap(constant_buffer.Get(), 0);
    
    return true;
}

void DirectX11Renderer::begin_frame() {
    if (!initialized) return;
    
    context->OMSetRenderTargets(1, rtv.GetAddressOf(), nullptr);
    
    // Set viewport
    D3D11_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(screen_width);
    viewport.Height = static_cast<float>(screen_height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);
    
    clear_screen();
}

void DirectX11Renderer::clear_screen() {
    float color[4] = {0.0f, 0.0f, 0.0f, 0.0f};  // Transparent black
    context->ClearRenderTargetView(rtv.Get(), color);
}

void DirectX11Renderer::end_frame() {
    if (!swap_chain) return;
    swap_chain->Present(0, 0);
}

bool DirectX11Renderer::is_ready() const {
    return initialized && device && context;
}

uint32_t DirectX11Renderer::get_width() const {
    return screen_width;
}

uint32_t DirectX11Renderer::get_height() const {
    return screen_height;
}

void DirectX11Renderer::draw_aabb(const AABB& box, const Color& color, float thickness) {
    Vector3 corners[8] = {
        Vector3(box.min.x, box.min.y, box.min.z),
        Vector3(box.max.x, box.min.y, box.min.z),
        Vector3(box.max.x, box.max.y, box.min.z),
        Vector3(box.min.x, box.max.y, box.min.z),
        Vector3(box.min.x, box.min.y, box.max.z),
        Vector3(box.max.x, box.min.y, box.max.z),
        Vector3(box.max.x, box.max.y, box.max.z),
        Vector3(box.min.x, box.max.y, box.max.z),
    };
    
    Vector2 screen_corners[8];
    for (int i = 0; i < 8; ++i) {
        if (!world_to_screen(corners[i], screen_corners[i])) {
            return;
        }
    }
    
    // Bottom quad
    draw_line_internal(screen_corners[0], screen_corners[1], color, thickness);
    draw_line_internal(screen_corners[1], screen_corners[2], color, thickness);
    draw_line_internal(screen_corners[2], screen_corners[3], color, thickness);
    draw_line_internal(screen_corners[3], screen_corners[0], color, thickness);
    
    // Top quad
    draw_line_internal(screen_corners[4], screen_corners[5], color, thickness);
    draw_line_internal(screen_corners[5], screen_corners[6], color, thickness);
    draw_line_internal(screen_corners[6], screen_corners[7], color, thickness);
    draw_line_internal(screen_corners[7], screen_corners[4], color, thickness);
    
    // Vertical edges
    draw_line_internal(screen_corners[0], screen_corners[4], color, thickness);
    draw_line_internal(screen_corners[1], screen_corners[5], color, thickness);
    draw_line_internal(screen_corners[2], screen_corners[6], color, thickness);
    draw_line_internal(screen_corners[3], screen_corners[7], color, thickness);
}

void DirectX11Renderer::draw_box_2d(const Rect2D& box, const Color& color, float thickness) {
    Vector2 top_left(box.x, box.y);
    Vector2 top_right(box.x + box.width, box.y);
    Vector2 bottom_right(box.x + box.width, box.y + box.height);
    Vector2 bottom_left(box.x, box.y + box.height);
    
    draw_line_internal(top_left, top_right, color, thickness);
    draw_line_internal(top_right, bottom_right, color, thickness);
    draw_line_internal(bottom_right, bottom_left, color, thickness);
    draw_line_internal(bottom_left, top_left, color, thickness);
}

void DirectX11Renderer::draw_line(const Vector2& from, const Vector2& to, const Color& color, float thickness) {
    draw_line_internal(from, to, color, thickness);
}

void DirectX11Renderer::draw_line_internal(const Vector2& from, const Vector2& to, const Color& color, float thickness) {
    if (!initialized) return;
    
    // Create vertices for line
    uint32_t color_argb = (static_cast<uint32_t>(color.a) << 24) |
                         (static_cast<uint32_t>(color.r) << 16) |
                         (static_cast<uint32_t>(color.g) << 8) |
                         static_cast<uint32_t>(color.b);
    
    VertexData vertices[2] = {
        {from.x, from.y, color_argb},
        {to.x, to.y, color_argb}
    };
    
    // Create temporary vertex buffer
    D3D11_BUFFER_DESC vb_desc = {};
    vb_desc.Usage = D3D11_USAGE_DEFAULT;
    vb_desc.ByteWidth = sizeof(vertices);
    vb_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    
    D3D11_SUBRESOURCE_DATA vb_data = {};
    vb_data.pSysMem = vertices;
    
    ComPtr<ID3D11Buffer> vertex_buffer;
    device->CreateBuffer(&vb_desc, &vb_data, vertex_buffer.GetAddressOf());
    
    // Set pipeline state
    context->IASetInputLayout(input_layout.Get());
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
    
    uint32_t stride = sizeof(VertexData);
    uint32_t offset = 0;
    context->IASetVertexBuffers(0, 1, vertex_buffer.GetAddressOf(), &stride, &offset);
    
    context->VSSetShader(vertex_shader.Get(), nullptr, 0);
    context->PSSetShader(pixel_shader.Get(), nullptr, 0);
    context->VSSetConstantBuffers(0, 1, constant_buffer.GetAddressOf());
    
    context->RSSetState(raster_state.Get());
    float blend_factor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    context->OMSetBlendState(blend_state.Get(), blend_factor, 0xFFFFFFFF);
    
    // Draw
    context->Draw(2, 0);
}

void DirectX11Renderer::draw_filled_rect(const Rect2D& rect, const Color& color) {
    // Create vertices for quad
    uint32_t color_argb = (static_cast<uint32_t>(color.a) << 24) |
                         (static_cast<uint32_t>(color.r) << 16) |
                         (static_cast<uint32_t>(color.g) << 8) |
                         static_cast<uint32_t>(color.b);
    
    VertexData vertices[4] = {
        {rect.x, rect.y, color_argb},
        {rect.x + rect.width, rect.y, color_argb},
        {rect.x + rect.width, rect.y + rect.height, color_argb},
        {rect.x, rect.y + rect.height, color_argb},
    };
    
    D3D11_BUFFER_DESC vb_desc = {};
    vb_desc.Usage = D3D11_USAGE_DEFAULT;
    vb_desc.ByteWidth = sizeof(vertices);
    vb_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    
    D3D11_SUBRESOURCE_DATA vb_data = {};
    vb_data.pSysMem = vertices;
    
    ComPtr<ID3D11Buffer> vertex_buffer;
    device->CreateBuffer(&vb_desc, &vb_data, vertex_buffer.GetAddressOf());
    
    context->IASetInputLayout(input_layout.Get());
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    
    uint32_t stride = sizeof(VertexData);
    uint32_t offset = 0;
    context->IASetVertexBuffers(0, 1, vertex_buffer.GetAddressOf(), &stride, &offset);
    
    context->VSSetShader(vertex_shader.Get(), nullptr, 0);
    context->PSSetShader(pixel_shader.Get(), nullptr, 0);
    context->VSSetConstantBuffers(0, 1, constant_buffer.GetAddressOf());
    
    float blend_factor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    context->OMSetBlendState(blend_state.Get(), blend_factor, 0xFFFFFFFF);
    
    context->Draw(4, 0);
}

void DirectX11Renderer::draw_circle(const Vector2& center, float radius, const Color& color) {
    // Approximate circle with lines
    const int segments = 32;
    const float angle_step = 6.28318f / segments;
    
    Vector2 prev_point;
    for (int i = 0; i <= segments; ++i) {
        float angle = i * angle_step;
        Vector2 point(
            center.x + radius * std::cos(angle),
            center.y + radius * std::sin(angle)
        );
        
        if (i > 0) {
            draw_line_internal(prev_point, point, color, 1.0f);
        }
        prev_point = point;
    }
}

void DirectX11Renderer::draw_text(const Vector2& pos, const char* text, const Color& color) {
    // Placeholder for text rendering
    // In real implementation, would use DirectWrite or GDI
}

ID3D11Device* DirectX11Renderer::get_device() {
    return device.Get();
}

ID3D11DeviceContext* DirectX11Renderer::get_context() {
    return context.Get();
}

void DirectX11Renderer::set_window_handle(HWND hwnd) {
    window_handle = hwnd;
}

bool DirectX11Renderer::world_to_screen(const Vector3& world_pos, Vector2& screen_pos) const {
    // Simplified perspective projection
    // Real implementation would use view-projection matrix from game
    
    const float fov = 90.0f;
    const float aspect = static_cast<float>(screen_width) / screen_height;
    
    // Apply view matrix (simplified - would need actual view matrix)
    Vector3 view_pos = world_pos; // Simplified
    
    // Apply projection matrix
    float fov_rad = fov * 3.14159f / 180.0f;
    float f = 1.0f / std::tan(fov_rad / 2.0f);
    
    glm::mat4 projection = glm::perspective(fov_rad, aspect, 0.1f, 1000.0f);
    glm::vec4 clip_pos = projection * glm::vec4(view_pos.x, view_pos.y, view_pos.z, 1.0f);
    
    if (clip_pos.w < 0.1f) return false; // Behind camera
    
    // Perspective divide
    Vector3 ndc_pos(
        clip_pos.x / clip_pos.w,
        clip_pos.y / clip_pos.w,
        clip_pos.z / clip_pos.w
    );
    
    // Viewport transform
    screen_pos.x = (ndc_pos.x + 1.0f) * 0.5f * screen_width;
    screen_pos.y = (1.0f - ndc_pos.y) * 0.5f * screen_height;
    
    return true;
}

#endif // _WIN32
