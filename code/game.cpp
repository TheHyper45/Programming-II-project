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
	//core::Game::Tank tank2;
	std::list<Bullet> List_Of_Bullets{};
	std::list<core::Game::Enemy_Tank> List_Of_Enemy_Tanks{};
	std::list<core::Game::Barrel>List_Of_Barrels{};
	Game::Game(Renderer* _renderer, Platform* _platform) :
		renderer(_renderer), platform(_platform), scene(Scene::Main_Menu), menu_choice() {
		tiles_sprite_atlas = renderer->sprite_atlas("./assets/tiles_16x16.bmp", 16);
		
		//bullet_alias = renderer->sprite("./assets/bullet.jpg");
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
	struct playerstatus
	{
		float x;
		float y;
		float z;
		float rotation;
		float direction;
	};
	pole mapArray[Background_Tile_Count_X * 2][Background_Tile_Count_Y * 2];
	playerstatus playerstatus1= { 0,0,0,0,0 };
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
					if (i == 0 || i == Background_Tile_Count_X * 2 - 1 || j == 0 || j == Background_Tile_Count_Y * 2 - 1)
					{

					}
					//save << "{";
					save << polemap[i][j].sprite_layer_index << ",";
					save << polemap[i][j].health << ",";
					toenumint = static_cast<int>(polemap[i][j].flag);
					std::cout << toenumint;
					save << toenumint;
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
	void savePlayerStatus(core::Game::Tank tank, string filename2)
	{
		float x, y, z, direction, rotation;
		x = tank.position.x;
		y = tank.position.y;
		z = tank.position.z;
		rotation = tank.rotation;
		direction = tank.direction;
		std::ofstream savestatus;
		savestatus.open(filename2, std::ios::out);
		if (savestatus.good())
		{
			savestatus << x << ",";
			savestatus << y << ",";
			savestatus << z << ",";
			savestatus << direction << ",";
			savestatus << rotation << ".";
		}
		else
		{
			exit(EXIT_FAILURE);
		}
		savestatus.close();

	}
	void SaveMenu(pole polemap[Background_Tile_Count_X * 2][Background_Tile_Count_Y * 2], core::Game::Tank tank)
	{
		int savechoice = 1;
		
		std::string filename,filename2;
		
		switch (savechoice)
		{
		case 1:
		{
			filename = "savefile1.txt";
			filename2 = "statussave1.txt";
			SaveMap(filename, polemap);
			//savePlayerStatus(tank, filename2);

		}
		case 2 :
		{
			filename = "savefile2.txt";
			filename2 = "statussave2.txt";
			SaveMap(filename, polemap);
			//savePlayerStatus(tank, filename2);
		}
		case 3:
		{
			filename = "savefile3.txt";
			filename2 = "statussave3.txt";
			SaveMap(filename, polemap);
			//savePlayerStatus(tank, filename2);
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
						else if(readflag =="Barrier")
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
	playerstatus loadstatus(std::string mapchoice ,playerstatus loaddata)
	{
		float x;
		std::string readdata;
		
		std::ifstream loadstatus;
		loadstatus.open(mapchoice);
		if (loadstatus.good())
		{
			if (loadstatus.eof())
			{
				exit;
			}
			else
			{
				getline(loadstatus, readdata,',');
				x = stof(readdata);
				playerstatus1.x = x;
				getline(loadstatus, readdata, ',');
				x = stof(readdata);
				playerstatus1.y = x;
				getline(loadstatus, readdata, ',');
				x = stof(readdata);
				playerstatus1.z = x;
				getline(loadstatus, readdata, ',');
				x = stof(readdata);
				playerstatus1.direction = x;
				getline(loadstatus, readdata, '.');
				x = stof(readdata);
				playerstatus1.rotation = x;
			}
		}
		else
		{
			exit(EXIT_FAILURE);
		}
		loadstatus.close();
		return loaddata;
	}
	playerstatus loadstatusmenu(int mapchoiceint, playerstatus playerstatus1)
	{
		std::string mapchoice;
		switch (mapchoiceint)
		
		{
		case 1:
		{
			mapchoice = "statussave1.txt";
			return loadstatus(mapchoice, playerstatus1);
		}
		case 2:
		{
			mapchoice = "statussave2.txt";
			return loadstatus(mapchoice, playerstatus1);
		}
		case 3:
		{
			mapchoice = "statussave1.txt";
			return loadstatus(mapchoice, playerstatus1);
		}
		default:
			break;
		}
		
	}
	pole loadmenu(pole polemap[Background_Tile_Count_X * 2][Background_Tile_Count_Y * 2],int mapchoiceint)
	{
		int loadsavedorloadmap = 2;
		std::string mapchoice;
		
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
				return mapload(polemap, mapchoice);
			}
			case 2:
			{
				mapchoice = "savefile1.txt";
				return mapload(polemap, mapchoice);
			}
			case 3:
			{
				mapchoice = "savefile1.txt";
				return mapload(polemap, mapchoice);
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
			renderer->draw_text({ 2.0f,6.5f,1 }, { 0.8f,1.0f }, { 1.0f,1.0f,(float)((menu_choice + 2) % number_of_choices == 0 ? 0 : 1) }, "Load Game");
			renderer->draw_text({ 2.0f,7.5f,1 }, { 0.8f,1.0f }, { 1.0f,1.0f,(float)((menu_choice+2) % number_of_choices == -1 ? 0 : 1) }, "Construction");
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

//Game_1player
		case Scene::Game_1player:
			int a, b;
			for (float i = 0; i < Background_Tile_Count_X; i += 1) {
				for (float j = 0; j < Background_Tile_Count_Y; j += 1) 
				{
					a = static_cast<int>(i);
					b = static_cast<int>(j);
					renderer->draw_sprite({ 0.5f + i,0.5f + j,0 }, { 1.0f,1.0f }, 0.0f, tiles_sprite_atlas, mapArray[a][b].sprite_layer_index);
				}
			}
			tank.animate(delta_time);
			renderer->draw_sprite(tank.position, tank.size, (float)tank.rotation, tiles_sprite_atlas, tank.sprite_index);

			for (auto& bullet : List_Of_Bullets) {
				bullet.animate(delta_time);
				bullet.render(renderer, tiles_sprite_atlas);
				if (!on_screen(bullet.position, bullet.size)) {
					List_Of_Bullets.remove(bullet);
					goto do_not_crash_doing_bullets_loop;
				}
			}
		do_not_crash_doing_bullets_loop:;

			for (auto& barrel : List_Of_Barrels) {
				renderer->draw_sprite(barrel.position, barrel.size, 0, tiles_sprite_atlas, barrel.sprite_index);
			}
		do_not_crash_doing_barrels_loop:;

			break;
		case Scene::Select_Level_2player:
			break;
		case Scene::Game_2player:
			break;
		case Scene::Construction:
			int c, d;
			//drawing texture number 1;
			for (float i = 0; i < Background_Tile_Count_X; i += 1) {
				for (float j = 0; j < Background_Tile_Count_Y; j += 1) 
				{
					c = static_cast<int>(i);
					d = static_cast<int>(j);
					renderer->draw_sprite({ 0.5f + i,0.5f + j,0 }, { 1.0f,1.0f }, 0.0f, tiles_sprite_atlas, mapArray[c][d].sprite_layer_index);
				}
			}

			//drawing tank
			tank.go(delta_time);
			renderer->draw_sprite(tank.position, tank.size, (float)tank.rotation, tiles_sprite_atlas, tank.sprite_index);

			break;
		case Scene::Load_game:
			{
			renderer->draw_text({ 1.0f,3.0f,1 }, { 1.0f,1.0f }, { 1.0f,1.0f,1.0f }, "Choose save file");
			}
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

		//debuging 
			
			if (platform->was_key_pressed(core::Keycode::A)) {
				printf("TANK X Y Z: %f %f %f\n", tank.position.x, tank.position.y, tank.position.z);
			}
			
			if (platform->was_key_pressed(core::Keycode::B)) {
				
				List_Of_Barrels.push_front({ 18,{5.5f,5.5f,0.2f},{1.0f,1.0f} });
				printf("Added Barrel\n");
			}
			if (platform->was_key_pressed(core::Keycode::T)) 
			{
				SaveMenu(mapArray, tank);
				printf("saving \n");
			}
			if (platform->was_key_pressed(core::Keycode::Y)) 
			{
				mapArray[Background_Tile_Count_X * 2][Background_Tile_Count_Y * 2] = loadmenu(mapArray,1);
				printf("loading, \n ");

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

	bool Game::fully_on_screen(Vec3 position, Vec2 size)
	{
		if (position.x-size.x/2 > 0 && position.y-size.y/2 > 0 && position.x + size.x / 2 <Background_Tile_Count_X && position.y + size.y / 2 < Background_Tile_Count_Y) return true;

		return false;
	}
	bool Game::on_screen(Vec3 position, Vec2 size)
	{
		if (position.x + size.x / 2 > 0 && position.y + size.y / 2 > 0 && position.x - size.x / 2 < Background_Tile_Count_X && position.y - size.y / 2 < Background_Tile_Count_Y) return true;

		return false;
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
			this->rotation = (float)direction;

			//be inside of window
			Vec3 next_position;
			next_position= { this->position.x + scale * delta_time * (float)sin(direction), this->position.y + scale * delta_time * (float)cos(direction),this->position.z };
			if (next_position.x - size.x / 2 > 0 && next_position.y - size.y / 2 > 0 && next_position.x + size.x / 2 < Background_Tile_Count_X && next_position.y + size.y / 2 < Background_Tile_Count_Y) {
				this->position = next_position;
			}

		}

	}
	void Game::Tank::go(float delta_time)
	{
		float scale = 1;
		if(distance_to_travel>0){
			Vec3 next_position;
			next_position = { this->position.x + scale * (float)sin(direction), this->position.y + scale  * (float)cos(direction),this->position.z };
			if (next_position.x> 0 && next_position.y > 0 && next_position.x < Background_Tile_Count_X && next_position.y < Background_Tile_Count_Y) {
				this->position = next_position;
			}
		//this->position = { this->position.x + scale *  (float)sin(direction), this->position.y + scale *  (float)cos(direction),this->position.z };
		this->rotation = (float)direction;
		this->distance_to_travel=0;
		}
	}
}
