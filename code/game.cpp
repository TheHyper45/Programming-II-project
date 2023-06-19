#include <cstdio>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <cinttypes>
#include "game.hpp"
#include "exceptions.hpp"
#include <Windows.h>
namespace core {
	static constexpr float Main_Menu_First_Option_Y_Offset = 7.0f;
	static constexpr const char* Main_Menu_Options[] = {"1 player","2 players","Load Level","Construction","Quit"};
	static constexpr std::size_t Main_Menu_Options_Count = sizeof(Main_Menu_Options) / sizeof(*Main_Menu_Options);
	static constexpr float Map_First_Option_Y_Offset = 5.0f;
	static constexpr const char* Map_Options[] = {"Level 1","Level 2","Level 3","Level 4","Level 5 ","Search in windows"};
	static constexpr std::size_t Map_Options_Count = sizeof(Map_Options) / sizeof(*Map_Options);
	static constexpr float Players_Mode_Option_Y_Offset = 4.0f;
	static constexpr const char* Players_Mode_Options[] = { "1 Player","2 Players" };
	static constexpr std::size_t Players_Mode_Options_Count = sizeof(Players_Mode_Options) / sizeof(*Players_Mode_Options);
	static constexpr float Tank_Speed = 4.0f;
	static constexpr float Bullet_Speed = 12.0f;
	static constexpr std::uint32_t Player_Tank_Sprite_Layer_Index = 0;
	static constexpr std::uint32_t Second_Player_Tank_Sprite_Layer_Index = 4;
	static constexpr std::uint32_t Enemy_Tank_Sprite_Layer_Index = 6;
	static constexpr std::uint32_t Bullet_Sprite_Layer_Index = 10;
	static constexpr std::uint32_t Eagle_Sprite_Layer_Index = 11;
	static constexpr Vec2 Eagle_Size = {1.0f,1.0f};
	static constexpr Vec2 Tank_Size = {1.0f,1.0f};
	static constexpr Vec2 Bullet_Size = {1.0f,1.0f};
	static constexpr Vec2 Enemy_Spawner_Locations[] = {{1,1},{Background_Tile_Count_X / 2,1},{Background_Tile_Count_X - 1 - 0.001f,1}};
	static constexpr std::size_t Enemy_Spawner_Location_Count = sizeof(Enemy_Spawner_Locations) / sizeof(*Enemy_Spawner_Locations);
	static constexpr std::uint32_t Spawn_Effect_Layer_Count = 7;
	static constexpr float Spawn_Effect_Frame_Duration = 0.1f;
	static constexpr float Tank_Shoot_Cooldown = 0.4f;
	static constexpr float Enemy_Spawn_Time = 2.0f;
	static constexpr float Collision_Offset = 0.1f;
	static constexpr std::uint32_t Player_Starting_Life_Count = 3;
	static constexpr std::uint32_t Enemy_Hop_Count_To_Shoot = 10;
	static constexpr Entity_Direction_Triple Entity_Direction_Triples[] = {
		{Entity_Direction::Down,Entity_Direction::Up,Entity_Direction::Left},
		{Entity_Direction::Right,Entity_Direction::Left,Entity_Direction::Up},
		{Entity_Direction::Down,Entity_Direction::Up,Entity_Direction::Right},
		{Entity_Direction::Right,Entity_Direction::Left,Entity_Direction::Down}
	};

	//Bounding boxes for each 'Entity_Direction' value in order.
	static constexpr Rect Bullet_Bounding_Boxes[] = {{0.28125f,0.4375f,0.40625f,0.1875f},{0.4375f,0.28125f,0.1875f,0.40625f},{0.3125f,0.40625f,0.40625f,0.1875f},{0.4375f,0.3125f,0.1875f,0.40625f}};
	static constexpr Vec2 Tank_Bullet_Firing_Positions[] = {{0.6f,0.0f},{0.0f,0.6f},{-0.6f,0.0f},{0.0f,-0.6f}};

	Game::Game(Renderer* _renderer,Platform* _platform) : renderer(_renderer),platform(_platform),scene(Scene::Main_Menu),
		current_main_menu_option(),update_timer(),construction_marker_pos(),construction_choosing_tile(),construction_tile_choice_marker_pos(),
		construction_current_tile_template_index(),tile_templates(),show_fps(),quit(),tiles(),eagle(),game_lose_timer(),spawn_effects(),enemy_tanks(),
		random_engine(),enamy_spawn_point_random_dist(),enemy_action_duration_dist(),chance_0_1_dist(),max_enemy_count_on_screen(),remaining_enemy_count_to_spawn(),
		enemy_spawn_timer(),game_win_timer(),current_stage_index(),first_player(),second_player() {
		random_engine = std::minstd_rand0(std::time(nullptr));
		enamy_spawn_point_random_dist = std::uniform_int_distribution<std::size_t>(0,Enemy_Spawner_Location_Count - 1);
		enemy_action_duration_dist = std::uniform_real_distribution<float>(0.5f,3.0f);
		chance_0_1_dist = std::uniform_real_distribution<float>(0.0f,1.0f);

		tiles_texture = renderer->sprite_atlas("./assets/tiles_16x16.bmp",16);
		construction_place_marker = renderer->sprite("./assets/marker.bmp");
		entity_sprites = renderer->sprite_atlas("./assets/entities_32x32.bmp",32);
		spawn_effect_sprite_atlas = renderer->sprite_atlas("./assets/spawn_effect_32x32.bmp",32);
		explosion_sprite = renderer->sprite_atlas("./assets/explosions_16x16.bmp",16);

		{
			static constexpr const char* Tiles_Info_File_Path = "./assets/tiles_16x16.txt";
			std::ifstream file{Tiles_Info_File_Path,std::ios::binary};
			if(!file.is_open()) throw File_Open_Exception(Tiles_Info_File_Path);

			std::string line{};
			while(std::getline(file,line)) {
				//Skip comments.
				if(line.size() < 2 || line[0] == '#') continue;

				std::uint32_t index_buffer = 0;
				char health_buffer[32] = {};
				char flag_buffer[32] = {};
				std::uint32_t rotation = 0;

				int count = std::sscanf(line.c_str(),"%" SCNu32 " %31s %" SCNu32 " %31s",&index_buffer,health_buffer,&rotation,flag_buffer);
				if(count < 0) throw File_Exception(Tiles_Info_File_Path,"Invalid format.");

				Tile_Template tile_template{};
				tile_template.tile_layer_index = index_buffer;

				switch(rotation) {
					case 0: tile_template.rotation = 0.0f; break;
					case 1: tile_template.rotation = PI / 2.0f; break;
					case 2: tile_template.rotation = PI; break;
					case 3: tile_template.rotation = 3.0f * PI / 2.0f; break;
					default: throw File_Exception(Tiles_Info_File_Path,"Invalid rotation value.");
				}

				if(std::strcmp(health_buffer,"-1") == 0) {
					tile_template.health = std::uint32_t(-1);
				}
				else {
					count = std::sscanf(health_buffer,"%" SCNu32,&tile_template.health);
					if(count < 0) throw File_Exception(Tiles_Info_File_Path,"Invalid format.");
				}

				if(std::strcmp(flag_buffer,"solid") == 0) {
					tile_template.flag = Tile_Flag::Solid;
				}
				else if(std::strcmp(flag_buffer,"below") == 0) {
					tile_template.flag = Tile_Flag::Below;
				}
				else if(std::strcmp(flag_buffer,"above") == 0) {
					tile_template.flag = Tile_Flag::Above;
				}
				else if(std::strcmp(flag_buffer,"bulletpass") == 0) {
					tile_template.flag = Tile_Flag::Bulletpass;
				}
				else throw File_Exception(Tiles_Info_File_Path,"Invalid format.");

				tile_templates.push_back(tile_template);
			}
		}
		load_map("./assets/maps/map_menu.txt");
	}

