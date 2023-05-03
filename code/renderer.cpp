#include <new>
#include <cstdio>
#include <vector>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include "defer.hpp"
#include "opengl.hpp"
#include "renderer.hpp"
#include "platform.hpp"
#include "exceptions.hpp"

namespace core {
	//static constexpr std::size_t Max_Simultaneous_Textures = 4;
	static constexpr std::size_t Scene_Data_Uniform_Buffer_Size = 16384;
	static char info_log_buffer[4096];

	struct Object_Data {
		alignas(16) Mat4 matrix;
	};

	struct Sprite {
		bool has_value;
		GLuint texture_id;
		std::uint32_t generation;
		GLuint scene_data_uniform_buffer;
		std::vector<Object_Data> object_datas;
		std::size_t current_object_data_index;
	};

	struct Renderer_Internal_Data {
		GLuint shader_program;
		GLuint sprite_vertex_array_id;
		GLuint sprite_buffer_id;
		std::vector<Sprite> sprites;
	};

	static constexpr const char Vertex_Shader_Source_Format[] = R"xxx(
		#version 420 core
		in vec3 position;
		in vec2 tex_coords;
		out vec2 out_tex_coords;
		uniform mat4 projection_matrix;
		struct Object_Data {
			mat4 matrix;
		};
		layout(std140,binding = 0) uniform Scene_Data {
			Object_Data[204] object_datas;
		};
		void main() {
			gl_Position = projection_matrix * object_datas[gl_InstanceID].matrix * vec4(position,1.0);
			out_tex_coords = tex_coords;
		}
	)xxx";

	static constexpr const char Fragment_Shader_Source_Format[] = R"xxx(
		#version 420 core
		in vec2 out_tex_coords;
		out vec4 out_color;
		uniform sampler2D sprite_textures;
		void main() {
			out_color = texture(sprite_textures,out_tex_coords);
		}
	)xxx";

	[[nodiscard]] static const char* opengl_debug_source_to_string(GLenum value) {
		switch(value) {
			case GL_DEBUG_SOURCE_API: return "GL_DEBUG_SOURCE_API";
			case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "GL_DEBUG_SOURCE_WINDOW_SYSTEM";
			case GL_DEBUG_SOURCE_SHADER_COMPILER: return "GL_DEBUG_SOURCE_SHADER_COMPILER";
			case GL_DEBUG_SOURCE_THIRD_PARTY: return "GL_DEBUG_SOURCE_THIRD_PARTY";
			case GL_DEBUG_SOURCE_APPLICATION: return "GL_DEBUG_SOURCE_APPLICATION";
			case GL_DEBUG_SOURCE_OTHER: return "GL_DEBUG_SOURCE_OTHER";
			default: return "[Invalid]";
		}
	}

	[[nodiscard]] static const char* opengl_debug_type_to_string(GLenum value) {
		switch(value) {
			case GL_DEBUG_TYPE_ERROR: return "GL_DEBUG_TYPE_ERROR";
			case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR";
			case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR";
			case GL_DEBUG_TYPE_PORTABILITY: return "GL_DEBUG_TYPE_PORTABILITY";
			case GL_DEBUG_TYPE_PERFORMANCE: return "GL_DEBUG_TYPE_PERFORMANCE";
			case GL_DEBUG_TYPE_MARKER: return "GL_DEBUG_TYPE_MARKER";
			case GL_DEBUG_TYPE_PUSH_GROUP: return "GL_DEBUG_TYPE_PUSH_GROUP";
			case GL_DEBUG_TYPE_POP_GROUP: return "GL_DEBUG_TYPE_POP_GROUP";
			case GL_DEBUG_TYPE_OTHER: return "GL_DEBUG_TYPE_OTHER ";
			default: return "[Invalid]";
		}
	}

	[[nodiscard]] static const char* opengl_debug_severity_to_string(GLenum value) {
		switch(value) {
			case GL_DEBUG_SEVERITY_LOW: return "GL_DEBUG_SEVERITY_LOW";
			case GL_DEBUG_SEVERITY_MEDIUM: return "GL_DEBUG_SEVERITY_MEDIUM";
			case GL_DEBUG_SEVERITY_HIGH: return "GL_DEBUG_SEVERITY_HIGH";
			case GL_DEBUG_SEVERITY_NOTIFICATION: return "GL_DEBUG_SEVERITY_NOTIFICATION";
			default: return "[Invalid]";
		}
	}

	static void APIENTRY debug_message_callback(GLenum source,GLenum type,GLuint id,GLenum severity,[[maybe_unused]] GLsizei message_length,const GLchar* message,const void*) {
		std::cerr << "[OpenGL] Source: " << core::opengl_debug_source_to_string(source) <<
					 ", type: " << core::opengl_debug_type_to_string(type) <<
					 ", id: " << id <<
					 ", severity: " << core::opengl_debug_severity_to_string(severity) <<
					 "\n" << message << "\n" << std::endl;
	}

	Renderer::Renderer(Platform* _platform) try : platform(_platform),data_buffer() {
		static_assert(sizeof(Renderer_Internal_Data) <= sizeof(data_buffer));
		Renderer_Internal_Data& data = *new(data_buffer) Renderer_Internal_Data();

		glDisable(GL_DITHER);
		glDisable(GL_MULTISAMPLE);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_TRUE);
		glClearDepth(1.0f);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CW);
