#include "overlay/overlay.hpp"

Overlay::~Overlay() {
    stop();
}

bool Overlay::initialize(const Config& cfg) {
    config = cfg;
    
    // Create memory reader
#ifdef _WIN32
    memory_reader = std::make_shared<WindowsMemoryReader>(L"cs2.exe");
#else
    // For Linux, would need process ID
    memory_reader = std::make_shared<MockMemoryReader>();
#endif
    
    if (!memory_reader) {
        return false;
    }
    
    // Create game memory interface
    game_memory = std::make_shared<GameMemory>(memory_reader);
    if (!game_memory) {
        return false;
    }
    
    // Create renderer
    renderer = std::make_shared<OverlayRenderer>();
    if (!renderer || !renderer->initialize(config.window_width, config.window_height)) {
        return false;
    }
    
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