	std::optional<Ipoint> Game::check_collision_with_tiles(Vec2* out_position,Vec2 collider_size,std::int32_t start_x,std::int32_t start_y,std::int32_t end_x,std::int32_t end_y,Entity_Direction dir,bool is_bullet) {
		if(dir == Entity_Direction::Right && (end_x >= 0 && end_x < Background_Tile_Count_X * 2)) {
			for(std::int32_t y = start_y;y <= end_y;y += 1) {
				if(y < 0 || y >= Background_Tile_Count_Y * 2) continue;
				auto& tile = tiles[y * (Background_Tile_Count_X * 2) + end_x];
				if(tile.template_index == Invalid_Tile_Index) continue;

				const auto& tile_template = tile_templates[tile.template_index];
				if(is_bullet) {
					if(tile_template.flag != Tile_Flag::Solid) continue;
				}
				else {
					if(tile_template.flag != Tile_Flag::Solid && tile_template.flag != Tile_Flag::Bulletpass) continue;
				}
				out_position->x = end_x / 2.0f - collider_size.x / 2.0f - 0.001f;
				return Ipoint{end_x,y};
			}
		}
		if(dir == Entity_Direction::Down && (end_y >= 0 && end_y < Background_Tile_Count_Y * 2)) {
			for(std::int32_t x = start_x;x <= end_x;x += 1) {
				if(x < 0 || x >= Background_Tile_Count_X * 2) continue;
				auto& tile = tiles[end_y * (Background_Tile_Count_X * 2) + x];
				if(tile.template_index == Invalid_Tile_Index) continue;

				const auto& tile_template = tile_templates[tile.template_index];
				if(is_bullet) {
					if(tile_template.flag != Tile_Flag::Solid) continue;
				}
				else {
					if(tile_template.flag != Tile_Flag::Solid && tile_template.flag != Tile_Flag::Bulletpass) continue;
				}
				out_position->y = end_y / 2.0f - collider_size.y / 2.0f - 0.001f;
				return Ipoint{x,end_y};
			}
		}
		if(dir == Entity_Direction::Left && (start_x >= 0 && start_x < Background_Tile_Count_X * 2)) {
			for(std::int32_t y = start_y;y <= end_y;y += 1) {
				if(y < 0 || y >= Background_Tile_Count_Y * 2) continue;
				auto& tile = tiles[y * (Background_Tile_Count_X * 2) + start_x];
				if(tile.template_index == Invalid_Tile_Index) continue;

				const auto& tile_template = tile_templates[tile.template_index];
				if(is_bullet) {
					if(tile_template.flag != Tile_Flag::Solid) continue;
				}
				else {
					if(tile_template.flag != Tile_Flag::Solid && tile_template.flag != Tile_Flag::Bulletpass) continue;
				}
				out_position->x = start_x / 2.0f + collider_size.x + 0.001f;
				return Ipoint{start_x,y};
			}
		}
		if(dir == Entity_Direction::Up && (start_y >= 0 && start_y < Background_Tile_Count_Y * 2)) {
			for(std::int32_t x = start_x;x <= end_x;x += 1) {
				if(x < 0 || x >= Background_Tile_Count_X * 2) continue;
				auto& tile = tiles[start_y * (Background_Tile_Count_X * 2) + x];
				if(tile.template_index == Invalid_Tile_Index) continue;

				const auto& tile_template = tile_templates[tile.template_index];
				if(is_bullet) {
					if(tile_template.flag != Tile_Flag::Solid) continue;
				}
				else {
					if(tile_template.flag != Tile_Flag::Solid && tile_template.flag != Tile_Flag::Bulletpass) continue;
				}
				out_position->y = start_y / 2.0f + collider_size.y + 0.001f;
				return Ipoint{x,start_y};
			}
		}
		return {};
	}

	Raycast_Outcome Game::raycast(Vec2 origin,Entity_Direction dir,bool include_bulletpass_tiles,bool skip_tiles,bool skip_targets) {
		Rect first_player_rect = {first_player.tank.position.x - Tank_Size.x / 2.0f,first_player.tank.position.y - Tank_Size.y / 2.0f,Tank_Size.x,Tank_Size.y};
		Rect second_player_rect = {second_player.tank.position.x - Tank_Size.x / 2.0f,second_player.tank.position.y - Tank_Size.y / 2.0f,Tank_Size.x,Tank_Size.y};
		Rect eagle_rect = {eagle.position.x - Eagle_Size.x / 2.0f,eagle.position.y - Eagle_Size.y / 2.0f,Eagle_Size.x,Eagle_Size.y};

		auto align_to_tile_grid = [](Vec2 point,Entity_Direction curr_dir) {
			switch(curr_dir) {
				case Entity_Direction::Right: { point.x = std::floor(point.x * 2.0f) / 2.0f; break; }
				case Entity_Direction::Down: { point.y = std::floor(point.y * 2.0f) / 2.0f; break; }
				case Entity_Direction::Left: { point.x = std::ceil(point.x * 2.0f) / 2.0f; break; }
				case Entity_Direction::Up: { point.y = std::ceil(point.y * 2.0f) / 2.0f; break; }
			}
			return point;
		};

		auto increment = core::entity_direction_to_vector(dir) * 0.5f;
		for(;;origin += increment) {
			if(origin.x < 0.0f || origin.x >= (float(Background_Tile_Count_X))) break;
			if(origin.y < 0.0f || origin.y >= (float(Background_Tile_Count_Y))) break;
			
			if(!skip_targets) {
				if(!eagle.destroyed && eagle_rect.point_inside(origin)) {
					return {Raycast_Outcome::Type::Eagle,align_to_tile_grid(origin,dir)};
				}
				if(!first_player.tank.destroyed && first_player_rect.point_inside(origin)) {
					return {Raycast_Outcome::Type::Player1,align_to_tile_grid(origin,dir)};
				}
				if(scene == Scene::Game_2player) {
					if(!second_player.tank.destroyed && second_player_rect.point_inside(origin)) {
						return {Raycast_Outcome::Type::Player2,align_to_tile_grid(origin,dir)};
					}
				}
			}
			if(!skip_tiles) {
				Ipoint coords = {std::int32_t(origin.x * 2.0f),std::int32_t(origin.y * 2.0f)};
				const Tile& tile = tiles[coords.y * (Background_Tile_Count_X * 2) + coords.x];
				if(tile.template_index == Invalid_Tile_Index) continue;

				const Tile_Template& temp = tile_templates[tile.template_index];
				if(!include_bulletpass_tiles) {
					if(temp.flag == Tile_Flag::Solid) {
						return {Raycast_Outcome::Type::Tile,align_to_tile_grid(origin,dir)};
					}
				}
				else {
					if(temp.flag == Tile_Flag::Solid || temp.flag == Tile_Flag::Bulletpass) {
						return {Raycast_Outcome::Type::Tile,align_to_tile_grid(origin,dir)};
					}
				}
			}
		}
		return {Raycast_Outcome::Type::None};
	}