#if defined(DEBUG_BUILD)
		glEnable(GL_DEBUG_OUTPUT);
		glDebugMessageControl(GL_DONT_CARE,GL_DONT_CARE,GL_DONT_CARE,0,nullptr,GL_TRUE);
		glDebugMessageCallback(&debug_message_callback,nullptr);
#endif

		auto create_shader = [&](GLenum type,const char* type_string,const char* source,std::size_t source_byte_length){
			GLuint shader = glCreateShader(type);
			if(shader == 0) throw Object_Creation_Exception(type_string);
			
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
			GLuint vertex_shader = create_shader(GL_VERTEX_SHADER,"GL_VERTEX_SHADER",Vertex_Shader_Source_Format,sizeof(Vertex_Shader_Source_Format) - 1);
			defer[&]{glDeleteShader(vertex_shader);};

			GLuint fragment_shader = create_shader(GL_FRAGMENT_SHADER,"GL_FRAGMENT_SHADER",Fragment_Shader_Source_Format,sizeof(Fragment_Shader_Source_Format) - 1);
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
		glUseProgram(data.shader_program);
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(glGetUniformLocation(data.shader_program,"sprite_textures"),0);

		GLint position_location = glGetAttribLocation(data.shader_program,"position");
		GLint tex_coords_location = glGetAttribLocation(data.shader_program,"tex_coords");
		
		{
			glGenBuffers(1,&data.sprite_buffer_id);
			glBindBuffer(GL_ARRAY_BUFFER,data.sprite_buffer_id);

			static constexpr Vertex Sprite_Vertices[] = {
				{{0.0f,0.0f},{0.0f,0.0f}},
				{{1.0f,0.0f},{1.0f,0.0f}},
				{{1.0f,1.0f},{1.0f,1.0f}},
				{{0.0f,0.0f},{0.0f,0.0f}},
				{{1.0f,1.0f},{1.0f,1.0f}},
				{{0.0f,1.0f},{0.0f,1.0f}},
			};

			glBufferData(GL_ARRAY_BUFFER,sizeof(Sprite_Vertices),Sprite_Vertices,GL_STATIC_DRAW);

			GLint64 actual_buffer_size = 0;
			glGetBufferParameteri64v(GL_ARRAY_BUFFER,GL_BUFFER_SIZE,&actual_buffer_size);
			if(actual_buffer_size != sizeof(Sprite_Vertices)) throw Runtime_Exception("Couldn't allocate background tiles buffer memory.");

			glGenVertexArrays(1,&data.sprite_vertex_array_id);
			glBindVertexArray(data.sprite_vertex_array_id);

			glEnableVertexAttribArray(position_location);
			glVertexAttribPointer(position_location,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(0 * sizeof(float)));

			glEnableVertexAttribArray(tex_coords_location);
			glVertexAttribPointer(tex_coords_location,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(3 * sizeof(float)));
		}
		adjust_viewport();
	}
	catch(...) { destroy(); throw; }

	void Renderer::destroy() noexcept {
		Renderer_Internal_Data& data = *std::launder(reinterpret_cast<Renderer_Internal_Data*>(data_buffer));
		for(auto& sprite : data.sprites) {
			glDeleteBuffers(1,&sprite.scene_data_uniform_buffer);
			glDeleteTextures(1,&sprite.texture_id);
		}
		data.sprites.clear();

		if(glIsVertexArray(data.sprite_vertex_array_id))
			glDeleteVertexArrays(1,&data.sprite_vertex_array_id);
		if(glIsBuffer(data.sprite_buffer_id))
			glDeleteBuffers(1,&data.sprite_buffer_id);
		if(glIsProgram(data.shader_program))
			glDeleteProgram(data.shader_program);
	}

	Renderer::~Renderer() { destroy(); }

	void Renderer::begin(float r,float g,float b) {
		Renderer_Internal_Data& data = *std::launder(reinterpret_cast<Renderer_Internal_Data*>(data_buffer));
		if(platform->window_resized()) adjust_viewport();

		glClearColor(r,g,b,1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void Renderer::end() {
		Renderer_Internal_Data& data = *std::launder(reinterpret_cast<Renderer_Internal_Data*>(data_buffer));
		for(auto& sprite : data.sprites) {
			if(!sprite.has_value) continue;
			glBindBufferBase(GL_UNIFORM_BUFFER,0,sprite.scene_data_uniform_buffer);
			glBufferSubData(GL_UNIFORM_BUFFER,0,sprite.current_object_data_index * sizeof(Object_Data),sprite.object_datas.data());
			glBindTexture(GL_TEXTURE_2D,sprite.texture_id);
			glDrawArraysInstanced(GL_TRIANGLES,0,6,sprite.current_object_data_index);
			sprite.current_object_data_index = 0;
		}
		glDrawArrays(GL_TRIANGLES,0,6);
	}

	void Renderer::draw_sprite(Vec2 position,Vec2 size,const Sprite_Index& sprite_index) {
		Renderer_Internal_Data& data = *std::launder(reinterpret_cast<Renderer_Internal_Data*>(data_buffer));
		if(sprite_index.index >= data.sprites.size()) throw Runtime_Exception("Invalid sprite index (out of bounds).");
		Sprite& sprite = data.sprites[sprite_index.index];
		if(sprite.generation != sprite_index.generation || !sprite.has_value) throw Runtime_Exception("Invalid sprite index (outdated).");

		if((sprite.current_object_data_index + 1) >= sprite.object_datas.size()) {
			glBindBufferBase(GL_UNIFORM_BUFFER,0,sprite.scene_data_uniform_buffer);
			glBufferSubData(GL_UNIFORM_BUFFER,0,sprite.current_object_data_index * sizeof(Object_Data),sprite.object_datas.data());
			glBindTexture(GL_TEXTURE_2D,sprite.texture_id);
			glDrawArraysInstanced(GL_TRIANGLES,0,6,sprite.current_object_data_index);
			sprite.current_object_data_index = 0;
		}
		sprite.object_datas[sprite.current_object_data_index].matrix = core::translate(position.x,position.y,0) * core::scale(size.x,size.y,1);
		sprite.current_object_data_index += 1;
	}

	[[nodiscard]] static std::vector<std::uint8_t> load_bitmap_from_file(const char* file_path,std::uint32_t* out_width,std::uint32_t* out_height) {
		std::strcpy(info_log_buffer,file_path);

		auto file = std::ifstream(file_path,std::ios::binary);
		if(!file.is_open()) throw File_Open_Exception(info_log_buffer);

		auto read = [&]<typename T>() {
			T value = T();
			if(!file.read(reinterpret_cast<char*>(&value),sizeof(value))) throw File_Read_Exception(info_log_buffer,sizeof(value));
			return value;
		};

		char magic_bytes[2] = {};
		if(!file.read(magic_bytes,2)) throw File_Read_Exception(info_log_buffer,2);
		if(magic_bytes[0] != 'B' || magic_bytes[1] != 'M') throw File_Exception(info_log_buffer,"Invalid magic bytes at the beginning");

		auto bitmap_file_size = read.operator()<std::uint32_t>();
		auto reserved_data = read.operator()<std::uint32_t>();
		auto pixel_data_offset = read.operator()<std::uint32_t>();

		auto info_header_size = read.operator()<std::uint32_t>();
		if(info_header_size != 124) throw File_Exception(info_log_buffer,"Only BITMAPV5HEADER is supported");

		auto width = read.operator()<std::int32_t>();
		auto height = read.operator()<std::int32_t>();

		auto plane_count = read.operator()<std::uint16_t>();
		if(plane_count != 1) throw File_Exception(info_log_buffer,"Plane count must be 1");

		auto bit_count = read.operator()<std::uint16_t>();
		if(bit_count != 32) throw File_Exception(info_log_buffer,"Bit count must be 32");

		auto compression_method_index = read.operator()<std::uint32_t>();
		if(compression_method_index != 3) throw File_Exception(info_log_buffer,"Compression must be BI_BITFIELDS");

		[[maybe_unused]] auto image_size = read.operator()<std::uint32_t>();
		[[maybe_unused]] auto hor_resolution = read.operator()<std::int32_t>();
		[[maybe_unused]] auto ver_resolution = read.operator()<std::int32_t>();
		[[maybe_unused]] auto palatte_used_color_count = read.operator()<std::uint32_t>();
		[[maybe_unused]] auto palatte_important_color_count = read.operator()<std::uint32_t>();

		auto red_mask = read.operator()<std::uint32_t>();
		auto green_mask = read.operator()<std::uint32_t>();
		auto blue_mask = read.operator()<std::uint32_t>();
		auto alpha_mask = read.operator()<std::uint32_t>();

		if(!file.seekg(pixel_data_offset,std::ios::beg)) throw File_Seek_Exception(info_log_buffer,pixel_data_offset);

		std::vector<std::uint8_t> pixels{};
		pixels.resize(std::size_t(width) * height * 4);
		if(!file.read(reinterpret_cast<char*>(pixels.data()),pixels.size())) throw File_Read_Exception(info_log_buffer,pixels.size());

		for(std::size_t i = 0;i < pixels.size();i += 4) {
			std::uint32_t value = (std::uint32_t(pixels[i + 3]) << 24u) | (std::uint32_t(pixels[i + 2]) << 16u) | (std::uint32_t(pixels[i + 1]) << 8u) | (std::uint32_t(pixels[i + 0]) << 0u);
			pixels[i + 0] = ((value & red_mask) >> core::leading_zeroes(red_mask));
			pixels[i + 1] = ((value & green_mask) >> core::leading_zeroes(green_mask));
			pixels[i + 2] = ((value & blue_mask) >> core::leading_zeroes(blue_mask));
			pixels[i + 3] = ((value & alpha_mask) >> core::leading_zeroes(alpha_mask));
		}
		*out_width = width;
		*out_height = height;
		return pixels;
	}

	Sprite_Index Renderer::new_sprite(const char* file_path) {
		Renderer_Internal_Data& data = *std::launder(reinterpret_cast<Renderer_Internal_Data*>(data_buffer));

		std::uint32_t width = 0;
		std::uint32_t height = 0;
		std::vector<std::uint8_t> pixels = core::load_bitmap_from_file(file_path,&width,&height);

		GLuint texture_id = 0;
		glGenTextures(1,&texture_id);
		glBindTexture(GL_TEXTURE_2D,texture_id);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());

		GLuint uniform_buffer_id = 0;
		glGenBuffers(1,&uniform_buffer_id);
		glBindBuffer(GL_UNIFORM_BUFFER,uniform_buffer_id);
		glBufferData(GL_UNIFORM_BUFFER,Scene_Data_Uniform_Buffer_Size,nullptr,GL_DYNAMIC_DRAW);

		GLint64 actual_size = 0;
		glGetBufferParameteri64v(GL_UNIFORM_BUFFER,GL_BUFFER_SIZE,&actual_size);
		if(Scene_Data_Uniform_Buffer_Size != actual_size) {
			glDeleteTextures(1,&texture_id);
			throw Runtime_Exception("Couldn't allocate an uniform buffer.");
		}

		for(std::size_t i = 0;i < data.sprites.size();i += 1) {
			auto& sprite = data.sprites[i];
			if(!sprite.has_value) {
				sprite.has_value = true;
				sprite.generation += 1;
				sprite.texture_id = texture_id;
				sprite.scene_data_uniform_buffer = uniform_buffer_id;
				sprite.current_object_data_index = 0;
				return {i,sprite.generation};
			}
		}
		try {
			Sprite sprite{};
			sprite.has_value = true;
			sprite.generation = 1;
			sprite.texture_id = texture_id;
			sprite.scene_data_uniform_buffer = uniform_buffer_id;
			sprite.current_object_data_index = 0;
			sprite.object_datas.resize(204);
			data.sprites.push_back(std::move(sprite));
			return {data.sprites.size() - 1,1};
		}
		catch(...) {
			glDeleteBuffers(1,&uniform_buffer_id);
			glDeleteTextures(1,&texture_id);
			throw;
		}
	}

	void Renderer::adjust_viewport() {
		Renderer_Internal_Data& data = *std::launder(reinterpret_cast<Renderer_Internal_Data*>(data_buffer));

		auto dims = platform->window_client_dimensions();
		std::uint32_t offset_x = 0,offset_y = 0;
		if(dims.width * 3 >= dims.height * 4) {
			std::uint32_t new_width = (4 * dims.height) / 3;
			offset_x = (dims.width - new_width) / 2;
			dims.width = new_width;
		}
		else {
			std::uint32_t new_height = (3 * dims.width) / 4;
			offset_y = (dims.height - new_height) / 2;
			dims.height = new_height;
		}
		glViewport(offset_x,offset_y,dims.width,dims.height);

		Mat4 matrix = core::orthographic(0,float(Background_Tile_Count_X),0,float(Background_Tile_Count_Y),-1,1);
		glUniformMatrix4fv(glGetUniformLocation(data.shader_program,"projection_matrix"),1,GL_FALSE,&matrix(0,0));
	}
}
