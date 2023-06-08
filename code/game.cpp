#include "game.hpp"
#include <corecrt_math.h>
#include "math.hpp"
#include <stdio.h>
#include "bullets.hpp"
#include <list>

#define number_of_choices 3
using namespace std;
namespace core {
	core::Game::Tank tank;
	std::list<Bullet> List_Of_Bullets{};
	Game::Game(Renderer* _renderer, Platform* _platform) :
		renderer(_renderer), platform(_platform), scene(Scene::Main_Menu), menu_choice() {
		tiles_sprite_atlas = renderer->sprite_atlas("./assets/tiles_16x16.bmp", 16);
	}
	void Game::menu() {
		menu_choice = 0;
	}

	enum class Tile_flag
	{
		Solid,
		Below,
		Above,
		Bulletpass,
		Barrier,
	};
	struct pole
	{
		std::uint32_t sprite_layer_index;
		std::uint32_t health;
		Tile_flag flag;
	};
	pole mapArray[Background_Tile_Count_X * 2][Background_Tile_Count_Y * 2] = { 2, 1, Tile_flag::Below };
	pole* pointertomap;
	//pole polemap[Background_Tile_Count_X*2][Background_Tile_Count_Y*2]
	void SaveMap(std::string filename, pole polemap[Background_Tile_Count_X * 2][Background_Tile_Count_Y * 2])
	{
		int toenumint;
		std::ofstream save;
		save.open(filename, std::ios::out);
		if (save.good())
		{
			for (int i = 0; i < Background_Tile_Count_Y * 2; i++)
			{
				for (int j = 0; j < Background_Tile_Count_X * 2; j++)
				{
					//save << "{";
					save << polemap[i][j].sprite_layer_index << ",";
					save << polemap[i][j].health << ",";
					toenumint = static_cast<int>(polemap[i][j].flag);
					//save << toenumint << "}";
					//save << polemap[i][j].flag;
					save << ";";

				}
			}

		}
		else
		{
			exit(EXIT_FAILURE);
		}
		save.close();
	}
	void SaveMenu(pole polemap[Background_Tile_Count_X * 2][Background_Tile_Count_Y * 2])
	{
		int savechoice = 1;
		
		std::string filename;
		
		switch (savechoice)
		{
		case 1:
		{
			filename = "savefile1.txt";
			SaveMap(filename, polemap);

		}
		case 2 :
		{
			filename = "savefile2.txt";
			SaveMap(filename, polemap);
		}
		case 3:
		{
			filename = "savefile3.txt";
			SaveMap(filename, polemap);
		}
		default:
		{
			break;
		}
		}
	}

	pole mapload(pole polemap[Background_Tile_Count_X * 2][Background_Tile_Count_Y * 2], std::string mapchoice)
	{

		std::ifstream load;
		load.open(mapchoice);
		std::string readdata, readhealth, readflag, readindex;
		int passedindex, passedhealth;

		Tile_flag passedflag;
		if (load.good())
		{
			if (load.eof())
			{
				exit;
			}
			else
			{
				for (int i = 0; i < Background_Tile_Count_Y * 2; i++)
				{
					for (int j = 0; j < Background_Tile_Count_X * 2; j++)
					{
						getline(load, readdata, ',');
						readindex = readdata;
						getline(load, readdata, ',');
						readhealth = readdata;
						getline(load, readdata, ';');
						readflag = readdata;
						passedindex = std::stoi(readindex);
						passedhealth = std::stoi(readhealth);
						if (readflag == "Solid")
						{
							passedflag = Tile_flag::Solid;
						}
						else if (readflag == "Below")
						{
							passedflag = Tile_flag::Below;
						}
						else if (readflag == "Above")
						{
							passedflag = Tile_flag::Above;
						}
						else if (readflag == "Bulletpass")
						{
							passedflag = Tile_flag::Bulletpass;
						}
						else
						{
							passedflag = Tile_flag::Barrier;
						}
						polemap[i][j].sprite_layer_index = passedindex;
						polemap[i][j].health = passedhealth;
						polemap[i][j].flag = passedflag;
						/*
		Solid,
		Below,
		Above,
		Bulletpass,
		Barrier,
						*/
					}
				}
			}

		}
		else
		{
			exit(EXIT_FAILURE);
		}
		return polemap[Background_Tile_Count_X * 2][Background_Tile_Count_Y * 2];
	}

	pole loadmenu(pole polemap[Background_Tile_Count_X * 2][Background_Tile_Count_Y * 2])
	{
		int loadsavedorloadmap;
		std::string mapchoice;
		int mapchoiceint = 0;
		if (loadsavedorloadmap == 1)
		{
			switch (mapchoiceint)

			{
			case 1:
			{
				mapchoice = "map1.txt";
				return mapload(polemap, mapchoice);
			}
			case 2:
			{
				mapchoice = "map2.txt";
				return mapload(polemap, mapchoice);
			}
			case 3:
			{
				mapchoice = "map3.txt";
				return mapload(polemap, mapchoice);
			}
			default:
				break;
			}
		}
		else
		{
			switch (mapchoiceint)

			{
			case 1:
			{
				mapchoice = "savefile1.txt";
				mapload(polemap, mapchoice);
			}
			case 2:
			{
				mapchoice = "savefile1.txt";
				mapload(polemap, mapchoice);
			}
			case 3:
			{
				mapchoice = "savefile1.txt";
				mapload(polemap, mapchoice);
			}
			default:
				break;
			}
		};
	};
		

