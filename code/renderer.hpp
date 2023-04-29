#ifndef RENDERER_HPP
#define RENDERER_HPP

namespace core {
	class Renderer {
	public:
		Renderer();
		~Renderer();
		void begin(float r,float g,float b);
		void end();
	private:
	};
}

#endif
