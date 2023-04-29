#include <new>
#include "defer.hpp"
#include "opengl.hpp"
#include "renderer.hpp"
#include "platform.hpp"
#include "exceptions.hpp"

namespace core {
	static char info_log_buffer[4096];

	struct Renderer_Internal_Data {
		GLuint shader_program;
		GLuint vertex_array;
		GLuint quad_buffer;
	};

	static constexpr const char Vertex_Shader_Source[] = R"xxx(
		#version 420 core
		in vec2 position;
		void main() {
			gl_Position = vec4(position,0.0,1.0);
		}
	)xxx";

	static constexpr const char Fragment_Shader_Source[] = R"xxx(
		#version 420 core
		out vec4 out_color;
		void main() {
			out_color = vec4(1.0,0.0,0.0,1.0);
		}
	)xxx";

	Renderer::Renderer(Platform* _platform) : platform(_platform),data_buffer() {
		static_assert(sizeof(Renderer_Internal_Data) <= sizeof(data_buffer));
		Renderer_Internal_Data& data = *new(data_buffer) Renderer_Internal_Data();

		auto dims = platform->window_client_dimensions();
		glViewport(0,0,dims.width,dims.height);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_TRUE);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CW);
		glDisable(GL_DITHER);
		glDisable(GL_MULTISAMPLE);

		auto create_shader = [&](GLenum type,const char* source,std::size_t source_byte_length){
			GLuint shader = glCreateShader(type);
			if(shader == 0) throw Tagged_Runtime_Exception("Couldn't create a shader.",type);
			
			auto int_length = GLint(source_byte_length);
			glShaderSource(shader,1,&source,&int_length);
			glCompileShader(shader);

			GLint status = 0;
			glGetShaderiv(shader,GL_COMPILE_STATUS,&status);
			if(!status) {
				GLsizei msg_byte_length = 0;
				glGetShaderInfoLog(shader,sizeof(info_log_buffer) - 1,&msg_byte_length,info_log_buffer);
				glDeleteShader(shader);
				throw Runtime_Exception(info_log_buffer);
			}
			return shader;
		};

		{
			GLuint vertex_shader = create_shader(GL_VERTEX_SHADER,Vertex_Shader_Source,sizeof(Vertex_Shader_Source) - 1);
			defer[&]{glDeleteShader(vertex_shader);};

			GLuint fragment_shader = create_shader(GL_FRAGMENT_SHADER,Fragment_Shader_Source,sizeof(Fragment_Shader_Source) - 1);
			defer[&]{glDeleteShader(fragment_shader);};

			data.shader_program = glCreateProgram();
			if(data.shader_program == 0) throw Runtime_Exception("Couldn't create a shader program.");

			glAttachShader(data.shader_program,vertex_shader);
			defer[&]{glDetachShader(data.shader_program,vertex_shader);};

			glAttachShader(data.shader_program,fragment_shader);
			defer[&]{glDetachShader(data.shader_program,fragment_shader);};

			glLinkProgram(data.shader_program);

			GLint status = 0;
			glGetProgramiv(data.shader_program,GL_LINK_STATUS,&status);
			if(!status) {
				GLsizei msg_byte_length = 0;
				glGetProgramInfoLog(data.shader_program,sizeof(info_log_buffer) - 1,&msg_byte_length,info_log_buffer);
				glDeleteProgram(data.shader_program);
				throw Runtime_Exception(info_log_buffer);
			}
		}
		
		glGenVertexArrays(1,&data.vertex_array);
		glBindVertexArray(data.vertex_array);

		glGenBuffers(1,&data.quad_buffer);
		glBindBuffer(GL_ARRAY_BUFFER,data.quad_buffer);

		Vec2 quad_data[] = {{-0.5f,-0.5f},{0.0f,0.5f},{0.5f,-0.5f}};
		glBufferData(GL_ARRAY_BUFFER,GLsizeiptr(sizeof(quad_data)),quad_data,GL_STATIC_DRAW);

		GLint position_location = glGetAttribLocation(data.shader_program,"position");

		glEnableVertexAttribArray(position_location);
		glVertexAttribPointer(position_location,2,GL_FLOAT,GL_FALSE,0,0);
	};

	Renderer::~Renderer() {
		Renderer_Internal_Data& data = *std::launder(reinterpret_cast<Renderer_Internal_Data*>(data_buffer));
		glDeleteBuffers(1,&data.quad_buffer);
		glDeleteVertexArrays(1,&data.vertex_array);
		glDeleteProgram(data.shader_program);
	}

	void Renderer::begin(float r,float g,float b) {
		Renderer_Internal_Data& data = *std::launder(reinterpret_cast<Renderer_Internal_Data*>(data_buffer));

		if(platform->window_resized()) {
			auto dims = platform->window_client_dimensions();
			glViewport(0,0,dims.width,dims.height);
		}

		glClearColor(r,g,b,1.0f);
		glClearDepth(1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(data.shader_program);
		glDrawArrays(GL_TRIANGLES,0,3);
	}

	void Renderer::end() {

	}
}
