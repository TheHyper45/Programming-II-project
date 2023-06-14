#ifndef GAME_HPP
#define GAME_HPP

#include <list>
#include <vector>
#include <random>
#include <cstddef>
#include "platform.hpp"
#include "renderer.hpp"

namespace core {
	enum class Scene {
		Main_Menu,
		Intro_1player,
		Game_1player,
		Outro_1player,
		Construction,
		Level_Selection,
		Game_Over
	};

	enum struct Tile_Flag {
		Solid,
		Below,
		Above,
		Bulletpass
	};
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

	class Game {
	public:
		Game(const Game&) = delete;
		Game& operator=(const Game&) = delete;
		Game(Renderer* _renderer,Platform* _platform);

		void update(float delta_time);
		void render(float delta_time);
		[[nodiscard]] bool quit_requested() const noexcept;
	private:
		void update_player(float delta_time);
		void add_spawn_effect(Vec2 position);
		void add_explosion(Vec2 position,float delta_time);
		void save_map(const char* file_path);
		void load_map(const char* file_path);
		static inline constexpr std::uint32_t Invalid_Tile_Index = std::uint32_t(-1);

		Renderer* renderer;
		Platform* platform;
		Scene scene;
		std::size_t current_main_menu_option;
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
		std::uint32_t player_lifes;
		Tank player_tank;
		Eagle eagle;
		std::vector<Bullet> bullets;
		std::vector<Tank> enemy_tanks;
		std::vector<Explosion> explosions;
		float game_lose_timer;
		std::vector<Spawn_Effect> spawn_effects;
		float player_shoot_cooldown;
		float player_respawn_timer;
	};
}
#endif
