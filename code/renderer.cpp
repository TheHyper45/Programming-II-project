#include "opengl.hpp"
#include "renderer.hpp"

namespace core {
	Renderer::Renderer() {

	}

	Renderer::~Renderer() {

	}

	void Renderer::begin(float r,float g,float b) {
		glClearColor(r,g,b,1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	void Renderer::end() {

	}
}
