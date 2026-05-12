#include "rendering/opengl_renderer.hpp"

#ifndef _WIN32

#include <vector>
#include <X11/Xlib.h>
#include <X11/extensions/shape.h>

#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>

// Vertex shader source
const char* VERTEX_SHADER_SRC = R"glsl(
#version 330 core
layout(location = 0) in vec2 position;
layout(location = 1) in vec4 color;

out VS_OUT {
    vec4 color;
} vs_out;

uniform mat4 projection;

void main() {
    gl_Position = projection * vec4(position, 0.0, 1.0);
    vs_out.color = color;
}
)glsl";

// Fragment shader source
const char* FRAGMENT_SHADER_SRC = R"glsl(
#version 330 core
in VS_OUT {
    vec4 color;
} fs_in;

out vec4 FragColor;

void main() {
    FragColor = fs_in.color;
}
)glsl";

OpenGLRenderer::OpenGLRenderer() 
    : window(nullptr), screen_width(0), screen_height(0), initialized(false),
      shader_program(0), vao(0), vbo(0), ebo(0) {
}

OpenGLRenderer::~OpenGLRenderer() {
    if (shader_program) {
        glDeleteProgram(shader_program);
    }
    if (vao) {
        glDeleteVertexArrays(1, &vao);
    }
    if (vbo) {
        glDeleteBuffers(1, &vbo);
    }
    if (ebo) {
        glDeleteBuffers(1, &ebo);
    }
    
    if (window) {
        glfwDestroyWindow(window);
    }
    
    glfwTerminate();
}

bool OpenGLRenderer::initialize(uint32_t width, uint32_t height) {
    screen_width = width;
    screen_height = height;
    
    // Initialize GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return false;
    }
    
    if (!create_window()) {
        glfwTerminate();
        return false;
    }
    
    if (!setup_opengl()) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return false;
    }
    
    if (!compile_shaders()) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return false;
    }
    
    setup_orthographic_projection();
    initialized = true;
    
    return true;
}

bool OpenGLRenderer::create_window() {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    
    window = glfwCreateWindow(screen_width, screen_height, "CS2 Overlay", nullptr, nullptr);
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        return false;
    }
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    
    // Make window click-through using X11 Shape extension
    Display* x11_display = glfwGetX11Display();
    Window x11_window = glfwGetX11Window(window);
    
    if (x11_display && x11_window) {
        XShapeCombineRectangles(x11_display, x11_window, ShapeInput, 0, 0, NULL, 0, ShapeSet, Unsorted);
        XFlush(x11_display);
        fprintf(stdout, "[OpenGLRenderer] Set window to click-through mode\n");
    }
    
    return true;
}

bool OpenGLRenderer::setup_opengl() {
    // Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "GLEW initialization failed: %s\n", glewGetErrorString(err));
        return false;
    }
    
    // Setup viewport
    glViewport(0, 0, screen_width, screen_height);
    
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Disable depth test for 2D overlay
    glDisable(GL_DEPTH_TEST);
    
    // Setup vertex arrays
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 1024 * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Color attribute
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, r));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    return true;
}

bool OpenGLRenderer::compile_shaders() {
    // Compile vertex shader
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &VERTEX_SHADER_SRC, nullptr);
    glCompileShader(vertex_shader);
    
    // Check vertex shader compilation
    int success;
    char info_log[512];
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex_shader, 512, nullptr, info_log);
        fprintf(stderr, "Vertex shader compilation failed:\n%s\n", info_log);
        return false;
    }
    
    // Compile fragment shader
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &FRAGMENT_SHADER_SRC, nullptr);
    glCompileShader(fragment_shader);
    
    // Check fragment shader compilation
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment_shader, 512, nullptr, info_log);
        fprintf(stderr, "Fragment shader compilation failed:\n%s\n", info_log);
        return false;
    }
    
    // Link shaders
    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);
    
    // Check linking
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader_program, 512, nullptr, info_log);
        fprintf(stderr, "Shader program linking failed:\n%s\n", info_log);
        return false;
    }
    
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    return true;
}

