#include <new>
#include <vector>
#include <fstream>
#include <iomanip>
#include <iostream>
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

	Texture::Texture(Renderer* _renderer,std::uint32_t width,std::uint32_t height,const std::uint8_t* pixels) : renderer(_renderer),texture_id() {
		Renderer_Internal_Data& data = *std::launder(reinterpret_cast<Renderer_Internal_Data*>(renderer->data_buffer));

		glGenTextures(1,&texture_id);
		glBindTexture(GL_TEXTURE_2D,texture_id);

		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);

		glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
	}

	Texture::~Texture() {
		glDeleteTextures(1,&texture_id);
	}

	static constexpr const char Vertex_Shader_Source[] = R"xxx(
		#version 420 core
		in vec2 position;
		in vec2 tex_coords;
		out vec2 out_tex_coords;
		void main() {
			gl_Position = vec4(position,0.0,1.0);
			out_tex_coords = tex_coords;
		}
	)xxx";

	static constexpr const char Fragment_Shader_Source[] = R"xxx(
		#version 420 core
		in vec2 out_tex_coords;
		out vec4 out_color;
		uniform sampler2D tex0;
		void main() {
			out_color = texture(tex0,out_tex_coords);
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
			if(shader == 0) throw Tagged_Runtime_Exception("Couldn't create a shader",type);
			
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

		Vertex quad_data[] = {
			{{-0.5f, 0.5f},{0.0f,0.0f}},
			{{ 0.5f, 0.5f},{1.0f,0.0f}},
			{{ 0.5f,-0.5f},{1.0f,1.0f}},
			{{-0.5f, 0.5f},{0.0f,0.0f}},
			{{ 0.5f,-0.5f},{1.0f,1.0f}},
			{{-0.5f,-0.5f},{0.0f,1.0f}},
		};
		glBufferData(GL_ARRAY_BUFFER,GLsizeiptr(sizeof(quad_data)),quad_data,GL_STATIC_DRAW);

		GLint position_location = glGetAttribLocation(data.shader_program,"position");
		GLint tex_coords_location = glGetAttribLocation(data.shader_program,"tex_coords");

		glEnableVertexAttribArray(position_location);
		glVertexAttribPointer(position_location,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(0));

		glEnableVertexAttribArray(tex_coords_location);
		glVertexAttribPointer(tex_coords_location,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(2 * sizeof(float)));

		glUseProgram(data.shader_program);
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(glGetUniformLocation(data.shader_program,"tex0"),0);
	};

	Renderer::~Renderer() {
		Renderer_Internal_Data& data = *std::launder(reinterpret_cast<Renderer_Internal_Data*>(data_buffer));
		glDeleteBuffers(1,&data.quad_buffer);
		glDeleteVertexArrays(1,&data.vertex_array);
		glDeleteProgram(data.shader_program);
	}

	void Renderer::begin(float r,float g,float b,const Texture& texture) {
		Renderer_Internal_Data& data = *std::launder(reinterpret_cast<Renderer_Internal_Data*>(data_buffer));

		if(platform->window_resized()) {
			auto dims = platform->window_client_dimensions();
			glViewport(0,0,dims.width,dims.height);
		}

		glClearColor(r,g,b,1.0f);
		glClearDepth(1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBindTexture(GL_TEXTURE_2D,texture.texture_id);
		glDrawArrays(GL_TRIANGLES,0,6);
	}

	void Renderer::end() {

	}

	Texture Renderer::create_texture(std::uint32_t width,std::uint32_t height,const std::uint8_t* pixels) {
		return Texture(this,width,height,pixels);

		/*static constexpr std::uint32_t Compression_None = 0;
		static constexpr std::uint32_t Compression_None_With_Bitfields = 3;

		std::strcpy(info_log_buffer,file_name);

		auto file = std::ifstream(file_name,std::ios::binary);
		if(!file.is_open()) throw Tagged_Runtime_Exception<const char*>("Couldn't open a file",info_log_buffer);

		char magic[2] = {};
		if(!file.read(magic,2)) throw Tagged_Runtime_Exception<const char*>("Couldn't read 2 bytes from file",info_log_buffer);
		if(magic[0] != 'B' || magic[1] != 'M') throw Tagged_Runtime_Exception<const char*>("Invalid magic bytes at the beginning",info_log_buffer);

		std::uint32_t bitmap_size = 0;
		if(!file.read(reinterpret_cast<char*>(&bitmap_size),4)) throw Tagged_Runtime_Exception<const char*>("Couldn't read 4 bytes from file",info_log_buffer);

		std::uint32_t reserved_bytes = 0;
		if(!file.read(reinterpret_cast<char*>(&reserved_bytes),4)) throw Tagged_Runtime_Exception<const char*>("Couldn't read 4 bytes from file",info_log_buffer);

		std::uint32_t pixel_byte_offset = 0;
		if(!file.read(reinterpret_cast<char*>(&pixel_byte_offset),4)) throw Tagged_Runtime_Exception<const char*>("Couldn't read 4 bytes from file",info_log_buffer);

		std::uint32_t bitmap_info_header_size = 0;
		if(!file.read(reinterpret_cast<char*>(&bitmap_info_header_size),4)) throw Tagged_Runtime_Exception<const char*>("Couldn't read 4 bytes from file",info_log_buffer);
		
		//BITMAPV4HEADER or BITMAPV5HEADER
		std::uint32_t compression_method = 0;
		std::int32_t width = 0;
		std::int32_t height = 0;
		if(bitmap_info_header_size == 108 || bitmap_info_header_size == 124) {
			if(!file.read(reinterpret_cast<char*>(&width),4)) throw Tagged_Runtime_Exception<const char*>("Couldn't read 4 bytes from file",info_log_buffer);
			if(!file.read(reinterpret_cast<char*>(&height),4)) throw Tagged_Runtime_Exception<const char*>("Couldn't read 4 bytes from file",info_log_buffer);

			std::uint16_t plane_count = 0;
			if(!file.read(reinterpret_cast<char*>(&plane_count),2)) throw Tagged_Runtime_Exception<const char*>("Couldn't read 2 bytes from file",info_log_buffer);
			if(plane_count != 1) throw Tagged_Runtime_Exception<const char*>("Invalid bitmap info header",info_log_buffer);

			std::uint16_t bits_per_pixel = 0;
			if(!file.read(reinterpret_cast<char*>(&bits_per_pixel),2)) throw Tagged_Runtime_Exception<const char*>("Couldn't read 2 bytes from file",info_log_buffer);
			if(bits_per_pixel != 32) throw Tagged_Runtime_Exception<const char*>("Invalid bitmap info header (number of bits per pixel != 32)",info_log_buffer);

			if(!file.read(reinterpret_cast<char*>(&compression_method),4)) throw Tagged_Runtime_Exception<const char*>("Couldn't read 4 bytes from file",info_log_buffer);
			if(compression_method != Compression_None && compression_method != Compression_None_With_Bitfields) throw Tagged_Runtime_Exception<const char*>("Invalid bitmap info header (bitmap uses compression)",info_log_buffer);

			std::uint32_t image_size = 0;
			if(!file.read(reinterpret_cast<char*>(&image_size),4)) throw Tagged_Runtime_Exception<const char*>("Couldn't read 4 bytes from file",info_log_buffer);

			std::int32_t hor_resolution = 0;
			if(!file.read(reinterpret_cast<char*>(&hor_resolution),4)) throw Tagged_Runtime_Exception<const char*>("Couldn't read 4 bytes from file",info_log_buffer);

			std::int32_t ver_resolution = 0;
			if(!file.read(reinterpret_cast<char*>(&ver_resolution),4)) throw Tagged_Runtime_Exception<const char*>("Couldn't read 4 bytes from file",info_log_buffer);

			std::uint32_t important_color_count = 0;
			if(!file.read(reinterpret_cast<char*>(&important_color_count),4)) throw Tagged_Runtime_Exception<const char*>("Couldn't read 4 bytes from file",info_log_buffer);

			std::uint32_t used_important_color_count = 0;
			if(!file.read(reinterpret_cast<char*>(&used_important_color_count),4)) throw Tagged_Runtime_Exception<const char*>("Couldn't read 4 bytes from file",info_log_buffer);

			std::uint32_t rgba_masks[4] = {};
			if(!file.read(reinterpret_cast<char*>(rgba_masks),16)) throw Tagged_Runtime_Exception<const char*>("Couldn't read 16 bytes from file",info_log_buffer);
		}
		else throw Tagged_Runtime_Exception<const char*>("Invalid bitmap info header",info_log_buffer);

		if(!file.seekg(pixel_byte_offset,std::ios::beg)) throw Tagged_Runtime_Exception<const char*>("Couldn't read some bytes",info_log_buffer);

		std::vector<std::uint8_t> pixels = {};
		pixels.resize(std::size_t(width) * height * 4);
		if(compression_method == Compression_None) {
			if(!file.read(reinterpret_cast<char*>(pixels.data()),pixels.size())) throw Tagged_Runtime_Exception<const char*>("Couldn't read pixels",info_log_buffer);
		}
		else if(compression_method == Compression_None_With_Bitfields) {
			if(!file.read(reinterpret_cast<char*>(pixels.data()),pixels.size())) throw Tagged_Runtime_Exception<const char*>("Couldn't read pixels",info_log_buffer);
			for(std::size_t i = 0;i < pixels.size();i += 4) {
				std::uint8_t byte0 = pixels[i + 0];
				std::uint8_t byte1 = pixels[i + 1];
				std::uint8_t byte2 = pixels[i + 2];
				std::uint8_t byte3 = pixels[i + 3];

			}
		}
		return Texture(this,0,0,pixels.data());*/
	}
}
