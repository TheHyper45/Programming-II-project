#include <cstdio>
#include <vector>
#include <random>
#include <chrono>
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

        auto random_engine = std::mt19937_64(2683257612ull);
        auto random_dist = std::uniform_int_distribution<int>(0,255);

        std::vector<std::uint8_t> pixels = {};
        pixels.resize(1024);
        for(std::size_t i = 0;i < pixels.size();i += 4) {
            pixels[i + 0] = random_dist(random_engine);
            pixels[i + 1] = random_dist(random_engine);
            pixels[i + 2] = random_dist(random_engine);
            pixels[i + 3] = 255;
        }

        std::vector<std::uint8_t> pixels1 = {};
        pixels1.resize(1024);
        for(std::size_t i = 0;i < pixels1.size();i += 4) {
            pixels1[i + 0] = random_dist(random_engine);
            pixels1[i + 1] = pixels1[i + 0];
            pixels1[i + 2] = pixels1[i + 0];
            pixels1[i + 3] = 255;
        }

        auto texture = renderer.create_texture(16,16,pixels.data());
        auto texture1 = renderer.create_texture(16,16,pixels1.data());

        float pos_x = 512.0f;
        float pos_y = 384.0f;

        auto start_time = std::chrono::steady_clock::now();
        while(!platform.window_closed()) {
            auto end_time = std::chrono::steady_clock::now();
            float delta_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() / 1000000.0f;
            start_time = end_time;

            platform.process_events();

            if(platform.is_key_down(core::Keycode::A)) {
                pos_x -= delta_time * 500.0f;
            }
            if(platform.is_key_down(core::Keycode::D)) {
                pos_x += delta_time * 500.0f;
            }
            if(platform.is_key_down(core::Keycode::W)) {
                pos_y -= delta_time * 500.0f;
            }
            if(platform.is_key_down(core::Keycode::S)) {
                pos_y += delta_time * 500.0f;
            }
            if(platform.was_key_pressed(core::Keycode::Space)) {
                std::cout << "FPS: " << 1.0f / delta_time << std::endl;
            }

            renderer.begin(0.0f,0.0f,0.0f);
            
            for(std::uint32_t x = 0;x < 32;x += 1) {
                for(std::uint32_t y = 0;y < 24;y += 1) {
                    renderer.draw_quad({x * 32.0f,y * 32.0f,0},{32,32},texture1);
                }
            }

            renderer.draw_quad({pos_x,pos_y,0},{32,32},texture);
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
