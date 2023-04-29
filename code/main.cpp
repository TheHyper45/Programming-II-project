#include <iostream>
#include <exception>
#include "platform.hpp"
#include "exceptions.hpp"

int main() {
    core::Platform platform = {};
    try {
        platform.create_main_window("Tanks",1024,768);
        while(!platform.window_closed()) {
            platform.process_events();
            platform.swap_buffers();
        }
    }
    catch(const core::Runtime_Exception& except) {
        platform.error_message_box(except.message());
    }
    catch(const std::exception& except) {
        platform.error_message_box(except.what());
    }
}
