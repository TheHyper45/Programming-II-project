#ifndef GAME_HPP
#define GAME_HPP
#include "renderer.hpp"
#include "platform.hpp"
//#include <list>
#include <fstream>
#include <string>

namespace core {
	enum class Scene {
		Main_Menu,
		Select_Level_1player,
		Game_1player,
		Select_Level_2player,
		Game_2player,
		Construction,
	};

	class Game {
	public:
		void menu();
		void graphics(float delta_time);
		void logic();
		
		//void Game_1player();

		Game(core::Renderer* renderer,core::Platform* platform);
		
		class Tank {
		public:
			std::int32_t sprite_index = 15;
			Vec3 position = { 7.5f,4.5f,0 };
			float rotation = 0;
			Vec2 size = { 1.0f,1.0f };
			float direction = 0;
			float distance_to_travel = 0;
			void move(float direction);
			void move_one(float direction);
			void animate(float delta_time);
			void go(float delta_time);
			void shoot();
			

			
		};
		class Barrel {
		public:
			std::int32_t prite_index;
			Vec2 position;
		};
	private:
		Sprite_Index tiles_sprite_atlas;
		Renderer* renderer;
		Platform* platform;
		Scene scene;
		int menu_choice;
		};
}

#endif