	void Game::graphics(float delta_time) {
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

		case Scene::Select_Level_1player:
			for (float i = 0; i < Background_Tile_Count_X; i += 1) {
				for (float j = 0; j < Background_Tile_Count_Y; j += 1) {
					renderer->draw_sprite({ 0.5f + i,0.5f + j }, { 1.0f,1.0f }, 0.0f, tiles_sprite_atlas, 5);
				}
			}

			renderer->draw_text({ 1.0f,3.0f,1 }, { 1.0f,1.0f }, { 1.0f,1.0f,1.0f }, "The only one level 1");

			renderer->draw_text({ 2.0f,5.0f,1 }, { 0.5f,0.5f }, { 1.0f,1.0f,1.0f }, "Press Enter to play that level");

			break;


		case Scene::Game_1player:
			for (float i = 0; i < Background_Tile_Count_X; i += 1) {
				for (float j = 0; j < Background_Tile_Count_Y; j += 1) {
					renderer->draw_sprite({ 0.5f + i,0.5f + j,0 }, { 1.0f,1.0f }, 0.0f, tiles_sprite_atlas, 1);
				}
			}
			tank.animate(delta_time);
			renderer->draw_sprite(tank.position, tank.size, (float)tank.rotation, tiles_sprite_atlas, tank.sprite_index);
			for (auto& bullet : List_Of_Bullets) {
				bullet.animate(delta_time);
				bullet.render(renderer, tiles_sprite_atlas);
			}

			break;
		case Scene::Select_Level_2player:
			break;
		case Scene::Game_2player:
			break;
		case Scene::Construction:
			//drawing texture number 1;
			for (float i = 0; i < Background_Tile_Count_X; i += 1) {
				for (float j = 0; j < Background_Tile_Count_Y; j += 1) {
					renderer->draw_sprite({ 0.5f + i,0.5f + j,0 }, { 1.0f,1.0f }, 0.0f, tiles_sprite_atlas, 1);
				}
			}


			//drawing tank
			tank.go(delta_time);
			renderer->draw_sprite(tank.position, tank.size, (float)tank.rotation, tiles_sprite_atlas, tank.sprite_index);

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
				//printf("menu choice: %d\n", menu_choice);
				if (menu_choice < 1) {
					menu_choice+=3;
				}
			}
			if (platform->was_key_pressed(core::Keycode::Up)) {
				menu_choice++;
				if (menu_choice >4) {
					menu_choice -=3;
				}
				//printf("menu choice: %d\n", menu_choice);
			}
			if (platform->was_key_pressed(core::Keycode::Return)) {
				//printf("Execute choice: %d\n", menu_choice);
				if (menu_choice==3||menu_choice==0) {
					scene = Scene::Select_Level_1player;
				}
				if (menu_choice == 2) {
					scene = Scene::Select_Level_2player;
				}
				if (menu_choice == 1||menu_choice==4) {
					scene = Scene::Construction;
				}
			}
			break;
		case Scene::Select_Level_1player:
			if (platform->was_key_pressed(core::Keycode::Return)) {
				//Game_1player();
				
				scene = Scene::Game_1player;
			}
			break;
		case Scene::Game_1player:
			//control
			if (platform->is_key_down(core::Keycode::Up)) {
				tank.move(PI);
			}
			if (platform->is_key_down(core::Keycode::Down)) {
				tank.move(0);
			}
			if (platform->is_key_down(core::Keycode::Right)) {
				tank.move(PI/2);
			}
			if (platform->is_key_down(core::Keycode::Left)) {
				tank.move(3*PI/2);
			}
			//shooting
			if (platform->was_key_pressed(core::Keycode::A) || platform->was_key_pressed(core::Keycode::Space)) {
				List_Of_Bullets.push_front({ 2, tank.direction, tank.position });
				}


			break;
		case Scene::Select_Level_2player:
			if (platform->was_key_pressed(core::Keycode::Return))
				scene = Scene::Game_1player;
			break;
		case Scene::Game_2player:
			break;
		case Scene::Construction:
			//control
			if (platform->was_key_pressed(core::Keycode::Up)) {
				tank.move_one(PI);
			}
			if (platform->was_key_pressed(core::Keycode::Down)) {
				tank.move_one(0);
			}
			if (platform->was_key_pressed(core::Keycode::Right)) {
				tank.move_one(PI/2);
			}
			if (platform->was_key_pressed(core::Keycode::Left)) {
				tank.move_one(3*PI/2);
			}

			if (platform->was_key_pressed(core::Keycode::A) || platform->was_key_pressed(core::Keycode::Space)) {
				//do sth
			}
			break;
		default:
			printf("BAD SCENE\n");
			break;
		}
		
	}


	void Game::Tank::move(float direction)
	{
		this->direction = direction;
		distance_to_travel = 0.01f;
		
	}
	void Game::Tank::move_one(float direction)
	{
		this->direction = direction;
		distance_to_travel = 0.5f;
	}
	void Game::Tank::animate(float delta_time)
	{
		if (distance_to_travel >= 0) {
			const float scale = PI;
			this->distance_to_travel -= delta_time;
			this->position = { this->position.x + scale * delta_time * (float)sin(direction), this->position.y + scale * delta_time * (float)cos(direction),this->position.z };
			this->rotation = (float)direction;
		}

	}
	void Game::Tank::go(float delta_time)
	{
		float scale = 1;
		if(distance_to_travel>0){
		this->position = { this->position.x + scale *  (float)sin(direction), this->position.y + scale *  (float)cos(direction),this->position.z };
		this->rotation = (float)direction;
		this->distance_to_travel=0;
		}
	}


}
