#include <cstdio>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <cinttypes>
#include "game.hpp"
#include "exceptions.hpp"


namespace core {
	static constexpr float Main_Menu_First_Option_Y_Offset = 6.0f;
	static constexpr const char* Main_Menu_Options[] = {"1 player","2 players","Load Level","Construction","Quit"};
	static constexpr std::size_t Main_Menu_Options_Count = sizeof(Main_Menu_Options) / sizeof(*Main_Menu_Options);
	static constexpr float Tank_Speed = 4.0f;
	static constexpr float Bullet_Speed = 12.0f;
	static constexpr std::uint32_t Player_Tank_Sprite_Layer_Index = 0;
	static constexpr std::uint32_t Enemy_Tank_Sprite_Layer_Index = 6;
	static constexpr std::uint32_t Bullet_Sprite_Layer_Index = 10;
	static constexpr std::uint32_t Eagle_Sprite_Layer_Index = 11;
	static constexpr Vec2 Eagle_Size = {1.0f,1.0f};
	static constexpr Vec2 Tank_Size = {1.0f,1.0f};
	static constexpr Vec2 Bullet_Size = {1.0f,1.0f};

	//Bounding boxes for each 'Entity_Direction' value in order.
	static constexpr Rect Bullet_Bounding_Boxes[] = {{0.28125f,0.4375f,0.40625f,0.1875f},{0.4375f,0.28125f,0.1875f,0.40625f},{0.3125f,0.40625f,0.40625f,0.1875f},{0.4375f,0.3125f,0.1875f,0.40625f}};

	static constexpr Vec2 Player_Tank_Bullet_Firing_Positions[] = {{0.7f,0.0f},{0.0f,0.7f},{-0.7f,0.0f},{0.0f,-0.7f}};

	Game::Game(Renderer* _renderer,Platform* _platform) : renderer(_renderer),platform(_platform),scene(Scene::Main_Menu),
		current_main_menu_option(),update_timer(),construction_marker_pos(),construction_choosing_tile(),construction_tile_choice_marker_pos(),
		construction_current_tile_template_index(),tile_templates(),show_fps(),quit(),tiles(),player_lifes(),player_tank(),eagle(),game_lose_timer() {
		tiles_texture = renderer->sprite_atlas("./assets/tiles_16x16.bmp",16);
		construction_place_marker = renderer->sprite("./assets/marker.bmp");
		entity_sprites = renderer->sprite_atlas("./assets/entities_32x32.bmp",32);
		explosion_sprite = renderer->sprite_atlas("./assets/explosions_16x16.bmp", 16);

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
	}

