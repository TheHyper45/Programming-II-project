#include <iostream>
#include <exception>
#include "platform.hpp"
#include "renderer.hpp"
#include "exceptions.hpp"

int main() {
    core::Platform platform = {};
    try {
        platform.create_main_window("Tanks",1024,768);

        core::Renderer renderer = {};
        while(!platform.window_closed()) {
            platform.process_events();

            renderer.begin(0.0f,0.75f,1.0f);
            renderer.end();

            platform.swap_window_buffers();
        }
    }
    catch(const core::Runtime_Exception& except) {
        platform.error_message_box(except.message());
    }
    catch(const std::exception& except) {
        platform.error_message_box(except.what());
    }
}
