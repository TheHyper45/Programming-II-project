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

		bool fully_on_screen(Vec3 position, Vec2 size);
		bool on_screen(Vec3 position, Vec2 size);

		Game(core::Renderer* renderer,core::Platform* platform);
		
		class Tank {
		public:
			std::int32_t sprite_index = 15;
			Vec3 position = { 7.5f,10.5f,0 };
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

		
		class Enemy_Tank {
			std::int32_t sprite_index = 14;
			Vec3 position = { 0.5f,0.5f,0 };
			float rotation = 0;
			Vec2 size = { 1.0f,1.0f };
			float direction = 0;
			float distance_to_travel = 0;
			int inteligence = 0;
			int speed = 1;
			int durability = 100;
			int dmg = 100;
			float reloading_time = 10;
		public:
			Enemy_Tank(const std::int32_t& sprite_index, const Vec3& position, float rotation, const Vec2& size, float direction, float distance_to_travel, int inteligence, int speed, int durability, int dmg, float reloading_time)
				: sprite_index(sprite_index), position(position), rotation(rotation), size(size), direction(direction), distance_to_travel(distance_to_travel), inteligence(inteligence), speed(speed), durability(durability), dmg(dmg), reloading_time(reloading_time)
			{
			}
		};
		class Barrel {
		public:
			std::int32_t sprite_index=18;
			Vec3 position;
			Vec2 size;
		};

	private:
		Sprite_Index tiles_sprite_atlas;
		Sprite_Index bullet_alias;
		Sprite_Index player1_alias;
		Sprite_Index eagle_alias;
		Renderer* renderer;
		Platform* platform;
		Scene scene;
		int menu_choice;
		};
}
#endif