	void Game::update_player(Player* player,float delta_time) {
		if(player->tank.destroyed && player->lifes > 0) {
			player->respawn_timer -= delta_time;
			if(player->respawn_timer <= 0.0f) {
				player->respawn_timer = 0.0f;
				player->tank.destroyed = false;
				player->tank.dir = Entity_Direction::Up;
				player->tank.position = eagle.position + ((player == &first_player) ? Vec2{-3.0f,0.0f} : Vec2{3.0f,0.0f});
				player->lifes -= 1;
				player->invulnerability_timer = 5.0f;
				add_spawn_effect(player->tank.position);
			}
			return;
		}

		player->invulnerability_timer -= delta_time;
		if(player->invulnerability_timer <= 0.0f) player->invulnerability_timer = 0.0f;

		player->tank.shoot_cooldown -= delta_time;
		if(player->tank.shoot_cooldown <= 0.0f) player->tank.shoot_cooldown = 0.0f;

		Vec2 forward = {};
		auto key_right = (player == &first_player) ? Keycode::D : Keycode::Right;
		auto key_down = (player == &first_player) ? Keycode::S : Keycode::Down;
		auto key_left = (player == &first_player) ? Keycode::A : Keycode::Left;
		auto key_up = (player == &first_player) ? Keycode::W : Keycode::Up;
		auto key_shoot = (player == &first_player) ? Keycode::Space : Keycode::Return;

		if(platform->is_key_down(key_right)) {
			player->tank.dir = Entity_Direction::Right;
			forward = {1.0f,0.0f};
		}
		if(platform->is_key_down(key_down)) {
			player->tank.dir = Entity_Direction::Down;
			forward = {0.0f,1.0f};
		}
		if(platform->is_key_down(key_left)) {
			player->tank.dir = Entity_Direction::Left;
			forward = {-1.0f,0.0f};
		}
		if(platform->is_key_down(key_up)) {
			player->tank.dir = Entity_Direction::Up;
			forward = {0.0f,-1.0f};
		}
		if(platform->was_key_pressed(key_shoot) && player->tank.shoot_cooldown <= 0.0f) {
			Bullet bullet{};
			bullet.fired_by_player = true;
			bullet.dir = player->tank.dir;
			bullet.position = player->tank.position + Tank_Bullet_Firing_Positions[std::size_t(bullet.dir)];
			bullets.push_back(bullet);
			player->tank.shoot_cooldown = Tank_Shoot_Cooldown;
		}
		
		player->tank.position.x += forward.x * Tank_Speed * delta_time;
		player->tank.position.y += forward.y * Tank_Speed * delta_time;

		//Collision detection code uses the fact that coords of an object can be used as indicies into the map array thus creating quite efficient way of check collsions with tiles.
		std::int32_t start_x = std::int32_t((player->tank.position.x - Tank_Size.x / 2.0f + Collision_Offset) * 2.0f);
		std::int32_t start_y = std::int32_t((player->tank.position.y - Tank_Size.y / 2.0f + Collision_Offset) * 2.0f);
		std::int32_t end_x = std::int32_t((player->tank.position.x + Tank_Size.x / 2.0f - Collision_Offset) * 2.0f);
		std::int32_t end_y = std::int32_t((player->tank.position.y + Tank_Size.y / 2.0f - Collision_Offset) * 2.0f);

		check_collision_with_tiles(&player->tank.position,Tank_Size,start_x,start_y,end_x,end_y,player->tank.dir);
	}

	void Game::update_enemies(float delta_time) {
		if(remaining_enemy_count_to_spawn == 0 && enemy_tanks.size() == 0) {
			game_win_timer -= delta_time;
			if(game_win_timer <= 0.0f) {
				game_win_timer = 0.0f;
				scene = ((scene == Scene::Game_1player) ? Scene::Outro_1player : Scene::Outro_2player);
				return;
			}
		}

		if(remaining_enemy_count_to_spawn > 0) {
			enemy_spawn_timer -= delta_time;
			if(enemy_spawn_timer <= 0.0f) {
				enemy_spawn_timer = Enemy_Spawn_Time;

				if(enemy_tanks.size() < max_enemy_count_on_screen) {
					Tank enemy = {};
					enemy.dir = Entity_Direction::Up;
					enemy.position = Enemy_Spawner_Locations[enamy_spawn_point_random_dist(random_engine)];
					enemy.ai_react_timer = 2.0f;
					enemy.ai_dir_change_timer = 0.5f;
					enemy.hop_count_until_shoot = Enemy_Hop_Count_To_Shoot;
					add_spawn_effect(enemy.position);
					enemy_tanks.push_back(enemy);
					remaining_enemy_count_to_spawn -= 1;
				}
			}
		}

		Rect eagle_rect = {eagle.position.x - Eagle_Size.x / 2.0f,eagle.position.y - Eagle_Size.y / 2.0f,Eagle_Size.x,Eagle_Size.y};
		for(auto& enemy : enemy_tanks) {
			//AI tanks don't attack players immediately after spawning.
			enemy.ai_react_timer -= delta_time;
			if(enemy.ai_react_timer <= 0.0f) {
				enemy.ai_react_timer = 0.0f;

				enemy.shoot_cooldown -= delta_time;
				if(enemy.shoot_cooldown <= 0.0f) {
					enemy.shoot_cooldown = 0.0f;

					auto firing_pos = enemy.position + Tank_Bullet_Firing_Positions[std::size_t(enemy.dir)];
					if(enemy.hop_count_until_shoot == 0) {
						Bullet bullet{};
						bullet.dir = enemy.dir;
						bullet.position = firing_pos;
						bullets.push_back(bullet);
						enemy.shoot_cooldown = Tank_Shoot_Cooldown;
						enemy.hop_count_until_shoot = Enemy_Hop_Count_To_Shoot;
					}
					else {
						auto raycast_against_targets = raycast(firing_pos,enemy.dir,false,false,false);
						if(raycast_against_targets.hit_target()) {
							Bullet bullet{};
							bullet.dir = enemy.dir;
							bullet.position = firing_pos;
							bullets.push_back(bullet);
							enemy.shoot_cooldown = Tank_Shoot_Cooldown;
						}
					}
				}
			}

			enemy.ai_dir_change_timer -= delta_time;
			if(enemy.ai_dir_change_timer <= 0.0f) {
				enemy.ai_dir_change_timer = 0.15f;

				const auto& triple = Entity_Direction_Triples[std::size_t(enemy.dir)];
				for(auto dir : {triple.dir0,triple.dir1,triple.dir_back}) {
					auto raycast_result = raycast(enemy.position + Tank_Bullet_Firing_Positions[std::size_t(dir)],dir,false,true,false);
					if(raycast_result.type == Raycast_Outcome::Type::Eagle) {
						if(chance_0_1_dist(random_engine) <= 0.4f) {
							enemy.dir = dir;
							if(enemy.hop_count_until_shoot > 0) enemy.hop_count_until_shoot -= 1;
							if(enemy.shoot_cooldown <= 0.0f && chance_0_1_dist(random_engine) <= 0.25f) {
								Bullet bullet{};
								bullet.dir = enemy.dir;
								bullet.position = enemy.position + Tank_Bullet_Firing_Positions[std::size_t(enemy.dir)];
								bullets.push_back(bullet);
								enemy.shoot_cooldown = Tank_Shoot_Cooldown;
							}
							break;
						}
					}
					else if(raycast_result.type == Raycast_Outcome::Type::Player1 || raycast_result.type == Raycast_Outcome::Type::Player2) {
						float chance = (dir == triple.dir_back) ? 0.15f : 0.4f;
						if(chance_0_1_dist(random_engine) <= chance) {
							enemy.dir = dir;
							if(enemy.hop_count_until_shoot > 0) enemy.hop_count_until_shoot -= 1;
							if(enemy.shoot_cooldown <= 0.0f && chance_0_1_dist(random_engine) <= 0.25f) {
								Bullet bullet{};
								bullet.dir = enemy.dir;
								bullet.position = enemy.position + Tank_Bullet_Firing_Positions[std::size_t(enemy.dir)];
								bullets.push_back(bullet);
								enemy.shoot_cooldown = Tank_Shoot_Cooldown;
							}
							break;
						}
					}
					if(dir != triple.dir_back) {
						raycast_result = raycast(enemy.position + Tank_Bullet_Firing_Positions[std::size_t(dir)],dir,true,false,true);
						if(raycast_result.type == Raycast_Outcome::Type::Tile) {
							if(chance_0_1_dist(random_engine) <= 0.1f) {
								enemy.dir = dir;
								if(enemy.hop_count_until_shoot > 0) enemy.hop_count_until_shoot -= 1;
								break;
							}
						}
					}
				}
			}

			enemy.position.x += core::entity_direction_to_vector(enemy.dir).x * Tank_Speed * delta_time;
			enemy.position.y += core::entity_direction_to_vector(enemy.dir).y * Tank_Speed * delta_time;

			std::int32_t start_x = std::int32_t((enemy.position.x - Tank_Size.x / 2.0f + Collision_Offset) * 2.0f);
			std::int32_t start_y = std::int32_t((enemy.position.y - Tank_Size.y / 2.0f + Collision_Offset) * 2.0f);
			std::int32_t end_x = std::int32_t((enemy.position.x + Tank_Size.x / 2.0f - Collision_Offset) * 2.0f);
			std::int32_t end_y = std::int32_t((enemy.position.y + Tank_Size.y / 2.0f - Collision_Offset) * 2.0f);

			auto collision_status = check_collision_with_tiles(&enemy.position,Tank_Size,start_x,start_y,end_x,end_y,enemy.dir);
			if(collision_status.has_value()) {
				Entity_Direction avaialble_dirs[3] = {};
				std::size_t avaialble_dir_count = 0;

				const auto& triple = Entity_Direction_Triples[std::size_t(enemy.dir)];
				for(auto dir : {triple.dir0,triple.dir1,triple.dir_back}) {
					auto raycast_result = raycast(enemy.position + Tank_Bullet_Firing_Positions[std::size_t(dir)],dir,true,false,true);
					if(raycast_result.type == Raycast_Outcome::Type::Tile) avaialble_dirs[avaialble_dir_count++] = dir;
				}
				if(avaialble_dir_count > 0) {
					auto distribution = std::uniform_int_distribution<std::uint32_t>(0,avaialble_dir_count - 1);
					enemy.dir = avaialble_dirs[distribution(random_engine)];
					if(enemy.hop_count_until_shoot > 0) enemy.hop_count_until_shoot -= 1;
				}
			}
		}
		std::erase_if(enemy_tanks,[](const Tank& tank) { return tank.destroyed; });
	}

