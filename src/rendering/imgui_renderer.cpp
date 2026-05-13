#include "rendering/imgui_renderer.hpp"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <GL/glew.h>

ImGuiRenderer::ImGuiRenderer() : initialized(false) {}

ImGuiRenderer::~ImGuiRenderer() {
    if (initialized) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }
}

bool ImGuiRenderer::initialize(uint32_t width, uint32_t height) {
    screen_width = width;
    screen_height = height;
    return true;
}

void ImGuiRenderer::setup_imgui(SDL_Window* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    ImGui::StyleColorsDark();
    
    // Initialize backends
    // Note: The context must be current here!
    ImGui_ImplSDL2_InitForOpenGL(window, SDL_GL_GetCurrentContext());
    ImGui_ImplOpenGL3_Init("#version 130");
    
    initialized = true;
}

void ImGuiRenderer::begin_frame() {
    if (!initialized) return;
    
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    
    // Use the background draw list for ESP (so it's behind the menu)
    draw_list = ImGui::GetBackgroundDrawList();
}

void ImGuiRenderer::end_frame() {
    if (!initialized) return;
    
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool ImGuiRenderer::is_ready() const {
    return initialized;
}

uint32_t ImGuiRenderer::get_width() const {
    return screen_width;
}

uint32_t ImGuiRenderer::get_height() const {
    return screen_height;
}

void ImGuiRenderer::draw_aabb(const AABB& box, const Color& color, float thickness) {
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
    
    Vector2 sc[8];
    for (int i = 0; i < 8; ++i) {
        if (!world_to_screen(corners[i], sc[i])) return;
    }
    
    // Draw edges using ImGui draw list
    ImU32 col = IM_COL32(color.r, color.g, color.b, color.a);
    
    // Bottom
    draw_list->AddLine({sc[0].x, sc[0].y}, {sc[1].x, sc[1].y}, col, thickness);
    draw_list->AddLine({sc[1].x, sc[1].y}, {sc[2].x, sc[2].y}, col, thickness);
    draw_list->AddLine({sc[2].x, sc[2].y}, {sc[3].x, sc[3].y}, col, thickness);
    draw_list->AddLine({sc[3].x, sc[3].y}, {sc[0].x, sc[0].y}, col, thickness);
    
    // Top
    draw_list->AddLine({sc[4].x, sc[4].y}, {sc[5].x, sc[5].y}, col, thickness);
    draw_list->AddLine({sc[5].x, sc[5].y}, {sc[6].x, sc[6].y}, col, thickness);
    draw_list->AddLine({sc[6].x, sc[6].y}, {sc[7].x, sc[7].y}, col, thickness);
    draw_list->AddLine({sc[7].x, sc[7].y}, {sc[4].x, sc[4].y}, col, thickness);
    
    // Verticals
    draw_list->AddLine({sc[0].x, sc[0].y}, {sc[4].x, sc[4].y}, col, thickness);
    draw_list->AddLine({sc[1].x, sc[1].y}, {sc[5].x, sc[5].y}, col, thickness);
    draw_list->AddLine({sc[2].x, sc[2].y}, {sc[6].x, sc[6].y}, col, thickness);
    draw_list->AddLine({sc[3].x, sc[3].y}, {sc[7].x, sc[7].y}, col, thickness);
}

void ImGuiRenderer::draw_box_2d(const Rect2D& box, const Color& color, float thickness) {
    ImU32 col = IM_COL32(color.r, color.g, color.b, color.a);
    draw_list->AddRect({box.x, box.y}, {box.x + box.width, box.y + box.height}, col, 0.0f, 0, thickness);
}

void ImGuiRenderer::draw_line(const Vector2& from, const Vector2& to, const Color& color, float thickness) {
    ImU32 col = IM_COL32(color.r, color.g, color.b, color.a);
    draw_list->AddLine({from.x, from.y}, {to.x, to.y}, col, thickness);
}

void ImGuiRenderer::draw_filled_rect(const Rect2D& rect, const Color& color) {
    ImU32 col = IM_COL32(color.r, color.g, color.b, color.a);
    draw_list->AddRectFilled({rect.x, rect.y}, {rect.x + rect.width, rect.y + rect.height}, col);
}

void ImGuiRenderer::draw_circle(const Vector2& center, float radius, const Color& color) {
    ImU32 col = IM_COL32(color.r, color.g, color.b, color.a);
    draw_list->AddCircle({center.x, center.y}, radius, col, 32, 1.0f);
}

void ImGuiRenderer::draw_text(const Vector2& pos, const char* text, const Color& color) {
    ImU32 col = IM_COL32(color.r, color.g, color.b, color.a);
    draw_list->AddText({pos.x, pos.y}, col, text);
}

bool ImGuiRenderer::world_to_screen(const Vector3& world_pos, Vector2& screen_pos) const {
    float w = view_matrix.data[3] * world_pos.x + view_matrix.data[7] * world_pos.y + view_matrix.data[11] * world_pos.z + view_matrix.data[15];
    if (w < 0.01f) return false;
    float x = view_matrix.data[0] * world_pos.x + view_matrix.data[4] * world_pos.y + view_matrix.data[8] * world_pos.z + view_matrix.data[12];
    float y = view_matrix.data[1] * world_pos.x + view_matrix.data[5] * world_pos.y + view_matrix.data[9] * world_pos.z + view_matrix.data[13];
    float inv_w = 1.0f / w;
    x *= inv_w;
    y *= inv_w;
    screen_pos.x = (screen_width / 2.0f) + (x * screen_width / 2.0f);
    screen_pos.y = (screen_height / 2.0f) - (y * screen_height / 2.0f);
    return true;
}

void ImGuiRenderer::set_view_matrix(const Matrix4x4& matrix) {
    view_matrix = matrix;
}

void ImGuiRenderer::handle_event(SDL_Event* event) {
    ImGui_ImplSDL2_ProcessEvent(event);
}
