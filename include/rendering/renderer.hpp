#pragma once

#include <vector>
#include <memory>
#include "game/game_structures.hpp"

/**
 * Abstract rendering interface
 */
class Renderer {
public:
    Renderer() = default;
    virtual ~Renderer() = default;
    
    /**
     * Initialize renderer
     */
    virtual bool initialize(uint32_t width, uint32_t height) = 0;
    
    /**
     * Begin rendering frame
     */
    virtual void begin_frame() = 0;
    
    /**
     * End rendering frame and present
     */
    virtual void end_frame() = 0;
    
    /**
     * Check if renderer is ready
     */
    virtual bool is_ready() const = 0;
    
    /**
     * Get screen dimensions
     */
    virtual uint32_t get_width() const = 0;
    virtual uint32_t get_height() const = 0;
    
    /**
     * Rendering primitives - these convert 3D world coordinates to 2D screen coordinates
     * and draw the requested shapes
     */
    virtual void draw_aabb(const AABB& box, const Color& color, float thickness = 2.0f) = 0;
    
    virtual void draw_box_2d(const Rect2D& box, const Color& color, float thickness = 2.0f) = 0;
    
    virtual void draw_line(const Vector2& from, const Vector2& to, const Color& color, float thickness = 1.0f) = 0;
    
    virtual void draw_filled_rect(const Rect2D& rect, const Color& color) = 0;
    
    virtual void draw_circle(const Vector2& center, float radius, const Color& color) = 0;
    
    virtual void draw_text(const Vector2& pos, const char* text, const Color& color) = 0;
};

using RendererPtr = std::shared_ptr<Renderer>;

/**
 * Screen space overlay renderer using platform-specific rendering
 */
class OverlayRenderer : public Renderer {
public:
    OverlayRenderer();
    ~OverlayRenderer() override;
    
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
    
    /**
     * World-to-screen projection (requires camera/view-projection matrix)
     * This is a simplified version; real implementation would use game's camera
     */
    bool world_to_screen(const Vector3& world_pos, Vector2& screen_pos) const;
    
    /**
     * Set view-projection matrix for 3D-to-2D transformations
     */
    void set_view_matrix(const Matrix4x4& matrix);
    void set_projection_matrix(const Matrix4x4& matrix);
    
private:
    uint32_t screen_width = 0;
    uint32_t screen_height = 0;
    bool initialized = false;
    
    Matrix4x4 view_matrix;
    Matrix4x4 projection_matrix;
};