	void Game::update_bullets(float delta_time) {
		Rect screen_rect = {0.0f,0.0f,float(Background_Tile_Count_X),float(Background_Tile_Count_Y)};
		Urect tile_screen_rect = {0.0f,0.0f,Background_Tile_Count_X * 2,Background_Tile_Count_Y * 2};

		Rect eagle_rect = {eagle.position.x - Eagle_Size.x / 2.0f,eagle.position.y - Eagle_Size.y / 2.0f,Eagle_Size.x,Eagle_Size.y};
		for(auto& bullet : bullets) {
			bullet.position += core::entity_direction_to_vector(bullet.dir) * Bullet_Speed * delta_time;
			const auto& relative_bounding_box = Bullet_Bounding_Boxes[std::size_t(bullet.dir)];

			Rect bullet_rect = {
				bullet.position.x - Bullet_Size.x / 2.0f + relative_bounding_box.x,
				bullet.position.y - Bullet_Size.y / 2.0f + relative_bounding_box.y,
				relative_bounding_box.width,
				relative_bounding_box.height
			};
			if(!screen_rect.overlaps(bullet_rect)) {
				bullet.destroyed = true;
				continue;
			}

			if(!eagle.destroyed && bullet_rect.overlaps(eagle_rect)) {
				add_explosion(eagle.position,delta_time);
				eagle.destroyed = true;
				bullet.destroyed = true;
				game_lose_timer = 1.0f;
				continue;
			}

			for(auto player : {&first_player,&second_player}) {
				Rect player_rect = {player->tank.position.x - Tank_Size.x / 2.0f,player->tank.position.y - Tank_Size.y / 2.0f,1.0f,1.0f};
				if(!player->tank.destroyed && bullet_rect.overlaps(player_rect) && !bullet.fired_by_player) {
					add_explosion(player->tank.position,delta_time);
					bullet.destroyed = true;
					if(player->invulnerability_timer <= 0.0f) {
						player->tank.destroyed = true;
						player->respawn_timer = 2.0f;
					}
					break;
				}
			}
			if(bullet.destroyed) continue;

			for(auto& enemy_tank : enemy_tanks) {
				Rect enemy_rect = {enemy_tank.position.x - Tank_Size.x / 2.0f,enemy_tank.position.y - Tank_Size.y / 2.0f,1.0f,1.0f};
				if(bullet_rect.overlaps(enemy_rect) && bullet.fired_by_player) {
					add_explosion(enemy_tank.position,delta_time);
					enemy_tank.destroyed = true;
					bullet.destroyed = true;
					break;
				}
			}
			if(bullet.destroyed) continue;

			std::int32_t start_x = std::int32_t((bullet.position.x - Bullet_Size.x / 2.0f + relative_bounding_box.x) * 2.0f);
			std::int32_t start_y = std::int32_t((bullet.position.y - Bullet_Size.y / 2.0f + relative_bounding_box.y) * 2.0f);
			std::int32_t end_x = std::int32_t((bullet.position.x - Bullet_Size.x / 2.0f + relative_bounding_box.x + relative_bounding_box.width) * 2.0f);
			std::int32_t end_y = std::int32_t((bullet.position.y - Bullet_Size.y / 2.0f + relative_bounding_box.y + relative_bounding_box.height) * 2.0f);

			auto collision_status = check_collision_with_tiles(&bullet.position,Bullet_Size,start_x,start_y,end_x,end_y,bullet.dir,true);
			if(collision_status.has_value()) {
				auto coords = collision_status.value();
				auto& tile = tiles[coords.y * (Background_Tile_Count_X * 2) + coords.x];

				bullet.destroyed = true;
				if(tile.health == 0) tile.template_index = Invalid_Tile_Index;
				else tile.health -= 1;
				switch(bullet.dir) {
					case Entity_Direction::Right: { add_explosion(bullet.position + Vec2{0.5f,0.0f},delta_time); break; }
					case Entity_Direction::Down: { add_explosion(bullet.position + Vec2{0.0f,0.5f},delta_time); break; }
					case Entity_Direction::Left: { add_explosion(bullet.position - Vec2{0.5f,0.0f},delta_time); break; }
					case Entity_Direction::Up: { add_explosion(bullet.position - Vec2{0.0f,0.5f},delta_time); break; }
				}
			}
		}
		std::erase_if(bullets,[](const Bullet& bullet) { return bullet.destroyed; });
	}

