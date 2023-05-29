#include "game.hpp"
#define number_of_choices 3
namespace core {
	Game::Game(Renderer* _renderer,Platform* _platform) :
		renderer(_renderer),platform(_platform),scene(Scene::Main_Menu),menu_choice() {
		tiles_sprite_atlas = renderer->sprite_atlas("./assets/tiles_16x16.bmp",16);
		menu_choice = 0;
	}

	void Game::menu() {
		menu_choice = 0;
	}
	

	void Game::graphics() {
		switch (scene) {
		case Scene::Main_Menu:
			for(float i = 0;i < Background_Tile_Count_X;i += 1) {
				for(float j = 0;j < Background_Tile_Count_Y;j += 1) {
					renderer->draw_sprite({0.5f + i,0.5f + j},{1.0f,1.0f},0.0f,tiles_sprite_atlas,4);
				}
			}
			renderer->draw_text({1.0f,3.0f,1},{0.8f,1.0f},{1.0f,1.0f,1.0f},"Projekt na programowanie II");
			// to replace;
			
			renderer->draw_text({ 2.0f,4.0f,1 }, { 0.8f,1.0f }, { 1.0f,1.0f,(float)((menu_choice+2) % number_of_choices==2 ? 0 : 1) }, "1 player");
			renderer->draw_text({ 2.0f,5.0f,1 }, { 0.8f,1.0f }, { 1.0f,1.0f,(float)((menu_choice+2) % number_of_choices == 1 ? 0 : 1) }, "2 player");
			renderer->draw_text({ 2.0f,6.0f,1 }, { 0.8f,1.0f }, { 1.0f,1.0f,(float)((menu_choice+2) % number_of_choices == 0 ? 0 : 1) }, "Construction");
			break;
		case Scene::Select_Lecel_1player:
			for (float i = 0; i < Background_Tile_Count_X; i += 1) {
				for (float j = 0; j < Background_Tile_Count_Y; j += 1) {
					renderer->draw_sprite({ 0.5f + i,0.5f + j }, { 1.0f,1.0f }, 0.0f, tiles_sprite_atlas, 5);
				}
			}

			renderer->draw_text({ 1.0f,3.0f,1 }, { 1.0f,1.0f }, { 1.0f,1.0f,1.0f }, "The only one level 1");

			renderer->draw_text({ 2.0f,5.0f,1 }, { 0.5f,0.5f }, { 1.0f,1.0f,1.0f }, "Press Enter to play that level");
			break;
		case Scene::Game_1player:
		
			break;
		case Scene::Select_Lecel_2player:
			break;
		case Scene::Game_2player:
			break;
		case Scene::Construction:
			break;
		default:
			break;
		}
	}

	void Game::logic() {
		switch (scene) {
		case Scene::Main_Menu:
			if (platform->was_key_pressed(core::Keycode::Down)) {
				menu_choice--;
				printf("menu choice: %d\n", menu_choice);
				if (menu_choice < 1) {
					menu_choice+=3;
				}
			}
			if (platform->was_key_pressed(core::Keycode::Up)) {
				menu_choice++;
				if (menu_choice >4) {
					menu_choice -=3;
				}
				printf("menu choice: %d\n", menu_choice);
			}
			if (platform->was_key_pressed(core::Keycode::Return)) {
				printf("Execute choice: %d\n", menu_choice);
				if (menu_choice==3||menu_choice==0) {
					scene = Scene::Select_Lecel_1player;
				}
				if (menu_choice == 2) {
					scene = Scene::Select_Lecel_2player;
				}
				if (menu_choice == 1||menu_choice==4) {
					scene = Scene::Construction;
				}
			}
			break;
		case Scene::Select_Lecel_1player:
			if (platform->was_key_pressed(core::Keycode::Return))
				scene = Scene::Game_1player;
			break;
		case Scene::Game_1player:
			break;
		case Scene::Select_Lecel_2player:
			if (platform->was_key_pressed(core::Keycode::Return))
				scene = Scene::Game_1player;
			break;
		case Scene::Game_2player:
			break;
		case Scene::Construction:
			break;
		default:
			printf("BAD SCENE\n");
			break;
		}
		
	}
}