	void Game::upadate_player(float delta_time) {
		Vec2 forward = {};
		if(platform->is_key_down(Keycode::Right) || platform->is_key_down(Keycode::D)) {
			player_tank.dir = Entity_Direction::Right;
			forward = {1.0f,0.0f};
		}
		if(platform->is_key_down(Keycode::Down) || platform->is_key_down(Keycode::S)) {
			player_tank.dir = Entity_Direction::Down;
			forward = {0.0f,1.0f};
		}
		if(platform->is_key_down(Keycode::Left) || platform->is_key_down(Keycode::A)) {
			player_tank.dir = Entity_Direction::Left;
			forward = {-1.0f,0.0f};
		}
		if(platform->is_key_down(Keycode::Up) || platform->is_key_down(Keycode::W)) {
			player_tank.dir = Entity_Direction::Up;
			forward = {0.0f,-1.0f};
		}
		if(platform->was_key_pressed(Keycode::Space)) {
			Bullet bullet{};
			bullet.fired_by_player = true;
			bullet.dir = player_tank.dir;
			bullet.position = player_tank.position + Player_Tank_Bullet_Firing_Positions[std::size_t(bullet.dir)];
			bullets.push_back(bullet);
		}

		player_tank.position.x += forward.x * Tank_Speed * delta_time;
		player_tank.position.y += forward.y * Tank_Speed * delta_time;
		Rect player_tank_rect = {player_tank.position.x - Tank_Size.x / 2.0f,player_tank.position.y - Tank_Size.y / 2.0f,Tank_Size.x,Tank_Size.y};

		Rect eagle_rect = {eagle.position.x - Eagle_Size.x / 2.0f,eagle.position.y - Eagle_Size.y / 2.0f,Eagle_Size.x,Eagle_Size.y};
		if(!eagle.destroyed && player_tank_rect.overlaps(eagle_rect)) {
			switch(player_tank.dir) {
				case Entity_Direction::Right: { player_tank.position.x = eagle.position.x - 1.0f; break; }
				case Entity_Direction::Down: { player_tank.position.y = eagle.position.y - 1.0f; break; }
				case Entity_Direction::Left: { player_tank.position.x = eagle.position.x + 1.0f; break; }
				case Entity_Direction::Up: { player_tank.position.y = eagle.position.y + 1.0f; break; }
			}
		}

		const auto& relative_bounding_box = Rect{0,0,1,1};
		Point upper_right = {
			std::uint32_t(((player_tank.position.x - Tank_Size.x / 2.0f) + relative_bounding_box.x + relative_bounding_box.width) * 2.0f),
			std::uint32_t(((player_tank.position.y - Tank_Size.y / 2.0f) + relative_bounding_box.y) * 2.0f)
		};
		Point lower_right = {
			std::uint32_t(((player_tank.position.x - Tank_Size.x / 2.0f) + relative_bounding_box.x + relative_bounding_box.width) * 2.0f),
			std::uint32_t(((player_tank.position.y - Tank_Size.y / 2.0f) + relative_bounding_box.y + relative_bounding_box.height) * 2.0f)
		};
		Point upper_left = {
			std::uint32_t(((player_tank.position.x - Tank_Size.x / 2.0f) + relative_bounding_box.x) * 2.0f),
			std::uint32_t(((player_tank.position.y - Tank_Size.y / 2.0f) + relative_bounding_box.y) * 2.0f)
		};
		Point lower_left = {
			std::uint32_t(((player_tank.position.x - Tank_Size.x / 2.0f) + relative_bounding_box.x) * 2.0f),
			std::uint32_t(((player_tank.position.y - Tank_Size.y / 2.0f) + relative_bounding_box.y + relative_bounding_box.height) * 2.0f)
		};

		struct Point_Pair { Point a;Point b; };
		Point_Pair pairs[] = {{upper_right,lower_right},{lower_right,lower_left},{upper_left,lower_left},{upper_left,upper_right}};
		const Point_Pair& pair = pairs[std::size_t(player_tank.dir)];

		Irect tile_screen_rect = {0.0f,0.0f,Background_Tile_Count_X * 2,Background_Tile_Count_Y * 2};
		bool is_a_point = tile_screen_rect.point_inside(pair.a);
		bool is_b_point = tile_screen_rect.point_inside(pair.b);
		if(is_a_point || is_b_point) {
			const auto& point = is_a_point ? pair.a : pair.b;
			const auto& tile = tiles[point.y * (Background_Tile_Count_X * 2) + point.x];
			if(tile.template_index != Invalid_Tile_Index) {
				const auto& tile_template = tile_templates[tile.template_index];
				if(tile_template.flag == Tile_Flag::Solid) {
					switch(player_tank.dir) {
						case Entity_Direction::Right: {
							player_tank.position.x = point.x / 2.0f - Tank_Size.x / 2.0f;
							break;
						}
						case Entity_Direction::Down: {
							player_tank.position.y = point.y / 2.0f - Tank_Size.y / 2.0f;
							break;
						}
					}
				}
			}
		}
	}

