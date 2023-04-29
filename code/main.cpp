#include <cstdio>
#include <iostream>
#include <exception>
#include "platform.hpp"
#include "renderer.hpp"
#include "exceptions.hpp"

int main() {
    core::Platform platform = {};
    try {
        platform.create_main_window("Tanks",1024,768);

        auto renderer = core::Renderer(&platform);
        while(!platform.window_closed()) {
            platform.process_events();

            renderer.begin(0.0f,0.75f,1.0f);
            renderer.end();

            platform.swap_window_buffers();
        }
        return 0;
    }
    catch(const core::Runtime_Exception& except) {
        platform.error_message_box(except.message());
        return 1;
    }
    catch(const core::Tagged_Runtime_Exception<unsigned int>& except) {
        char buffer[1024] = {};
        std::snprintf(buffer,sizeof(buffer) - 1,"%s (%u).",except.message(),except.value());
        platform.error_message_box(buffer);
        return 1;
    }
    catch(const std::exception& except) {
        platform.error_message_box(except.what());
        return 1;
    }
    catch(...) {
        platform.error_message_box("Unknown exception type. Sorry for screwing up :(.");
        return 1;
    }
}
