#include "rendering/ui_backend.hpp"
#include <cstdio>

UIBackend::UIBackend(RendererPtr renderer) : renderer(renderer) {
}

void UIBackend::draw_watermark() {
    if (!renderer) return;
    
    const char* title = "ANTIGRAVITY CS:GO OVERLAY v1.0";
    Vector2 pos(20, 20);
    
    // Draw shadow
    renderer->draw_text(Vector2(pos.x + 1, pos.y + 1), title, Color(0, 0, 0, 200));
    // Draw main text
    renderer->draw_text(pos, title, Color(0, 255, 128, 255));
    
    // Underline
    renderer->draw_line(
        Vector2(pos.x, pos.y + 10), 
        Vector2(pos.x + 220, pos.y + 10), 
        Color(0, 255, 128, 255), 
        1.0f
    );
}

void UIBackend::draw_status_panel(int fps, size_t entity_count) {
    if (!renderer) return;
    
    char status_text[128];
    snprintf(status_text, sizeof(status_text), "FPS: %d | Entities: %zu", fps, entity_count);
    
    Vector2 pos(20, 40);
    renderer->draw_text(pos, status_text, Color(255, 255, 255, 255));
}

void UIBackend::show_notification(const std::string& message, float duration_sec) {
    notifications.push_back({message, duration_sec});
}

void UIBackend::update(float delta_time) {
    for (auto it = notifications.begin(); it != notifications.end();) {
        it->remaining_time -= delta_time;
        if (it->remaining_time <= 0) {
            it = notifications.erase(it);
        } else {
            ++it;
        }
    }
}

void UIBackend::render() {
    draw_watermark();
    
    // Render notifications
    float notify_y = renderer->get_height() - 40.0f;
    for (const auto& n : notifications) {
        renderer->draw_text(Vector2(20, notify_y), n.message.c_str(), Color(255, 255, 0, 255));
        notify_y -= 15.0f;
    }
}