void OpenGLRenderer::setup_orthographic_projection() {
    projection = glm::ortho(0.0f, static_cast<float>(screen_width),
                           static_cast<float>(screen_height), 0.0f,
                           -1.0f, 1.0f);
    view = glm::mat4(1.0f);
}

void OpenGLRenderer::begin_frame() {
    if (!initialized || !window) return;
    
    // Clear screen with transparent background
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glUseProgram(shader_program);
    glBindVertexArray(vao);
}

void OpenGLRenderer::end_frame() {
    if (!window) return;
    
    glBindVertexArray(0);
    glUseProgram(0);
    
    glfwSwapBuffers(window);
    glfwPollEvents();
}

bool OpenGLRenderer::is_ready() const {
    return initialized && window != nullptr && !glfwWindowShouldClose(window);
}

uint32_t OpenGLRenderer::get_width() const {
    return screen_width;
}

uint32_t OpenGLRenderer::get_height() const {
    return screen_height;
}

void OpenGLRenderer::draw_aabb(const AABB& box, const Color& color, float thickness) {
    // Get 8 corners of AABB
    std::vector<Vector3> corners = {
        Vector3(box.min.x, box.min.y, box.min.z),
        Vector3(box.max.x, box.min.y, box.min.z),
        Vector3(box.max.x, box.max.y, box.min.z),
        Vector3(box.min.x, box.max.y, box.min.z),
        Vector3(box.min.x, box.min.y, box.max.z),
        Vector3(box.max.x, box.min.y, box.max.z),
        Vector3(box.max.x, box.max.y, box.max.z),
        Vector3(box.min.x, box.max.y, box.max.z),
    };
    
    // Convert to screen space
    std::vector<Vector2> screen_corners(8);
    for (int i = 0; i < 8; ++i) {
        if (!world_to_screen(corners[i], screen_corners[i])) {
            return; // Box not on screen
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

void OpenGLRenderer::draw_box_2d(const Rect2D& box, const Color& color, float thickness) {
    draw_line_internal(Vector2(box.x, box.y), Vector2(box.x + box.width, box.y), color, thickness);
    draw_line_internal(Vector2(box.x + box.width, box.y), Vector2(box.x + box.width, box.y + box.height), color, thickness);
    draw_line_internal(Vector2(box.x + box.width, box.y + box.height), Vector2(box.x, box.y + box.height), color, thickness);
    draw_line_internal(Vector2(box.x, box.y + box.height), Vector2(box.x, box.y), color, thickness);
}

void OpenGLRenderer::draw_line(const Vector2& from, const Vector2& to, const Color& color, float thickness) {
    draw_line_internal(from, to, color, thickness);
}

void OpenGLRenderer::draw_line_internal(const Vector2& from, const Vector2& to, const Color& color, float thickness) {
    if (!initialized) return;
    
    // Create a thick line using a quad
    Vector2 dir;
    dir.x = to.x - from.x;
    dir.y = to.y - from.y;
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len < 0.001f) return;
    
    dir.x /= len;
    dir.y /= len;
    
    // Perpendicular vector for thickness
    Vector2 perp;
    perp.x = -dir.y * (thickness / 2.0f);
    perp.y = dir.x * (thickness / 2.0f);
    
    // Create quad vertices
    Vertex vertices[4] = {
        {from.x - perp.x, from.y - perp.y, color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f},
        {from.x + perp.x, from.y + perp.y, color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f},
        {to.x + perp.x, to.y + perp.y, color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f},
        {to.x - perp.x, to.y - perp.y, color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f},
    };
    
    submit_vertices(vertices, 4);
}

void OpenGLRenderer::draw_filled_rect(const Rect2D& rect, const Color& color) {
    Vertex vertices[4] = {
        {rect.x, rect.y, color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f},
        {rect.x + rect.width, rect.y, color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f},
        {rect.x + rect.width, rect.y + rect.height, color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f},
        {rect.x, rect.y + rect.height, color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f},
    };
    
    submit_vertices(vertices, 4);
}

void OpenGLRenderer::draw_circle(const Vector2& center, float radius, const Color& color) {
    const int segments = 32;
    const float angle_step = 6.28318f / segments;
    
    std::vector<Vertex> vertices;
    for (int i = 0; i <= segments; ++i) {
        float angle = i * angle_step;
        Vector2 point(
            center.x + radius * std::cos(angle),
            center.y + radius * std::sin(angle)
        );
        
        vertices.push_back({
            point.x, point.y,
            color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f
        });
    }
    
    submit_vertices(vertices.data(), vertices.size());
}

// Simple 8x8 bitmap font data (basic ASCII 32-126)
const unsigned char FONT_8X8[95][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // space
    {0x18,0x3c,0x3c,0x18,0x18,0x00,0x18,0x00}, // !
    {0x6c,0x6c,0x6c,0x00,0x00,0x00,0x00,0x00}, // "
    {0x6c,0x6c,0xfe,0x6c,0xfe,0x6c,0x6c,0x00}, // #
    {0x18,0x3e,0x60,0x3c,0x06,0x7c,0x18,0x00}, // $
    {0x00,0xc6,0xcc,0x18,0x30,0x66,0xc6,0x00}, // %
    {0x38,0x6c,0x38,0x76,0xdc,0xcc,0x76,0x00}, // &
    {0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00}, // '
    {0x0c,0x18,0x30,0x30,0x30,0x18,0x0c,0x00}, // (
    {0x30,0x18,0x0c,0x0c,0x0c,0x18,0x30,0x00}, // )
    {0x00,0x66,0x3c,0xff,0x3c,0x66,0x00,0x00}, // *
    {0x00,0x18,0x18,0x7e,0x18,0x18,0x00,0x00}, // +
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30}, // ,
    {0x00,0x00,0x00,0x7e,0x00,0x00,0x00,0x00}, // -
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00}, // .
    {0x02,0x06,0x0c,0x18,0x30,0x60,0x40,0x00}, // /
    {0x3c,0x66,0x6e,0x76,0x66,0x66,0x3c,0x00}, // 0
    {0x18,0x38,0x18,0x18,0x18,0x18,0x7e,0x00}, // 1
    {0x3c,0x66,0x06,0x0c,0x18,0x30,0x7e,0x00}, // 2
    {0x3c,0x66,0x06,0x1c,0x06,0x66,0x3c,0x00}, // 3
    {0x06,0x0e,0x16,0x26,0x7e,0x06,0x06,0x00}, // 4
    {0x7e,0x60,0x7c,0x06,0x06,0x66,0x3c,0x00}, // 5
    {0x3c,0x66,0x60,0x7c,0x66,0x66,0x3c,0x00}, // 6
    {0x7e,0x66,0x06,0x0c,0x18,0x18,0x18,0x00}, // 7
    {0x3c,0x66,0x66,0x3c,0x66,0x66,0x3c,0x00}, // 8
    {0x3c,0x66,0x66,0x3e,0x06,0x66,0x3c,0x00}, // 9
    {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00}, // :
    {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x30}, // ;
    {0x06,0x0c,0x18,0x30,0x18,0x0c,0x06,0x00}, // <
    {0x00,0x00,0x7e,0x00,0x7e,0x00,0x00,0x00}, // =
    {0x60,0x30,0x18,0x0c,0x18,0x30,0x60,0x00}, // >
    {0x3c,0x66,0x06,0x0c,0x18,0x00,0x18,0x00}, // ?
    {0x3c,0x66,0x6e,0x6e,0x60,0x66,0x3c,0x00}, // @
    {0x18,0x3c,0x66,0x66,0x7e,0x66,0x66,0x00}, // A
    {0x7c,0x66,0x66,0x7c,0x66,0x66,0x7c,0x00}, // B
    {0x3c,0x66,0x60,0x60,0x60,0x66,0x3c,0x00}, // C
    {0x78,0x6c,0x66,0x66,0x66,0x6c,0x78,0x00}, // D
    {0x7e,0x60,0x60,0x78,0x60,0x60,0x7e,0x00}, // E
    {0x7e,0x60,0x60,0x78,0x60,0x60,0x60,0x00}, // F
    {0x3c,0x66,0x60,0x6e,0x66,0x66,0x3c,0x00}, // G
    {0x66,0x66,0x66,0x7e,0x66,0x66,0x66,0x00}, // H
    {0x3c,0x18,0x18,0x18,0x18,0x18,0x3c,0x00}, // I
    {0x1e,0x06,0x06,0x06,0x06,0x66,0x3c,0x00}, // J
    {0x66,0x6c,0x78,0x70,0x78,0x6c,0x66,0x00}, // K
    {0x60,0x60,0x60,0x60,0x60,0x60,0x7e,0x00}, // L
    {0x63,0x77,0x7f,0x6b,0x63,0x63,0x63,0x00}, // M
    {0x66,0x66,0x76,0x7e,0x6e,0x66,0x66,0x00}, // N
    {0x3c,0x66,0x66,0x66,0x66,0x66,0x3c,0x00}, // O
    {0x7c,0x66,0x66,0x7c,0x60,0x60,0x60,0x00}, // P
    {0x3c,0x66,0x66,0x66,0x66,0x6c,0x36,0x00}, // Q
    {0x7c,0x66,0x66,0x7c,0x78,0x6c,0x66,0x00}, // R
    {0x3c,0x66,0x30,0x18,0x0c,0x66,0x3c,0x00}, // S
    {0x7e,0x5a,0x18,0x18,0x18,0x18,0x18,0x00}, // T
    {0x66,0x66,0x66,0x66,0x66,0x66,0x3c,0x00}, // U
    {0x66,0x66,0x66,0x66,0x66,0x3c,0x18,0x00}, // V
    {0x63,0x63,0x63,0x6b,0x7f,0x77,0x63,0x00}, // W
    {0x66,0x66,0x3c,0x18,0x3c,0x66,0x66,0x00}, // X
    {0x66,0x66,0x66,0x3c,0x18,0x18,0x18,0x00}, // Y
    {0x7e,0x06,0x0c,0x18,0x30,0x60,0x7e,0x00}, // Z
    {0x3c,0x30,0x30,0x30,0x30,0x30,0x3c,0x00}, // [
    {0x40,0x60,0x30,0x18,0x0c,0x06,0x02,0x00}, // \
    {0x3c,0x0c,0x0c,0x0c,0x0c,0x0c,0x3c,0x00}, // ]
    {0x10,0x38,0x6c,0x00,0x00,0x00,0x00,0x00}, // ^
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff}, // _
    {0x30,0x30,0x18,0x00,0x00,0x00,0x00,0x00}, // `
    {0x00,0x00,0x3c,0x06,0x3e,0x66,0x3e,0x00}, // a
    {0x60,0x60,0x7c,0x66,0x66,0x66,0x7c,0x00}, // b
    {0x00,0x00,0x3c,0x60,0x60,0x66,0x3c,0x00}, // c
    {0x06,0x06,0x3e,0x66,0x66,0x66,0x3e,0x00}, // d
    {0x00,0x00,0x3c,0x66,0x7e,0x60,0x3c,0x00}, // e
    {0x1c,0x30,0x78,0x30,0x30,0x30,0x30,0x00}, // f
    {0x00,0x00,0x3e,0x66,0x66,0x3e,0x06,0x3c}, // g
    {0x60,0x60,0x7c,0x66,0x66,0x66,0x66,0x00}, // h
    {0x18,0x00,0x38,0x18,0x18,0x18,0x3c,0x00}, // i
    {0x0c,0x00,0x1c,0x0c,0x0c,0x0c,0x6c,0x38}, // j
    {0x60,0x60,0x66,0x6c,0x78,0x6c,0x66,0x00}, // k
    {0x38,0x18,0x18,0x18,0x18,0x18,0x3c,0x00}, // l
    {0x00,0x00,0x66,0x7f,0x7f,0x6b,0x63,0x00}, // m
    {0x00,0x00,0x7c,0x66,0x66,0x66,0x66,0x00}, // n
    {0x00,0x00,0x3c,0x66,0x66,0x66,0x3c,0x00}, // o
    {0x00,0x00,0x7c,0x66,0x66,0x7c,0x60,0x60}, // p
    {0x00,0x00,0x3e,0x66,0x66,0x3e,0x06,0x06}, // q
    {0x00,0x00,0x7c,0x66,0x60,0x60,0x60,0x00}, // r
    {0x00,0x00,0x3e,0x60,0x3c,0x06,0x7c,0x00}, // s
    {0x30,0x30,0x7c,0x30,0x30,0x30,0x1c,0x00}, // t
    {0x00,0x00,0x66,0x66,0x66,0x66,0x3e,0x00}, // u
    {0x00,0x00,0x66,0x66,0x66,0x3c,0x18,0x00}, // v
    {0x00,0x00,0x63,0x6b,0x7f,0x7f,0x36,0x00}, // w
    {0x00,0x00,0x66,0x3c,0x18,0x3c,0x66,0x00}, // x
    {0x00,0x00,0x66,0x66,0x66,0x3e,0x06,0x3c}, // y
    {0x00,0x00,0x7e,0x0c,0x18,0x30,0x7e,0x00}, // z
    {0x0c,0x18,0x18,0x70,0x18,0x18,0x0c,0x00}, // {
    {0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00}, // |
    {0x30,0x18,0x18,0x0e,0x18,0x18,0x30,0x00}, // }
    {0x31,0x6b,0x46,0x00,0x00,0x00,0x00,0x00}, // ~
};

