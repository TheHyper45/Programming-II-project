#ifndef BULLETS_HPP
#define BULLETS_HPP
#include "game.hpp"
#include "renderer.hpp"
#include "platform.hpp"


namespace core {
	enum class bullet_type
	{
		normal_bullet,
	};
	

	class Bullet {
	public:
		std::int32_t sprite_index=1;
		float direction;
		Vec3 position;
		Vec2 size = { 1.0f,1.0f };
		void move(float direction);
		void animate(float delta_time);
		
		Bullet(std::int32_t sprite_index, float direction, Vec3 position);
		void render(core::Renderer* renderer, core::Sprite_Index tiles_sprite_atlas);

		bool operator == (const Bullet& b) const { 
			return position.x == b.position.x && position.y == b.position.y && position.z == b.position.z && 
				size.x==b.size.x && size.y ==b.size.y
				&& direction == b.direction; }
	};
	
}
#endif
