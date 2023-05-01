#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <cstddef>
#include <cstdint>
#include "math.hpp"

namespace core {
	struct Vertex {
		Vec3 position;
		Vec2 tex_coords;
	};

	class Renderer;
	class Texture {
	public:
		~Texture();
		Texture(const Texture&) = delete;
		Texture& operator=(const Texture&) = delete;
	private:
		explicit Texture(Renderer* _renderer,std::uint32_t width,std::uint32_t height,const std::uint8_t* pixels);
		Renderer* renderer;
		unsigned int texture_id;
		friend class Renderer;
	};

	class Platform;
	class Renderer {
	public:
		explicit Renderer(Platform* _platform);
		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;
		~Renderer();
		void begin(float r,float g,float b);
		void end();
		void draw_quad(Vec3 position,Vec2 size,const Texture& texture);
		[[nodiscard]] Texture create_texture(std::uint32_t width,std::uint32_t height,const std::uint8_t* pixels);
	private:
		Platform* platform;
		alignas(std::max_align_t) unsigned char data_buffer[128];
		friend class Texture;
	};
}

#endif