void OpenGLRenderer::draw_text(const Vector2& pos, const char* text, const Color& color) {
    if (!initialized || !text) return;
    
    float cursor_x = pos.x;
    float cursor_y = pos.y;
    float scale = 1.0f; // Could be made a parameter
    
    for (const char* p = text; *p; ++p) {
        if (*p == '\n') {
            cursor_x = pos.x;
            cursor_y += 10.0f * scale;
            continue;
        }
        
        if (*p < 32 || *p > 126) continue;
        
        int char_idx = *p - 32;
        for (int row = 0; row < 8; ++row) {
            unsigned char bits = FONT_8X8[char_idx][row];
            for (int col = 0; col < 8; ++col) {
                if (bits & (0x80 >> col)) {
                    Rect2D pixel(
                        cursor_x + col * scale, 
                        cursor_y + row * scale, 
                        scale, scale
                    );
                    draw_filled_rect(pixel, color);
                }
            }
        }
        
        cursor_x += 8.0f * scale;
    }
}

bool OpenGLRenderer::world_to_screen(const Vector3& world_pos, Vector2& screen_pos) const {
    // Standard Source Engine View-Projection Matrix (Row-Major)
    // The 4th row (indices 12, 13, 14, 15) provides the W component
    float w = current_view_matrix.data[12] * world_pos.x + 
              current_view_matrix.data[13] * world_pos.y + 
              current_view_matrix.data[14] * world_pos.z + 
              current_view_matrix.data[15];

    if (w < 0.01f) return false;

    float x = current_view_matrix.data[0] * world_pos.x + 
              current_view_matrix.data[1] * world_pos.y + 
              current_view_matrix.data[2] * world_pos.z + 
              current_view_matrix.data[3];
              
    float y = current_view_matrix.data[4] * world_pos.x + 
              current_view_matrix.data[5] * world_pos.y + 
              current_view_matrix.data[6] * world_pos.z + 
              current_view_matrix.data[7];

    float inv_w = 1.0f / w;
    x *= inv_w;
    y *= inv_w;

    screen_pos.x = (screen_width / 2.0f) + (x * screen_width / 2.0f);
    screen_pos.y = (screen_height / 2.0f) - (y * screen_height / 2.0f);

    return true;
}

void OpenGLRenderer::set_view_matrix(const Matrix4x4& matrix) {
    current_view_matrix = matrix;
}

void OpenGLRenderer::submit_vertices(const Vertex* vertices, size_t count) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(Vertex), vertices);
    
    glDrawArrays(GL_QUADS, 0, static_cast<GLsizei>(count));
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

GLFWwindow* OpenGLRenderer::get_window() const {
    return window;
}

#endif // !_WIN32
