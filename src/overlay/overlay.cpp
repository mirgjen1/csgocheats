#include "overlay/overlay.hpp"

#ifdef _WIN32
#include "rendering/dx11_renderer.hpp"
#include "overlay/overlay_window.hpp"
#else
#include "rendering/opengl_renderer.hpp"
#include "memory/memory_reader.hpp"
#endif

Overlay::~Overlay() {
    stop();
}

bool Overlay::initialize(const Config& cfg) {
    config = cfg;
    
    // Create memory reader
#ifdef _WIN32
    memory_reader = std::make_shared<WindowsMemoryReader>(L"csgo.exe");
#else
    // Linux: use process name from config
    memory_reader = std::make_shared<LinuxMemoryReader>(config.process_name);
#endif
    
    if (!memory_reader) {
        return false;
    }
    
    // Create game memory interface
    game_memory = std::make_shared<GameMemory>(memory_reader);
    if (!game_memory) {
        return false;
    }
    
    // Create renderer - use DirectX 11 on Windows, OpenGL on Linux
#ifdef _WIN32
    auto dx11_renderer = std::make_shared<DirectX11Renderer>();
    
    // Create overlay window
    auto overlay_window = std::make_shared<OverlayWindow>();
    if (!overlay_window->create(config.window_width, config.window_height, config.window_title)) {
        return false;
    }
    
    // Set window handle for DirectX rendering
    dx11_renderer->set_window_handle(overlay_window->get_handle());
    
    // Initialize renderer
    if (!dx11_renderer->initialize(config.window_width, config.window_height)) {
        return false;
    }
    
    renderer = dx11_renderer;
    overlay_win = overlay_window;
#else
    // Use OpenGL renderer on Linux
    auto opengl_renderer = std::make_shared<OpenGLRenderer>();
    if (!opengl_renderer->initialize(config.window_width, config.window_height)) {
        return false;
    }
    renderer = opengl_renderer;
#endif
    
    // Create entity manager
    entity_manager = std::make_shared<EntityManager>(game_memory, renderer);
    entity_manager->set_config(config.entity_config);
    
    running = true;
    return true;
}

void Overlay::run() {
    if (!renderer || !renderer->is_ready()) {
        running = false;
        return;
    }
    
    while (running) {
#ifdef _WIN32
        // Process window messages
        if (overlay_win) {
            overlay_win->process_messages();
            if (!overlay_win->is_active()) {
                running = false;
                break;
            }
        }
#endif
        
        // Update entity data
        if (entity_manager) {
            entity_manager->update();
        }
        
        // Render frame
        render_frame();
        
        // Small sleep to prevent 100% CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }
}

void Overlay::stop() {
    running = false;
}

bool Overlay::is_running() const {
    return running;
}

EntityManagerPtr Overlay::get_entity_manager() {
    return entity_manager;
}

void Overlay::set_render_callback(RenderCallback callback) {
    custom_render = callback;
}

void Overlay::render_frame() {
    if (!renderer) return;
    
    // Begin frame
    renderer->begin_frame();
    
    // Render entities
    if (entity_manager) {
        entity_manager->render();
    }
    
    // Call custom render callback if set
    if (custom_render) {
        custom_render(renderer);
    }
    
    // End frame and present
    renderer->end_frame();
}
