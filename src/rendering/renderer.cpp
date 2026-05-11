#include "rendering/renderer.hpp"
#include <cmath>

/**
 * OverlayRenderer implementation
 */

OverlayRenderer::OverlayRenderer() 
    : screen_width(0), screen_height(0), initialized(false) {
}

OverlayRenderer::~OverlayRenderer() {
    // Platform-specific cleanup would happen here
}

bool OverlayRenderer::initialize(uint32_t width, uint32_t height) {
    screen_width = width;
    screen_height = height;
    initialized = true;
    
    // In real implementation, would initialize rendering backend
    // (DirectX, OpenGL, Vulkan, etc.)
    
    return true;
}

void OverlayRenderer::begin_frame() {
    if (!initialized) return;
    
    // Clear screen for next frame
    // In real implementation: clear backbuffer, set render target, etc.
}

void OverlayRenderer::end_frame() {
    if (!initialized) return;
    
    // Present frame to screen
    // In real implementation: submit command buffers, swap buffers, etc.
}

bool OverlayRenderer::is_ready() const {
    return initialized;
}

uint32_t OverlayRenderer::get_width() const {
    return screen_width;
}

uint32_t OverlayRenderer::get_height() const {
    return screen_height;
}

void OverlayRenderer::draw_aabb(const AABB& box, const Color& color, float thickness) {
    // Project AABB corners to 2D screen space
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
            return; // Not all corners visible
        }
    }
    
    // Draw edges
    // Bottom quad
    draw_line(screen_corners[0], screen_corners[1], color, thickness);
    draw_line(screen_corners[1], screen_corners[2], color, thickness);
    draw_line(screen_corners[2], screen_corners[3], color, thickness);
    draw_line(screen_corners[3], screen_corners[0], color, thickness);
    
    // Top quad
    draw_line(screen_corners[4], screen_corners[5], color, thickness);
    draw_line(screen_corners[5], screen_corners[6], color, thickness);
    draw_line(screen_corners[6], screen_corners[7], color, thickness);
    draw_line(screen_corners[7], screen_corners[4], color, thickness);
    
    // Vertical edges
    draw_line(screen_corners[0], screen_corners[4], color, thickness);
    draw_line(screen_corners[1], screen_corners[5], color, thickness);
    draw_line(screen_corners[2], screen_corners[6], color, thickness);
    draw_line(screen_corners[3], screen_corners[7], color, thickness);
}

void OverlayRenderer::draw_box_2d(const Rect2D& box, const Color& color, float thickness) {
    // Draw 2D screen-space rectangle
    Vector2 top_left(box.x, box.y);
    Vector2 top_right(box.x + box.width, box.y);
    Vector2 bottom_right(box.x + box.width, box.y + box.height);
    Vector2 bottom_left(box.x, box.y + box.height);
    
    draw_line(top_left, top_right, color, thickness);
    draw_line(top_right, bottom_right, color, thickness);
    draw_line(bottom_right, bottom_left, color, thickness);
    draw_line(bottom_left, top_left, color, thickness);
}

void OverlayRenderer::draw_line(const Vector2& from, const Vector2& to, 
                                 const Color& color, float thickness) {
    if (!initialized) return;
    
    // In real implementation: use graphics API to draw line
    // This is placeholder for actual rendering code
}

void OverlayRenderer::draw_filled_rect(const Rect2D& rect, const Color& color) {
    if (!initialized) return;
    
    // In real implementation: use graphics API to draw filled rectangle
    // This is placeholder for actual rendering code
}

void OverlayRenderer::draw_circle(const Vector2& center, float radius, const Color& color) {
    if (!initialized) return;
    
    // Approximate circle with lines
    const int segments = 32;
    const float angle_step = 6.28318f / segments; // 2*pi
    
    Vector2 prev_point;
    for (int i = 0; i <= segments; ++i) {
        float angle = i * angle_step;
        Vector2 point(
            center.x + radius * std::cos(angle),
            center.y + radius * std::sin(angle)
        );
        
        if (i > 0) {
            draw_line(prev_point, point, color, 1.0f);
        }
        prev_point = point;
    }
}

void OverlayRenderer::draw_text(const Vector2& pos, const char* text, const Color& color) {
    if (!initialized) return;
    
    // In real implementation: use font rendering system
    // This is placeholder for actual rendering code
}

bool OverlayRenderer::world_to_screen(const Vector3& world_pos, Vector2& screen_pos) const {
    // Standard 4x4 Matrix projection
    // view_matrix here is assumed to be the View-Projection matrix from the game
    
    float w = view_matrix.data[3] * world_pos.x + 
              view_matrix.data[7] * world_pos.y + 
              view_matrix.data[11] * world_pos.z + 
              view_matrix.data[15];

    if (w < 0.01f) return false;

    float x = view_matrix.data[0] * world_pos.x + 
              view_matrix.data[4] * world_pos.y + 
              view_matrix.data[8] * world_pos.z + 
              view_matrix.data[12];
              
    float y = view_matrix.data[1] * world_pos.x + 
              view_matrix.data[5] * world_pos.y + 
              view_matrix.data[9] * world_pos.z + 
              view_matrix.data[13];

    float inv_w = 1.0f / w;
    x *= inv_w;
    y *= inv_w;

    screen_pos.x = (screen_width / 2.0f) + (x * screen_width / 2.0f);
    screen_pos.y = (screen_height / 2.0f) - (y * screen_height / 2.0f);

    return true;
}

void OverlayRenderer::set_view_matrix(const Matrix4x4& matrix) {
    view_matrix = matrix;
}

void OverlayRenderer::set_projection_matrix(const Matrix4x4& matrix) {
    projection_matrix = matrix;
}