	void Game::update(float delta_time) {
		if(platform->was_key_pressed(Keycode::F3)) show_fps = !show_fps;
		switch(scene) {
			case Scene::Main_Menu: {
				if(platform->is_key_down(Keycode::Escape))load_map("./assets/maps/map_menu.txt");
				if(platform->was_key_pressed(Keycode::Down) || platform->was_key_pressed(Keycode::S)) {
					current_main_menu_option += 1;
					if(current_main_menu_option >= Main_Menu_Options_Count) current_main_menu_option = 0;
				}
				if(platform->was_key_pressed(Keycode::Up) || platform->was_key_pressed(Keycode::W)) {
					if(current_main_menu_option == 0) current_main_menu_option = Main_Menu_Options_Count;
					current_main_menu_option -= 1;
				}
				if(platform->was_key_pressed(Keycode::Return)) {
					switch(current_main_menu_option) {
						case 0: {
							current_stage_index = 0;
							first_player.lifes = Player_Starting_Life_Count;
							scene = Scene::Intro_1player;
							break;
						}
						case 1: {
							current_stage_index = 0;
							first_player.lifes = Player_Starting_Life_Count;
							second_player.lifes = Player_Starting_Life_Count;
							scene = Scene::Intro_2player;
							break;
						}
						case 2: {
							first_player.lifes = Player_Starting_Life_Count;
							scene = Scene::Level_Selection;
							break;
						}
						case 3: {
							for(std::uint32_t y = 0;y < Background_Tile_Count_Y * 2;y += 1) {
								for(std::uint32_t x = 0;x < Background_Tile_Count_X * 2;x += 1) {
									tiles[y * (Background_Tile_Count_X * 2) + x] = Tile{Invalid_Tile_Index};
								}
							}
							eagle.destroyed = false;
							eagle.position = {Background_Tile_Count_X / 2.0f,Background_Tile_Count_Y - 2.0f};
							first_player.tank.destroyed = false;
							first_player.tank.dir = Entity_Direction::Up;
							first_player.tank.position = eagle.position - Vec2{3.0f,0.0f};
							second_player.tank.destroyed = false;
							second_player.tank.dir = Entity_Direction::Up;
							second_player.tank.position = eagle.position + Vec2{3.0f,0.0f};
							bullets.clear();
							enemy_tanks.clear();
							spawn_effects.clear();
							explosions.clear();
							scene = Scene::Construction;
							break;
						}
						case 4: {
							quit = true;
							break;
						}
					}
				}
				break;
			}
			case Scene::Intro_1player:
			case Scene::Intro_2player: {
				if(current_stage_index >= 5) {
					scene = Scene::Victory_Screen;
					break;
				}

				update_timer += delta_time;
				static constexpr float Intro_Screen_Duration = 2.5f;
				if(update_timer >= Intro_Screen_Duration) {
					update_timer = 0.0f;
					if (skip) {
						switch (current_stage_index) {
						case 0: load_map("./assets/maps/map1.txt"); break;
						case 1: load_map("./assets/maps/map2.txt"); break;
						case 2: load_map("./assets/maps/map3.txt"); break;
						case 3: load_map("./assets/maps/map4.txt"); break;
						case 4: load_map("./assets/maps/map5.txt"); break;
						}
					}
					eagle.destroyed = false;
					eagle.position = {Background_Tile_Count_X / 2.0f,Background_Tile_Count_Y - 2.0f};

					if(scene == Scene::Intro_1player) {
						max_enemy_count_on_screen = 3;
						remaining_enemy_count_to_spawn = 12;
					}
					else {
						max_enemy_count_on_screen = 4;
						remaining_enemy_count_to_spawn = 16;
					}
					enemy_spawn_timer = Enemy_Spawn_Time;
					game_win_timer = 1.0f;
					game_lose_timer = 1.0f;

					bullets.clear();
					enemy_tanks.clear();
					spawn_effects.clear();
					explosions.clear();

					first_player.tank.destroyed = false;
					first_player.tank.dir = Entity_Direction::Up;
					first_player.tank.position = eagle.position - Vec2{3.0f,0.0f};
					first_player.invulnerability_timer = 0.0f;
					first_player.respawn_timer = 0.0f;
					if(scene == Scene::Intro_1player) {
						scene = Scene::Game_1player;
					}
					else {
						second_player.tank.destroyed = false;
						second_player.tank.dir = Entity_Direction::Up;
						second_player.tank.position = eagle.position + Vec2{3.0f,0.0f};
						second_player.invulnerability_timer = 0.0f;
						second_player.respawn_timer = 0.0f;
						scene = Scene::Game_2player;
					}
				}
				break;
			}
			case Scene::Game_1player:
			case Scene::Game_2player: {
				if(platform->was_key_pressed(Keycode::Escape)) {
					load_map("./assets/maps/map_menu.txt");
					scene = Scene::Main_Menu;
					break;
				}

				update_player(&first_player,delta_time);
				if(scene == Scene::Game_2player) update_player(&second_player,delta_time);

				update_bullets(delta_time);
				update_enemies(delta_time);

				for(auto& effect : spawn_effects) {
					if(effect.current_frame >= Spawn_Effect_Layer_Count) continue;
					effect.timer -= delta_time;
					if(effect.timer <= 0.0f) {
						effect.timer = Spawn_Effect_Frame_Duration;
						effect.current_frame += 1;
					}
				}
				std::erase_if(spawn_effects,[](const Spawn_Effect& effect) { return effect.current_frame >= Spawn_Effect_Layer_Count; });

				bool first_player_lost = first_player.tank.destroyed && first_player.lifes == 0;
				bool second_player_lost = second_player.tank.destroyed && second_player.lifes == 0;
				bool lose_cond = (scene == Scene::Game_1player) ? (first_player_lost) : (first_player_lost && second_player_lost);
				if(eagle.destroyed || lose_cond) {
					game_lose_timer -= delta_time;
					if(game_lose_timer <= 0.0f) {
						game_lose_timer = 0.0f;
						scene = ((scene == Scene::Game_1player) ? Scene::Game_Over_1player : Scene::Game_Over_2player);
					}
				}
				break;
			}
			case Scene::Outro_1player:
			case Scene::Outro_2player: {
				skip = true;
				if(platform->was_key_pressed(Keycode::Return)) {
					current_stage_index += 1;
					scene = Scene::Intro_2player;
				}
				break;
			}


			case Scene::Level_Selection: {
				update_timer += delta_time;
				static constexpr float Intro_Screen_Duration = 0.0f;
				if (update_timer >= Intro_Screen_Duration) {
					update_timer = 0.0f;

					
						static constexpr float Intro_Screen_Duration = 30.0f;
						if (platform->was_key_pressed(Keycode::Up) || platform->was_key_pressed(Keycode::Left) || platform->was_key_pressed(Keycode::W)) {
							if (current_map_option == 0) current_map_option = Map_Options_Count;
							current_map_option -= 1;
							switch (current_map_option) {
							case 0: load_map("./assets/maps/map1.txt"); break;
							case 1: load_map("./assets/maps/map2.txt"); break;
							case 2: load_map("./assets/maps/map3.txt"); break;
							case 3: load_map("./assets/maps/map4.txt"); break;
							case 4: load_map("./assets/maps/map5.txt"); break;
							case 5: load_map("./assets/maps/map_menu.txt"); break;
							default: printf("Something is very worng");
							}
							update_timer = 0;
						}
						if (platform->was_key_pressed(Keycode::Down) || platform->was_key_pressed(Keycode::Right) || platform->was_key_pressed(Keycode::S)) {
							current_map_option += 1;
							if (current_map_option > Map_Options_Count) current_map_option = 0;
							switch (current_map_option) {
							case 0: load_map("./assets/maps/map1.txt"); break;
							case 1: load_map("./assets/maps/map2.txt"); break;
							case 2: load_map("./assets/maps/map3.txt"); break;
							case 3: load_map("./assets/maps/map4.txt"); break;
							case 4: load_map("./assets/maps/map5.txt"); break;
							case 5: load_map("./assets/maps/map_menu.txt"); break;
							default: printf("Something is very worng");
							}
							update_timer = 0;
						}
					if (update_timer >= Intro_Screen_Duration || platform->was_key_pressed(Keycode::Return)) {
						printf("ładowanie mapy %d\n", current_map_option);
						if (current_map_option == 5|| current_map_option == -1) {
							
							load_map_from_drive();
						}
						first_player.lifes = Player_Starting_Life_Count;
						skip = false;
							scene = Scene::Intro_1player;
						
					}
				}
				break;
			}
			case Scene::Level_Selected: {	
				if (platform->was_key_pressed(Keycode::Escape)) {
					load_map("./assets/maps/map_menu.txt");
					scene = Scene::Main_Menu;
					break;
				}

				break;
			};
			case Scene::Construction: {
				auto mouse_pos = platform->mouse_position();
				auto dims = renderer->render_client_rect_dimensions();
				float tile_width = float(dims.width) / float(Background_Tile_Count_X * 2);
				float tile_height = float(dims.height) / float(Background_Tile_Count_Y * 2);

				if(!construction_choosing_tile) {
					if(dims.point_inside(mouse_pos)) {
						construction_marker_pos.x = std::uint32_t(float(mouse_pos.x - dims.x) / tile_width);
						construction_marker_pos.y = std::uint32_t(float(mouse_pos.y - dims.y) / tile_height);

						if(platform->was_key_pressed(Keycode::Mouse_Left)) {
							if(construction_current_tile_template_index != Invalid_Tile_Index) {
								Tile& tile = tiles[construction_marker_pos.y * (Background_Tile_Count_X * 2) + construction_marker_pos.x];
								tile.template_index = construction_current_tile_template_index;
								tile.health = tile_templates[tile.template_index].health;
							}
						}
						if(platform->was_key_pressed(Keycode::Mouse_Right)) {
							Tile& tile = tiles[construction_marker_pos.y * (Background_Tile_Count_X * 2) + construction_marker_pos.x];
							tile.template_index = Invalid_Tile_Index;
						}
						if(platform->was_key_pressed(Keycode::Mouse_Middle)) {
							Tile& tile = tiles[construction_marker_pos.y * (Background_Tile_Count_X * 2) + construction_marker_pos.x];
							construction_current_tile_template_index = tile.template_index;
						}
						if(platform->was_key_pressed(Keycode::E)) construction_choosing_tile = true;
						if(platform->was_key_pressed(Keycode::Escape)) {
							load_map("./assets/maps/map_menu.txt");
							scene = Scene::Main_Menu;
						}
						if(platform->was_key_pressed(Keycode::S)) {
							try {
								//save_map("./assets/maps/map5.txt");
								save_map_on_drive();
							}
							catch(const File_Open_Exception& except) {
								char buffer[1024] = {};
								std::snprintf(buffer,sizeof(buffer) - 1,"Couldn't save file \"%s\".",except.file_path());
								platform->error_message_box(buffer);
							}
							catch(...) { throw; }
						}
						if(platform->was_key_pressed(Keycode::L)) {
							

							try {
								load_map_from_drive();
								
							}
							catch(const File_Open_Exception& except) {
								char buffer[1024] = {};
								std::snprintf(buffer,sizeof(buffer) - 1,"Couldn't load file \"%s\".",except.file_path());
								platform->error_message_box(buffer);
							}
							
							catch(...) { throw; }
							
						}
						if(platform->was_key_pressed(Keycode::B)) {
							tiles[0 * (Background_Tile_Count_X * 2) + 0] = Tile{11,std::uint32_t(-1)};
							tiles[(Background_Tile_Count_Y * 2 - 1) * (Background_Tile_Count_X * 2) + 0] = Tile{14,std::uint32_t(-1)};
							tiles[0 * (Background_Tile_Count_X * 2) + (Background_Tile_Count_X * 2 - 1)] = Tile{12,std::uint32_t(-1)};
							tiles[(Background_Tile_Count_Y * 2 - 1) * (Background_Tile_Count_X * 2) + (Background_Tile_Count_X * 2 - 1)] = Tile{13,std::uint32_t(-1)};

							for(std::uint32_t x = 1;x < Background_Tile_Count_X * 2 - 1;x += 1) {
								tiles[0 * (Background_Tile_Count_X * 2) + x] = Tile{8,std::uint32_t(-1)};
								tiles[(Background_Tile_Count_Y * 2 - 1) * (Background_Tile_Count_X * 2) + x] = Tile{10,std::uint32_t(-1)};
							}
							for(std::uint32_t y = 1;y < Background_Tile_Count_Y * 2 - 1;y += 1) {
								tiles[y * (Background_Tile_Count_X * 2) + 0] = Tile{7,std::uint32_t(-1)};
								tiles[y * (Background_Tile_Count_X * 2) + (Background_Tile_Count_X * 2 - 1)] = Tile{9,std::uint32_t(-1)};
							}
						}
					}
				}
				else {
					tile_width = float(dims.width) / float(Background_Tile_Count_X);
					tile_height = float(dims.height) / float(Background_Tile_Count_Y);

					Vec2 offset = {};
					for(std::size_t i = 0;i < tile_templates.size();i += 1) {
						const Tile_Template& elem = tile_templates[i];

						Urect rect = {
							std::uint32_t((1.0f + offset.x) * tile_width + dims.x),
							std::uint32_t((1.0f + offset.y) * tile_height + dims.y),
							tile_width,
							tile_height
						};

						if(platform->was_key_pressed(Keycode::Mouse_Left) && rect.point_inside(mouse_pos)) {
							construction_current_tile_template_index = i;
							construction_choosing_tile = false;
						}
						offset.x += 1.0f;
						if(offset.x > float(Background_Tile_Count_X) - 3.0f) {
							offset.x = 0.0f;
							offset.y += 1.0f;
						}
					}
					if(platform->was_key_pressed(Keycode::Escape) || platform->was_key_pressed(Keycode::E)) construction_choosing_tile = false;
				}
				break;
			}
			case Scene::Game_Over_1player:
			case Scene::Game_Over_2player: {
				if(platform->was_key_pressed(Keycode::Return)) {
					if(scene == Scene::Game_Over_1player) {
						first_player.lifes = Player_Starting_Life_Count;
						scene = Scene::Intro_1player;
					}
					else {
						first_player.lifes = Player_Starting_Life_Count;
						second_player.lifes = Player_Starting_Life_Count;
						scene = Scene::Intro_2player;
					}
				}
				if(platform->was_key_pressed(Keycode::Escape)) {
					load_map("./assets/maps/map_menu.txt");
					scene = Scene::Main_Menu;
				}
				break;
			}
			case Scene::Victory_Screen: {
				if(platform->was_key_pressed(Keycode::Return)) {
					load_map("./assets/maps/map_menu.txt");
					scene = Scene::Main_Menu;
				}
				break;
			}
		}
	}

