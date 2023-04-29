#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <cstddef>

namespace core {
	struct Vec2 {
		float x;
		float y;
	};

	class Platform;
	class Renderer {
	public:
		explicit Renderer(Platform* _platform);
		~Renderer();
		void begin(float r,float g,float b);
		void end();
	private:
		Platform* platform;
		alignas(std::max_align_t) unsigned char data_buffer[64];
	};
}

#endif