	void Game::update(float delta_time) {
		if(platform->was_key_pressed(Keycode::F3)) {
			show_fps = !show_fps;
		}

		switch(scene) {
			case Scene::Main_Menu: {
				if(platform->was_key_pressed(Keycode::Down)) {
					current_main_menu_option += 1;
					if(current_main_menu_option >= Main_Menu_Options_Count) current_main_menu_option = 0;
				}
				if(platform->was_key_pressed(Keycode::Up)) {
					if(current_main_menu_option == 0) current_main_menu_option = Main_Menu_Options_Count;
					current_main_menu_option -= 1;
				}
				if(platform->was_key_pressed(Keycode::Return)) {
					switch(current_main_menu_option) {
						case 0: {
							scene = Scene::Intro_1player;
							break;
						}
						case 1: {
							platform->error_message_box("Local multiplayer is not yet supported.");
							break;
						}
						case 2: {
							platform->error_message_box("Level selection is not yet supported.");
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
							player_tank.destroyed = false;
							player_tank.dir = Entity_Direction::Up;
							player_tank.position = eagle.position - Vec2{3.0f,0.0f};
							bullets.clear();
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
			case Scene::Intro_1player: {
				update_timer += delta_time;
				static constexpr float Intro_Screen_Duration = 0.0f;
				if(update_timer >= Intro_Screen_Duration) {
					update_timer = 0.0f;
					
					for(std::uint32_t y = 0;y < Background_Tile_Count_Y * 2;y += 1) {
						for(std::uint32_t x = 0;x < Background_Tile_Count_X * 2;x += 1) {
							tiles[y * (Background_Tile_Count_X * 2) + x] = Tile{Invalid_Tile_Index};
						}
					}
					tiles[0 * (Background_Tile_Count_X * 2) + 0] = Tile{11};
					tiles[(Background_Tile_Count_Y * 2 - 2) * (Background_Tile_Count_X * 2) + 0] = Tile{14};
					tiles[0 * (Background_Tile_Count_X * 2) + (Background_Tile_Count_X * 2 - 1)] = Tile{12};
					tiles[(Background_Tile_Count_Y * 2 - 2) * (Background_Tile_Count_X * 2) + (Background_Tile_Count_X * 2 - 1)] = Tile{13};

					for(std::uint32_t x = 1;x < Background_Tile_Count_X * 2 - 1;x += 1) {
						tiles[0 * (Background_Tile_Count_X * 2) + x] = Tile{8};
						tiles[(Background_Tile_Count_Y * 2 - 2) * (Background_Tile_Count_X * 2) + x] = Tile{10};
					}
					for(std::uint32_t y = 1;y < Background_Tile_Count_Y * 2 - 2;y += 1) {
						tiles[y * (Background_Tile_Count_X * 2) + 0] = Tile{7};
						tiles[y * (Background_Tile_Count_X * 2) + (Background_Tile_Count_X * 2 - 1)] = Tile{9};
					}

					tiles[6 * (Background_Tile_Count_X * 2) + 8] = Tile{0};

					eagle.destroyed = false;
					eagle.position = {Background_Tile_Count_X / 2.0f,Background_Tile_Count_Y - 2.0f};
					player_tank.destroyed = false;
					player_tank.dir = Entity_Direction::Up;
					player_tank.position = eagle.position - Vec2{3.0f,0.0f};
					player_lifes = 3;
					bullets.clear();

					scene = Scene::Game_1player;
				}
				break;
			}
			case Scene::Game_1player: {
				upadate_player(delta_time);

				Rect screen_rect = {0.0f,0.0f,float(Background_Tile_Count_X),float(Background_Tile_Count_Y)};
				Irect tile_screen_rect = {0.0f,0.0f,Background_Tile_Count_X * 2,Background_Tile_Count_Y * 2};
				for(auto& bullet : bullets) {
					bullet.position += core::entity_direction_to_vector(bullet.dir) * Bullet_Speed * delta_time;

					const auto& relative_bullet_rect = Bullet_Bounding_Boxes[std::size_t(bullet.dir)];
					Rect bullet_rect = {
						bullet.position.x - Bullet_Size.x / 2.0f + relative_bullet_rect.x,
						bullet.position.y - Bullet_Size.y / 2.0f + relative_bullet_rect.y,
						relative_bullet_rect.width,
						relative_bullet_rect.height
					};
					if(!screen_rect.overlaps(bullet_rect)) bullet.destroyed = true;

					Rect eagle_rect = {eagle.position.x - Eagle_Size.x / 2.0f,eagle.position.y - Eagle_Size.y / 2.0f,Eagle_Size.x,Eagle_Size.y};
					if(!eagle.destroyed && bullet_rect.overlaps(eagle_rect)) {
						eagle.destroyed = true;
						bullet.destroyed = true;
						static constexpr float Time_To_Lose = 1.0f;
						game_lose_timer = Time_To_Lose;
						continue;
					}

					const auto& relative_bounding_box = Bullet_Bounding_Boxes[std::size_t(bullet.dir)];
					Point upper_right = {
						std::uint32_t(((bullet.position.x - Bullet_Size.x / 2.0f) + relative_bounding_box.x + relative_bounding_box.width) * 2.0f),
						std::uint32_t(((bullet.position.y - Bullet_Size.y / 2.0f) + relative_bounding_box.y) * 2.0f)
					};
					Point lower_right = {
						std::uint32_t(((bullet.position.x - Bullet_Size.x / 2.0f) + relative_bounding_box.x + relative_bounding_box.width) * 2.0f),
						std::uint32_t(((bullet.position.y - Bullet_Size.y / 2.0f) + relative_bounding_box.y + relative_bounding_box.height) * 2.0f)
					};
					Point upper_left = {
						std::uint32_t(((bullet.position.x - Bullet_Size.x / 2.0f) + relative_bounding_box.x) * 2.0f),
						std::uint32_t(((bullet.position.y - Bullet_Size.y / 2.0f) + relative_bounding_box.y) * 2.0f)
					};
					Point lower_left = {
						std::uint32_t(((bullet.position.x - Bullet_Size.x / 2.0f) + relative_bounding_box.x) * 2.0f),
						std::uint32_t(((bullet.position.y - Bullet_Size.y / 2.0f) + relative_bounding_box.y + relative_bounding_box.height) * 2.0f)
					};

					struct Point_Pair { Point a;Point b; };
					Point_Pair pairs[] = {{upper_right,lower_right},{lower_right,lower_left},{upper_left,lower_left},{upper_left,upper_right}};
					const Point_Pair& pair = pairs[std::size_t(bullet.dir)];
					
					bool is_a_point = tile_screen_rect.point_inside_inclusive(pair.a);
					bool is_b_point = tile_screen_rect.point_inside_inclusive(pair.b);
					if(is_a_point || is_b_point) {
						const auto& point = is_a_point ? pair.a : pair.b;
						const auto& tile = tiles[point.y * (Background_Tile_Count_X * 2) + point.x];
						if(tile.template_index != Invalid_Tile_Index) {
							const auto& tile_template = tile_templates[tile.template_index];
							if (tile_template.flag == Tile_Flag::Solid) {
								explosions.push_back({ {bullet.position.x,bullet.position.y,0.3f},{1.0f,1.0f},1.0f,0,0,((int)(delta_time * 16384) % 8) });
								bullet.destroyed = true;
							}
						}
					}
				}
				std::erase_if(bullets,[](const Bullet& bullet) { return bullet.destroyed; });

				if(eagle.destroyed) {
					game_lose_timer -= delta_time;
					if(game_lose_timer <= 0.0f) {
						game_lose_timer = 0.0f;
						scene = Scene::Game_Over;
					}
				}
				break;
			}
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
						if(platform->was_key_pressed(Keycode::Escape)) scene = Scene::Main_Menu;
					}
				}
				else {
					tile_width = float(dims.width) / float(Background_Tile_Count_X);
					tile_height = float(dims.height) / float(Background_Tile_Count_Y);

					Vec2 offset = {};
					for(std::size_t i = 0;i < tile_templates.size();i += 1) {
						const Tile_Template& elem = tile_templates[i];

						Irect rect = {
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
			case Scene::Level_Selection: {
				break;
			}
			case Scene::Game_Over: {
				if(platform->was_key_pressed(Keycode::Return)) scene = Scene::Main_Menu;
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
				for(std::size_t i = 0;i < Main_Menu_Options_Count;i += 1) {
					auto rect = renderer->compute_text_dims({0,0,1},{0.25f,0.25f},Main_Menu_Options[i]);
					Vec3 color = (current_main_menu_option == i) ? Vec3{1,1,0} : Vec3{1,1,1};
					renderer->draw_text({Background_Tile_Count_X / 2.0f - rect.width / 2.0f,Main_Menu_First_Option_Y_Offset + float(i) * 0.5f,1},{0.25f,0.25f},color,Main_Menu_Options[i]);
				}
				break;
			}
			case Scene::Intro_1player: {
				break;
			}
			case Scene::Game_1player: {
				for(std::uint32_t y = 0;y < Background_Tile_Count_Y * 2;y += 1) {
					for(std::uint32_t x = 0;x < Background_Tile_Count_X * 2;x += 1) {
						const Tile& tile = tiles[y * (Background_Tile_Count_X * 2) + x];
						if(tile.template_index == Invalid_Tile_Index) continue;
						const auto& tile_template = tile_templates[tile.template_index];

						float z_order = 0.0f;
						switch(tile_template.flag) {
							case Tile_Flag::Above: z_order = 0.75f; break;
							case Tile_Flag::Bulletpass:
							case Tile_Flag::Below: z_order = 0.25f; break;
						}
						renderer->draw_sprite({0.25f + x * 0.5f,0.25f + y * 0.5f,z_order},{0.5f,0.5f},tile_template.rotation,tiles_texture,tile_template.tile_layer_index);
					}
				}

				for(const auto& bullet : bullets) {
					renderer->draw_sprite({bullet.position.x,bullet.position.y},{1.0f,1.0f},core::entity_direction_to_rotation(bullet.dir),entity_sprites,Bullet_Sprite_Layer_Index);
				}
//explosion
				for (auto& explosion : explosions) {
					renderer->draw_sprite(explosion.position, explosion.size, 0, explosion_sprite, explosion.sprite_index);
					explosion.sprite_index = 8*explosion.testure_serie+(7 - (int)(explosion.timer * 8));
					explosion.timer -= delta_time;
					printf("%f texture: %d random number %d\n",explosion.timer,explosion.sprite_index, (int)(delta_time*16384)%8);
					if (explosion.timer < 0) {
						explosion.destroyed = 1;
					}

				}

				std::erase_if(explosions, [](const Explosion& explosion) { return explosion.destroyed; });


				if(!eagle.destroyed) {
					renderer->draw_sprite({eagle.position.x,eagle.position.y,0.5f},Eagle_Size,0.0f,entity_sprites,Eagle_Sprite_Layer_Index);
				}
				if(!player_tank.destroyed) {
					renderer->draw_sprite({player_tank.position.x,player_tank.position.y,0.5f},Tank_Size,core::entity_direction_to_rotation(player_tank.dir),entity_sprites,Player_Tank_Sprite_Layer_Index);
				}

				renderer->draw_sprite({0.25f,Background_Tile_Count_Y - 0.25f,1.0f},{0.5f,0.5f},0,entity_sprites,Player_Tank_Sprite_Layer_Index);
				char lifes_buffer[32] = {};
				int count = std::snprintf(lifes_buffer,sizeof(lifes_buffer) - 1,"x%" PRIu32,player_lifes);
				if(count > 0) renderer->draw_text({0.75f,Background_Tile_Count_Y - 0.25f,1.0f},{0.5f,0.5f},{1,1,1},lifes_buffer);
				break;
			}
			case Scene::Construction: {
				if(!construction_choosing_tile) {
					for(std::uint32_t y = 0;y < Background_Tile_Count_Y * 2;y += 1) {
						for(std::uint32_t x = 0;x < Background_Tile_Count_X * 2;x += 1) {
							const Tile& tile = tiles[y * (Background_Tile_Count_X * 2) + x];
							if(tile.template_index == Invalid_Tile_Index) continue;
							const auto& tile_template = tile_templates[tile.template_index];

							float z_order = 0.0f;
							switch(tile_template.flag) {
								case Tile_Flag::Above: z_order = 0.75f; break;
								case Tile_Flag::Bulletpass:
								case Tile_Flag::Below: z_order = 0.25f; break;
							}
							renderer->draw_sprite({0.25f + x * 0.5f,0.25f + y * 0.5f,z_order},{0.5f,0.5f},tile_template.rotation,tiles_texture,tile_template.tile_layer_index);
						}
					}

					renderer->draw_sprite({eagle.position.x,eagle.position.y,0.5f},Eagle_Size,0.0f,entity_sprites,Eagle_Sprite_Layer_Index);
					renderer->draw_sprite({player_tank.position.x,player_tank.position.y,0.5f},Tank_Size,0.0f,entity_sprites,Player_Tank_Sprite_Layer_Index);
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
					renderer->draw_sprite({1.5f + marker_offset.x,1.5f + marker_offset.y},{1.0f,1.0f},0,construction_place_marker,0);
					renderer->draw_text({0.125f,0.125f,1},{0.25f,0.25f},{1,1,1},"Choose a tile from the list below (or press 'Escape' or 'E' to cancel).");
				}
				break;
			}
			case Scene::Level_Selection: {
				break;
			}
			case Scene::Game_Over: {
				auto rect = renderer->compute_text_dims({},{0.5f,0.5f},"You lost!");
				renderer->draw_text({Background_Tile_Count_X / 2.0f - rect.width / 2.0f,3.0f},{0.5f,0.5f},{1,1,1},"You lost!");

				rect = renderer->compute_text_dims({},{0.5f,0.5f},"The eagle has been destroyed.");
				renderer->draw_text({Background_Tile_Count_X / 2.0f - rect.width / 2.0f,5.0f},{0.5f,0.5f},{1,1,1},"The eagle has been destroyed.");

				rect = renderer->compute_text_dims({},{0.5f,0.5f},"Press 'Enter' to return to the main menu.");
				renderer->draw_text({Background_Tile_Count_X / 2.0f - rect.width / 2.0f,7.0f},{0.5f,0.5f},{1,1,1},"Press 'Enter' to return to the main menu.");
				break;
			}
		}
	}

	bool Game::quit_requested() const noexcept {
		return quit;
	}
}