	void Game::render(float delta_time) {
		if(show_fps) {
			char buffer[64] = {};
			std::snprintf(buffer,sizeof(buffer) - 1,"FPS: %f",1.0f / delta_time);
			renderer->draw_text({0.125f,0.125f,1},{0.25f,0.25f},{1,1,1},buffer);
		}
		switch(scene) {
			case Scene::Main_Menu: {
				render_map();
				for(std::size_t i = 0;i < Main_Menu_Options_Count;i += 1) {
					auto rect = renderer->compute_text_dims({0,0,1},{0.25f,0.25f},Main_Menu_Options[i]);
					Vec3 color = (current_main_menu_option == i) ? Vec3{1,1,0} : Vec3{1,1,1};
					renderer->draw_text({Background_Tile_Count_X / 2.0f - rect.width / 2.0f,Main_Menu_First_Option_Y_Offset + float(i) * 0.5f,1},{0.25f,0.25f},color,Main_Menu_Options[i]);
				}
				break;
			}
			case Scene::Intro_1player:
			case Scene::Intro_2player: {
				char buffer[32] = {};
				int count = std::snprintf(buffer,sizeof(buffer) - 1,"Stage %zu",current_stage_index + 1);
				if(count < 0) throw Runtime_Exception("Couldn't create intro's text.");

				auto rect = renderer->compute_text_dims({},{0.5f,0.5f},buffer);
				renderer->draw_text({Background_Tile_Count_X / 2.0f - rect.width / 2.0f,3.0f},{0.5f,0.5f},{1,1,1},buffer);

				count = std::snprintf(buffer,sizeof(buffer) - 1,"x%" PRIu32,first_player.lifes);
				if(count < 0) throw Runtime_Exception("Couldn't create intro's text.");
				rect = renderer->compute_text_dims({},{0.5f,0.5f},buffer);
				renderer->draw_text({Background_Tile_Count_X / 2.0f,5.0f},{0.5f,0.5f},{1,1,1},buffer);
				renderer->draw_sprite({Background_Tile_Count_X / 2.0f - 0.75f,5.0f},{1.0f,1.0f},0.0f,entity_sprites,Player_Tank_Sprite_Layer_Index);

				if(scene == Scene::Intro_2player) {
					count = std::snprintf(buffer,sizeof(buffer) - 1,"x%" PRIu32,second_player.lifes);
					if(count < 0) throw Runtime_Exception("Couldn't create intro's text.");
					rect = renderer->compute_text_dims({},{0.5f,0.5f},buffer);
					renderer->draw_text({Background_Tile_Count_X / 2.0f,6.0f},{0.5f,0.5f},{1,1,1},buffer);
					renderer->draw_sprite({Background_Tile_Count_X / 2.0f - 0.75f,6.0f},{1.0f,1.0f},0.0f,entity_sprites,Second_Player_Tank_Sprite_Layer_Index);
				}
				break;
			}
			case Scene::Game_1player:
			case Scene::Game_2player: {
				render_map();
				for(const auto& bullet : bullets) {
					renderer->draw_sprite({bullet.position.x,bullet.position.y},{1.0f,1.0f},core::entity_direction_to_rotation(bullet.dir),entity_sprites,Bullet_Sprite_Layer_Index);
				}
				for(auto& explosion : explosions) {
					if(explosion.sprite_index != -1) renderer->draw_sprite(explosion.position,explosion.size,0,explosion_sprite,explosion.sprite_index);
					explosion.sprite_index = 8 * explosion.texture_serie + (7 - (int)(explosion.timer * 8));
					explosion.timer -= delta_time;
					if(explosion.timer < 0) {
						explosion.destroyed = 1;
					}
				}
				std::erase_if(explosions,[](const Explosion& explosion) { return explosion.destroyed; });

				if(!eagle.destroyed) renderer->draw_sprite({eagle.position.x,eagle.position.y,0.5f},Eagle_Size,0.0f,entity_sprites,Eagle_Sprite_Layer_Index);

				if(!first_player.tank.destroyed) {
					auto rotation = core::entity_direction_to_rotation(first_player.tank.dir);
					bool is_protected = first_player.invulnerability_timer > 0.0f;
					renderer->draw_sprite({first_player.tank.position.x,first_player.tank.position.y,0.5f},Tank_Size,rotation,{1,1,1,1},is_protected,entity_sprites,Player_Tank_Sprite_Layer_Index);
					if(is_protected) renderer->draw_sprite({first_player.tank.position.x,first_player.tank.position.y,0.5f},Tank_Size,rotation,construction_place_marker);
				}
				if(scene == Scene::Game_2player && !second_player.tank.destroyed) {
					auto rotation = core::entity_direction_to_rotation(second_player.tank.dir);
					bool is_protected = second_player.invulnerability_timer > 0.0f;
					renderer->draw_sprite({second_player.tank.position.x,second_player.tank.position.y,0.5f},Tank_Size,rotation,{1,1,1,1},is_protected,entity_sprites,Second_Player_Tank_Sprite_Layer_Index);
					if(is_protected) renderer->draw_sprite({second_player.tank.position.x,second_player.tank.position.y,0.5f},Tank_Size,rotation,construction_place_marker);
				}

				for(const auto& enemy_tank : enemy_tanks) {
					renderer->draw_sprite({enemy_tank.position.x,enemy_tank.position.y,0.5f},Tank_Size,core::entity_direction_to_rotation(enemy_tank.dir),entity_sprites,Enemy_Tank_Sprite_Layer_Index);
				}
				for(const auto& effect : spawn_effects) {
					renderer->draw_sprite({effect.position.x,effect.position.y,0.9f},{1,1},0,spawn_effect_sprite_atlas,effect.current_frame);
				}

				renderer->draw_sprite({0.25f,Background_Tile_Count_Y - 0.25f,1.0f},{0.5f,0.5f},0,entity_sprites,Player_Tank_Sprite_Layer_Index);
				char text_buffer[32] = {};
				int count = std::snprintf(text_buffer,sizeof(text_buffer) - 1,"x%" PRIu32,first_player.lifes);
				if(count > 0) renderer->draw_text({0.75f,Background_Tile_Count_Y - 0.25f,1.0f},{0.5f,0.5f},{1,1,1},text_buffer);

				if(scene == Scene::Game_2player) {
					renderer->draw_sprite({4.25f,Background_Tile_Count_Y - 0.25f,1.0f},{0.5f,0.5f},0,entity_sprites,Second_Player_Tank_Sprite_Layer_Index);
					text_buffer[0] = '\0';
					int count = std::snprintf(text_buffer,sizeof(text_buffer) - 1,"x%" PRIu32,second_player.lifes);
					if(count > 0) renderer->draw_text({4.75f,Background_Tile_Count_Y - 0.25f,1.0f},{0.5f,0.5f},{1,1,1},text_buffer);
				}

				renderer->draw_sprite({8.25f,Background_Tile_Count_Y - 0.25f,1.0f},{0.5f,0.5f},0,entity_sprites,Enemy_Tank_Sprite_Layer_Index);
				count = std::snprintf(text_buffer,sizeof(text_buffer) - 1,"x%" PRIu32,remaining_enemy_count_to_spawn);
				if(count > 0) renderer->draw_text({8.75f,Background_Tile_Count_Y - 0.25f,1.0f},{0.5f,0.5f},{1,1,1},text_buffer);
				break;
			}
			case Scene::Outro_1player:
			case Scene::Outro_2player: {
				auto rect = renderer->compute_text_dims({},{0.5f,0.5f},"Stage cleared!");
				renderer->draw_text({Background_Tile_Count_X / 2.0f - rect.width / 2.0f,3.0f},{0.5f,0.5f},{1,1,1},"Stage cleared!");

				rect = renderer->compute_text_dims({},{0.5f,0.5f},"Press 'Enter' to advance to the next stage.");
				renderer->draw_text({Background_Tile_Count_X / 2.0f - rect.width / 2.0f,7.0f},{0.5f,0.5f},{1,1,1},"Press 'Enter' to advance to the next stage.");
				break;
			}
			case Scene::Level_Selection: {
				render_map();
				for (std::size_t i = 0; i < Map_Options_Count; i += 1) {
					auto rect = renderer->compute_text_dims({ 0,0,1 }, { 0.25f,0.25f }, Map_Options[i]);
					Vec3 color = (current_map_option == i) ? Vec3{1, 1, 0} : Vec3{ 1,1,1 };
					renderer->draw_text({ Background_Tile_Count_X / 2.0f - rect.width / 2.0f,Map_First_Option_Y_Offset + float(i) * 0.5f,1 }, { 0.25f,0.25f }, color, Map_Options[i]);
				}
			}
			case Scene::Construction: {
				if(!construction_choosing_tile) {
					render_map();
					for(const auto& pos : Enemy_Spawner_Locations) {
						renderer->draw_sprite({pos.x,pos.y,0.9f},{1.0f,1.0f},0.0f,spawn_effect_sprite_atlas,3);
					}

					renderer->draw_sprite({eagle.position.x,eagle.position.y,0.5f},Eagle_Size,0.0f,entity_sprites,Eagle_Sprite_Layer_Index);
					renderer->draw_sprite({first_player.tank.position.x,first_player.tank.position.y,0.5f},Tank_Size,0.0f,entity_sprites,Player_Tank_Sprite_Layer_Index);
					renderer->draw_sprite({second_player.tank.position.x,second_player.tank.position.y,0.5f},Tank_Size,0.0f,entity_sprites,Second_Player_Tank_Sprite_Layer_Index);
					renderer->draw_sprite({float(construction_marker_pos.x) * 0.5f + 0.25f,float(construction_marker_pos.y) * 0.5f + 0.25f,1.0f},{0.5f,0.5f},0,construction_place_marker);
				}
				else {
					Vec2 offset = {};
					Vec2 marker_offset = {};
					for(std::size_t i = 0;i < tile_templates.size();i += 1) {
						const Tile_Template& elem = tile_templates[i];
						renderer->draw_sprite({1.5f + offset.x,1.5f + offset.y},{1.0f,1.0f},elem.rotation,tiles_texture,elem.tile_layer_index);
						if(construction_current_tile_template_index == i) marker_offset = offset;

						offset.x += 1.0f;
						if(offset.x > float(Background_Tile_Count_X) - 3.0f) {
							offset.x = 0.0f;
							offset.y += 1.0f;
						}
					}
					renderer->draw_sprite({1.5f + marker_offset.x,1.5f + marker_offset.y},{1.0f,1.0f},0,construction_place_marker);
					renderer->draw_text({0.125f,0.125f,1},{0.25f,0.25f},{1,1,1},"Choose a tile from the list below (or press 'Escape' or 'E' to cancel).");
				}
				break;
			}
			case Scene::Game_Over_1player:
			case Scene::Game_Over_2player: {
				auto rect = renderer->compute_text_dims({},{0.5f,0.5f},"You lost!");
				renderer->draw_text({Background_Tile_Count_X / 2.0f - rect.width / 2.0f,3.0f},{0.5f,0.5f},{1,1,1},"You lost!");

				if(eagle.destroyed) {
					rect = renderer->compute_text_dims({},{0.5f,0.5f},"The eagle has been destroyed.");
					renderer->draw_text({Background_Tile_Count_X / 2.0f - rect.width / 2.0f,5.0f},{0.5f,0.5f},{1,1,1},"The eagle has been destroyed.");
				}
				else {
					rect = renderer->compute_text_dims({},{0.5f,0.5f},"Your tank has been destroyed.");
					renderer->draw_text({Background_Tile_Count_X / 2.0f - rect.width / 2.0f,5.0f},{0.5f,0.5f},{1,1,1},"Your tank has been destroyed.");
				}

				rect = renderer->compute_text_dims({},{0.25f,0.25f},"Press 'Enter' to try again or 'Escape' to leave to the main menu.");
				renderer->draw_text({Background_Tile_Count_X / 2.0f - rect.width / 2.0f,7.0f},{0.25f,0.25f},{1,1,1},"Press 'Enter' to try again or 'Escape' to leave to the main menu.");
				break;
			}
			case Scene::Victory_Screen: {
				auto rect = renderer->compute_text_dims({},{0.5f,0.5f},"You have beaten the game!!!");
				renderer->draw_text({Background_Tile_Count_X / 2.0f - rect.width / 2.0f,3.0f},{0.5f,0.5f},{1,1,1},"You have beaten the game!!!");

				rect = renderer->compute_text_dims({},{0.5f,0.5f},"Thank you for playing!");
				renderer->draw_text({Background_Tile_Count_X / 2.0f - rect.width / 2.0f,5.0f},{0.5f,0.5f},{1,1,1},"Thank you for playing!");

				rect = renderer->compute_text_dims({},{0.5f,0.5f},"Press 'Enter' to return to the main menu.");
				renderer->draw_text({Background_Tile_Count_X / 2.0f - rect.width / 2.0f,7.0f},{0.5f,0.5f},{1,1,1},"Press 'Enter' to return to the main menu.");
				break;
			}
		}
	}

