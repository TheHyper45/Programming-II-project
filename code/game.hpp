#ifndef GAME_HPP
#define GAME_HPP

#include <list>
#include <vector>
#include <random>
#include <cstddef>
#include <optional>
#include "platform.hpp"
#include "renderer.hpp"



namespace core {
	enum class Scene {
		Main_Menu,
		Intro_1player,
		Game_1player,
		Outro_1player,
		Intro_2player,
		Game_2player,
		Outro_2player,
		Construction,
		Level_Selection,
		Single_Multiplayer_Selection,
		Level_Selected,
		Game_Over_1player,
		Game_Over_2player,
		Victory_Screen
	};

	enum struct Tile_Flag {
		Solid,
		Below,
		Above,
		Bulletpass
	};
	[[nodiscard]] inline float tile_flag_to_z_order(Tile_Flag flag) {
		switch(flag) {
			case Tile_Flag::Above: return 0.75f;
			case Tile_Flag::Bulletpass:
			case Tile_Flag::Below: return -0.25f;
			default: return 0.0f;
		}
	}

	struct Tile_Template {
		std::uint32_t tile_layer_index;
		std::uint32_t health;
		float rotation;
		Tile_Flag flag;
	};
	struct Tile {
		std::uint32_t template_index;
		std::uint32_t health;
	};

	enum class Entity_Direction { Right,Down,Left,Up };
	struct Entity_Direction_Triple {
		Entity_Direction dir0;
		Entity_Direction dir1;
		Entity_Direction dir_back;
	};

	[[nodiscard]] inline float entity_direction_to_rotation(Entity_Direction dir) {
		switch(dir) {
			case Entity_Direction::Right: return PI / 2.0f;
			case Entity_Direction::Down: return PI;
			case Entity_Direction::Left: return -PI / 2.0f;
			case Entity_Direction::Up: return 0.0f;
			default: return 0.0f;
		}
	}
	[[nodiscard]] inline Vec2 entity_direction_to_vector(Entity_Direction dir) {
		switch(dir) {
			case Entity_Direction::Right: return {1.0f,0.0f};
			case Entity_Direction::Down: return {0.0f,1.0f};
			case Entity_Direction::Left: return {-1.0f,0.0f};
			case Entity_Direction::Up: return {0.0f,-1.0f};
			default: return {};
		}
	}

	struct Tank {
		Vec2 position;
		Entity_Direction dir;
		bool destroyed;
		float shoot_cooldown;
		float ai_dir_change_timer;
		float ai_react_timer;
		std::uint32_t hop_count_until_shoot;
	};
	struct Bullet {
		Vec2 position;
		Entity_Direction dir;
		bool fired_by_player;
		bool destroyed;
	};
	struct Eagle {
		Vec2 position;
		bool destroyed;
	};
	struct Spawn_Effect {
		Vec2 position;
		std::uint32_t current_frame;
		float timer;
	};
	struct Explosion {
		Vec3 position;
		Vec2 size;
		float timer;
		int sprite_index;
		bool destroyed;
		int texture_serie;
	};
	struct Player {
		Tank tank;
		float respawn_timer;
		std::uint32_t lifes;
		float invulnerability_timer;
	};
	struct Raycast_Outcome {
		enum class Type { None,Tile,Player1,Player2,Eagle };
		Type type;
		Vec2 impact_point;
		[[nodiscard]] bool hit_target() const noexcept { return type == Type::Player1 || type == Type::Player2 || type == Type::Eagle; }
	};

	class Game {
	public:
		Game(const Game&) = delete;
		Game& operator=(const Game&) = delete;
		Game(Renderer* _renderer,Platform* _platform);

		void update(float delta_time);
		void render(float delta_time);
		[[nodiscard]] bool quit_requested() const noexcept;
	private:
		void update_player(Player* player,float delta_time);
		void update_enemies(float delta_time);
		void update_bullets(float delta_time);
		std::optional<Ipoint> check_collision_with_tiles(Vec2* out_position,Vec2 collider_size,std::int32_t start_x,std::int32_t start_y,std::int32_t end_x,std::int32_t end_y,Entity_Direction dir,bool is_bullet = false);
		[[nodiscard]] Raycast_Outcome raycast(Vec2 origin,Entity_Direction dir,bool include_bulletpass_tiles,bool skip_tiles,bool skip_targets);

		void add_spawn_effect(Vec2 position);
		void add_explosion(Vec2 position,float delta_time);
		void save_map(const char* file_path);
		void load_map(const char* file_path);
		void render_map();
		static inline constexpr std::uint32_t Invalid_Tile_Index = std::uint32_t(-1);

		Renderer* renderer;
		Platform* platform;
		Scene scene;
		std::size_t current_main_menu_option;
		std::size_t current_map_option=0;
		std::size_t current_player_mode_option = 0;
		float update_timer;
		Sprite_Index tiles_texture;
		Sprite_Index construction_place_marker;
		Sprite_Index entity_sprites;
		Sprite_Index spawn_effect_sprite_atlas;
		Sprite_Index explosion_sprite;
		Point construction_marker_pos;
		bool construction_choosing_tile;
		Point construction_tile_choice_marker_pos;
		std::uint32_t construction_current_tile_template_index;
		std::vector<Tile_Template> tile_templates;
		bool show_fps;
		bool quit;
		Tile tiles[(Background_Tile_Count_X * 2) * (Background_Tile_Count_Y * 2)];
		Eagle eagle;
		std::vector<Bullet> bullets;
		std::vector<Tank> enemy_tanks;
		std::vector<Explosion> explosions;
		float game_lose_timer;
		std::vector<Spawn_Effect> spawn_effects;
		std::minstd_rand0 random_engine;
		std::uniform_int_distribution<std::size_t> enamy_spawn_point_random_dist;
		std::uniform_real_distribution<float> enemy_action_duration_dist;
		std::uniform_real_distribution<float> chance_0_1_dist;
		std::uint32_t max_enemy_count_on_screen;
		std::uint32_t remaining_enemy_count_to_spawn;
		float enemy_spawn_timer;
		float game_win_timer;
		std::size_t current_stage_index;
		Player first_player;
		Player second_player;
	};
}
#endif
