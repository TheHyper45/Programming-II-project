#include <cstdio>
#include <vector>
#include <random>
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

        /*std::uint8_t pixels[] = {
            0,0,0,255, 255,255,255,255, 0,0,0,255, 255,255,255,255, 0,0,0,255, 255,255,255,255, 0,0,0,255, 255,255,255,255,
            255,255,255,255, 0,0,0,255, 255,255,255,255, 0,0,0,255, 255,255,255,255, 0,0,0,255, 255,255,255,255, 0,0,0,255,
            0,0,0,255, 255,255,255,255, 0,0,0,255, 255,255,255,255, 0,0,0,255, 255,255,255,255, 0,0,0,255, 255,255,255,255,
            255,255,255,255, 0,0,0,255, 255,255,255,255, 0,0,0,255, 255,255,255,255, 0,0,0,255, 255,255,255,255, 0,0,0,255,
            0,0,0,255, 255,255,255,255, 0,0,0,255, 255,255,255,255, 0,0,0,255, 255,255,255,255, 0,0,0,255, 255,255,255,255,
            255,255,255,255, 0,0,0,255, 255,255,255,255, 0,0,0,255, 255,255,255,255, 0,0,0,255, 255,255,255,255, 0,0,0,255,
            0,0,0,255, 255,255,255,255, 0,0,0,255, 255,255,255,255, 0,0,0,255, 255,255,255,255, 0,0,0,255, 255,255,255,255,
            255,255,255,255, 0,0,0,255, 255,255,255,255, 0,0,0,255, 255,255,255,255, 0,0,0,255, 255,255,255,255, 0,0,0,255,
        };*/

        auto random_engine = std::mt19937_64(2683257612ull);
        auto random_dist = std::uniform_int_distribution<int>(0,255);

        std::vector<std::uint8_t> pixels = {};
        pixels.resize(1024);
        for(std::size_t i = 0;i < pixels.size();i += 1) pixels[i] = random_dist(random_engine);

        auto texture = renderer.create_texture(16,16,pixels.data());

        while(!platform.window_closed()) {
            platform.process_events();

            renderer.begin(0.0f,0.0f,0.0f,texture);
            renderer.end();

            platform.swap_window_buffers();
        }
        return 0;
    }
    catch(const core::Runtime_Exception& except) {
        platform.error_message_box(except.message());
        return 1;
    }
    catch(const core::Tagged_Runtime_Exception<const char*>& except) {
        char buffer[1024] = {};
        std::snprintf(buffer,sizeof(buffer) - 1,"%s (\"%s\").",except.message(),except.value());
        platform.error_message_box(buffer);
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