	bool Game::quit_requested() const noexcept {
		return quit;
	}

	void Game::add_spawn_effect(Vec2 position) {
		Spawn_Effect effect{};
		effect.position = position;
		effect.timer = Spawn_Effect_Frame_Duration;
		spawn_effects.push_back(effect);
	}

	void Game::add_explosion(Vec2 position,float delta_time) {
		explosions.push_back({{position.x,position.y,0.6f},{1.0f,1.0f},1.0f,0,0,((int)(delta_time * 16384) % 8)});
	}

	void Game::save_map(const char* file_path) {
		static char name_buffer[256] = {};
		std::strcpy(name_buffer,file_path);

		std::ofstream file{name_buffer,std::ios::binary};
		if(!file.is_open()) throw File_Open_Exception(name_buffer);

		for(std::uint32_t y = 0;y < Background_Tile_Count_Y * 2;y += 1) {
			for(std::uint32_t x = 0;x < Background_Tile_Count_X * 2;x += 1) {
				const auto& tile = tiles[y * (Background_Tile_Count_X * 2) + x];
				file << tile.template_index << " " << tile.health;
				if((x + 1) < (Background_Tile_Count_X * 2)) file << " ";
			}
			file << "\n";
		}
		return;
	}

	void Game::load_map(const char* file_path) {
		static char name_buffer[256] = {};
		std::strcpy(name_buffer,file_path);

		std::ifstream file{name_buffer,std::ios::binary};
		if(!file.is_open()) throw File_Open_Exception(name_buffer);

		for(std::uint32_t y = 0;y < Background_Tile_Count_Y * 2;y += 1) {
			for(std::uint32_t x = 0;x < Background_Tile_Count_X * 2;x += 1) {
				auto& tile = tiles[y * (Background_Tile_Count_X * 2) + x];
				file >> tile.template_index >> tile.health;
			}
		}
	}

