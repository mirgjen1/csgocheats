#pragma once

#ifndef _WIN32

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "renderer.hpp"

/**
 * OpenGL overlay renderer for Linux/Unix systems
 * Uses GLFW for window management and OpenGL 3.3+ for rendering
 */
class OpenGLRenderer : public Renderer {
public:
    OpenGLRenderer();
    ~OpenGLRenderer() override;
    
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
     * Get GLFW window handle
     */
    GLFWwindow* get_window() const;
    
private:
    // Window and context
    GLFWwindow* window = nullptr;
    uint32_t screen_width = 0;
    uint32_t screen_height = 0;
    bool initialized = false;
    
    // OpenGL state
    GLuint shader_program = 0;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    
    // View/projection matrices
    glm::mat4 projection;
    glm::mat4 view;
    
    struct Vertex {
        float x, y;
        float r, g, b, a;
    };
    
    // Helper methods
    bool create_window();
    bool setup_opengl();
    bool compile_shaders();
    void draw_line_internal(const Vector2& from, const Vector2& to, const Color& color, float thickness);
    void submit_vertices(const Vertex* vertices, size_t count);
    void setup_orthographic_projection();
};

#endif // !_WIN32
