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
        auto renderer = platform.create_renderer();

        auto test0 = renderer.new_sprite("./assets/test0.bmp");
        auto test1 = renderer.new_sprite("./assets/test1.bmp");

        float pos_x = core::Background_Tile_Count_X / 2.0f;
        float pos_y = core::Background_Tile_Count_Y / 2.0f;

        auto start_time = std::chrono::steady_clock::now();
        while(!platform.window_closed()) {
            auto end_time = std::chrono::steady_clock::now();
            float delta_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() / 1000000.0f;
            start_time = end_time;

            platform.process_events();

            if(platform.was_key_pressed(core::Keycode::Space)) {
                std::cout << "FPS: " << (1.0f / delta_time) << std::endl;
            }
            if(platform.is_key_down(core::Keycode::D)) {
                pos_x += 4.0f * delta_time;
            }
            if(platform.is_key_down(core::Keycode::A)) {
                pos_x -= 4.0f * delta_time;
            }
            if(platform.is_key_down(core::Keycode::S)) {
                pos_y += 4.0f * delta_time;
            }
            if(platform.is_key_down(core::Keycode::W)) {
                pos_y -= 4.0f * delta_time;
            }

            renderer.begin(0.0f,0.0f,0.0f);

            renderer.draw_sprite({1,1},{1,1},test0);
            renderer.draw_sprite({pos_x,pos_y},{1,1},test1);
            renderer.draw_sprite({3,1},{0.5f,0.5f},test0);
            renderer.draw_sprite({3.5f,1},{0.5f,0.5f},test1);

            renderer.end();

            platform.swap_window_buffers();
        }
        return 0;
    }
    catch(const core::Runtime_Exception& except) {
        platform.error_message_box(except.message());
        return 1;
    }
    catch(const core::File_Open_Exception& except) {
        char buffer[1024] = {};
        std::snprintf(buffer,sizeof(buffer) - 1,"Couldn't open file \"%s\".",except.file_path());
        platform.error_message_box(buffer);
        return 1;
    }
    catch(const core::File_Read_Exception& except) {
        char buffer[1024] = {};
        std::snprintf(buffer,sizeof(buffer) - 1,"Couldn't read %zu bytes from file \"%s\".",except.byte_count(),except.file_path());
        platform.error_message_box(buffer);
        return 1;
    }
    catch(const core::File_Seek_Exception& except) {
        char buffer[1024] = {};
        std::snprintf(buffer,sizeof(buffer) - 1,"Couldn't seek %zu bytes in file \"%s\".",except.byte_offset(),except.file_path());
        platform.error_message_box(buffer);
        return 1;
    }
    catch(const core::File_Exception& except) {
        char buffer[1024] = {};
        std::snprintf(buffer,sizeof(buffer) - 1,"Error during reading file \"%s\": %s.",except.file_path(),except.message());
        platform.error_message_box(buffer);
        return 1;
    }
    catch(const core::Object_Creation_Exception& except) {
        char buffer[1024] = {};
        std::snprintf(buffer,sizeof(buffer) - 1,"Couldn't create an object of type \"%s\".",except.object_type());
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
