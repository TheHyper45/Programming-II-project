#ifndef GAME_HPP
#define GAME_HPP
#include <stdio.h>
#include "renderer.hpp"
#include "platform.hpp"
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
		void graphics();
		void logic();
		Game(core::Renderer* renderer,core::Platform* platform);
		
	private:
		Sprite_Index tiles_sprite_atlas;
		Renderer* renderer;
		Platform* platform;
		Scene scene;
		int menu_choice;
	};
}

#endif