	void Game::render_map() {
		for(std::uint32_t y = 0;y < Background_Tile_Count_Y * 2;y += 1) {
			for(std::uint32_t x = 0;x < Background_Tile_Count_X * 2;x += 1) {
				const Tile& tile = tiles[y * (Background_Tile_Count_X * 2) + x];
				if(tile.template_index == Invalid_Tile_Index) continue;
				const auto& tile_template = tile_templates[tile.template_index];
				renderer->draw_sprite({0.25f + x * 0.5f,0.25f + y * 0.5f,core::tile_flag_to_z_order(tile_template.flag)},{0.5f,0.5f},tile_template.rotation,tiles_texture,tile_template.tile_layer_index);
			}
		}
	}
	void Game::load_map_from_drive() {
		OPENFILENAMEA ofn;
		char fileName[MAX_PATH] = {};
		char filePath[MAX_PATH] = {};
		ZeroMemory(&ofn, sizeof(ofn));

		ofn.lStructSize = sizeof(ofn);
		ofn.lpstrFile = filePath;
		ofn.lpstrFile[0] = '\0';
		ofn.nMaxFile = sizeof(filePath) - 1;
		ofn.lpstrFilter = "All Files (*.*)\0*.*\0";
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetOpenFileNameA(&ofn))
		{
			load_map(filePath);
		}
		return;
		////wprintf(L"No file selected.\n");
	}	
	void Game::save_map_on_drive() {
		OPENFILENAMEA ofn;       // Structure for the save file dialog
		TCHAR szFile[MAX_PATH]; // Buffer to store the selected file name

		// Initialize the OPENFILENAME structure
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = NULL;
		ofn.lpstrFile = szFile;
		ofn.lpstrFile[0] = '\0';
		ofn.nMaxFile = sizeof(szFile)-1;
		ofn.lpstrFilter = "Map (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

		// Display the Save File dialog
		if (GetSaveFileNameA(&ofn) == TRUE)
		{
			save_map(szFile);	
		}
		return;
	}
}
