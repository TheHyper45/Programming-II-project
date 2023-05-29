#include <cstdio>
#include <vector>
#include <random>
#include <chrono>
#include <iostream>
#include <exception>
#include "platform.hpp"
#include "renderer.hpp"
#include "exceptions.hpp"
#include "game.hpp"

int main() {
    core::Platform platform = {};
    try {
        platform.create_main_window("Tanks",1024,768);
        
        auto renderer = platform.create_renderer();

        core::Game game(&renderer, &platform);
        game.menu();
        auto start_time = std::chrono::steady_clock::now();
        while(!platform.window_closed()) {
            auto end_time = std::chrono::steady_clock::now();
            float delta_time = float(std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count()) / 1000000.0f;
            start_time = end_time;
            
            platform.process_events();
            game.logic();

            renderer.begin(delta_time);
            game.graphics();
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
