#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <cstddef>
#include <cstdint>
#include "math.hpp"

namespace core {
	static inline constexpr std::uint32_t Background_Tile_Count_X = 16;
	static inline constexpr std::uint32_t Background_Tile_Count_Y = 12;

	struct Vertex {
		Vec3 position;
		Vec2 tex_coords;
	};

	struct Sprite_Index {
		std::size_t index;
		std::uint32_t generation;
	};

	class Platform;
	class Renderer {
		void destroy() noexcept;
		void adjust_viewport();
		[[nodiscard]] Sprite_Index insert_sprite(unsigned int texture_id,unsigned int uniform_buffer_id,std::uint32_t array_layers);
		explicit Renderer(Platform* _platform);
	public:
		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;
		~Renderer();
		void begin(float r,float g,float b);
		void end();
		void draw_sprite(Vec2 position,Vec2 size,const Sprite_Index& sprite_index,std::uint32_t tile_index = 0);
		[[nodiscard]] Sprite_Index sprite(const char* file_path);
		[[nodiscard]] Sprite_Index sprite_atlas(const char* file_path,std::uint32_t tile_dimension);
	private:
		Platform* platform;
		alignas(std::max_align_t) unsigned char data_buffer[128];
		friend class Platform;
	};
}

#endif
