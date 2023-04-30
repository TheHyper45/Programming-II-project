#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <cstddef>
#include <cstdint>

namespace core {
	struct Vec2 {
		float x;
		float y;
	};

	struct Vertex {
		Vec2 position;
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
		void begin(float r,float g,float b,const Texture& texture);
		void end();
		[[nodiscard]] Texture create_texture(std::uint32_t width,std::uint32_t height,const std::uint8_t* pixels);
	private:
		Platform* platform;
		alignas(std::max_align_t) unsigned char data_buffer[64];
		friend class Texture;
	};
}

#endif
