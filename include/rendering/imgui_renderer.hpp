#pragma once

#include "rendering/renderer.hpp"
#include "imgui.h"
#include <SDL2/SDL.h>
#include <memory>

/**
 * Renderer implementation using Dear ImGui
 */
class ImGuiRenderer : public Renderer {
public:
    ImGuiRenderer();
    ~ImGuiRenderer() override;
    
    bool initialize(uint32_t width, uint32_t height) override;
    void setup_imgui(SDL_Window* window);
    
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
    void set_view_matrix(const Matrix4x4& matrix) override;
    
    void handle_event(SDL_Event* event);

private:
    uint32_t screen_width = 0;
    uint32_t screen_height = 0;
    bool initialized = false;
    Matrix4x4 view_matrix;
    
    ImDrawList* draw_list = nullptr;
};
