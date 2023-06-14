#include <new>
#include <cctype>
#include <cstdio>
#include <vector>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <cinttypes>
#include <filesystem>
#include "defer.hpp"
#include "opengl.hpp"
#include "renderer.hpp"
#include "platform.hpp"
#include "exceptions.hpp"

namespace core {
	//Using 'std140' layout in shaders, requires every member be aligned to 16 bytes.
	struct Object_Data {
		alignas(16) Mat4 matrix;
		alignas(16) std::uint32_t texture_index;
		alignas(16) Vec4 multiply_color;
		alignas(16) std::uint32_t effect_id;
	};

	struct Sprite {
		bool has_value;
		GLuint texture_id;
		std::uint32_t generation;
		GLuint scene_data_uniform_buffer;
		std::vector<Object_Data> object_datas;
		std::size_t current_object_data_index;
		std::uint32_t array_layers;
		std::uint32_t layer_size;
	};

	struct Font_Character_Info {
		std::uint32_t x_offset;
		std::uint32_t y_baseline_offset;
		std::uint32_t x_advance;
	};

	struct Renderer_Internal_Data {
		GLuint shader_program;
		GLuint sprite_vertex_array_id;
		GLuint sprite_buffer_id;
		std::vector<Sprite> sprites;
		std::size_t object_data_uniform_buffer_size;
		Sprite_Index font_sprite;
		std::vector<Font_Character_Info> font_character_infos;
		std::uint32_t font_largest_y_baseline_offset;
		GLint time_uniform_location;
		float time;
		Irect render_rect;
	};

	static constexpr const char Vertex_Shader_Source_Format[] = R"xxx(
		#version 460 core
		in vec3 position;
		in vec2 tex_coords;
		out vec2 out_tex_coords;
		out flat uint out_texture_index;
		out vec4 out_multiply_color;
		out flat uint out_effect_id;
		uniform mat4 projection_matrix;
		struct Object_Data {
			mat4 matrix;
			uint texture_index;
			vec4 multiply_color;
			uint effect_id;
		};
		layout(std140,binding = 0) uniform Scene_Data {
			Object_Data[%zu] object_datas;
		};
		void main() {
			gl_Position = projection_matrix * object_datas[gl_InstanceID].matrix * vec4(position,1.0);
			out_tex_coords = tex_coords;
			out_texture_index = object_datas[gl_InstanceID].texture_index;
			out_multiply_color = object_datas[gl_InstanceID].multiply_color;
			out_effect_id = object_datas[gl_InstanceID].effect_id;
		}
	)xxx";

	static constexpr const char Fragment_Shader_Source_Format[] = R"xxx(
		#version 460 core
		in vec2 out_tex_coords;
		in flat uint out_texture_index;
		in vec4 out_multiply_color;
		in flat uint out_effect_id;
		out vec4 out_color;
		uniform sampler2DArray sprite_textures;
		uniform float time;
		void main() {
			vec4 color = out_multiply_color * texture(sprite_textures,vec3(out_tex_coords,float(out_texture_index)));
			if(color.a < 0.5) discard;
			if(out_effect_id == 0) out_color = color;
			else {
				float tmp = (out_tex_coords.x + out_tex_coords.y) / 2.0 + time * 3.0;
				out_color = color * vec4(abs(sin(tmp)),abs(sin(tmp + 1)),abs(sin(tmp + 2)),1.0);
			}
		}
	)xxx";

	[[nodiscard]] static const char* opengl_debug_source_to_string(GLenum value) noexcept {
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

	[[nodiscard]] static const char* opengl_debug_type_to_string(GLenum value) noexcept {
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

	[[nodiscard]] static const char* opengl_debug_severity_to_string(GLenum value) noexcept {
		switch(value) {
			case GL_DEBUG_SEVERITY_LOW: return "GL_DEBUG_SEVERITY_LOW";
			case GL_DEBUG_SEVERITY_MEDIUM: return "GL_DEBUG_SEVERITY_MEDIUM";
			case GL_DEBUG_SEVERITY_HIGH: return "GL_DEBUG_SEVERITY_HIGH";
			case GL_DEBUG_SEVERITY_NOTIFICATION: return "GL_DEBUG_SEVERITY_NOTIFICATION";
			default: return "[Invalid]";
		}
	}

	static void APIENTRY debug_message_callback(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei,const GLchar* message,const void*) {
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
		{
			data.object_data_uniform_buffer_size = 16384; //OpenGL implementations don't have to support uniform buffers larger than 16384 bytes in size.
			GLint64 max_uniform_buffer_size = 0;
			glGetInteger64v(GL_MAX_UNIFORM_BLOCK_SIZE,&max_uniform_buffer_size);
			static constexpr std::size_t Desired_Uniform_Buffer_Size = 65536;
			if(std::size_t(max_uniform_buffer_size) < Desired_Uniform_Buffer_Size) data.object_data_uniform_buffer_size = max_uniform_buffer_size;
			else data.object_data_uniform_buffer_size = Desired_Uniform_Buffer_Size;
		}

		auto create_shader = [&](GLenum type,const char* type_string,const char* source,std::size_t source_byte_length){
			GLuint shader = glCreateShader(type);
			if(shader == 0) throw Object_Creation_Exception(type_string);
			
			auto int_length = GLint(source_byte_length);
			glShaderSource(shader,1,&source,&int_length);
			glCompileShader(shader);

			GLint status = 0;
			glGetShaderiv(shader,GL_COMPILE_STATUS,&status);
			if(!status) {
				static char info_log_buffer[4096];
				GLsizei msg_byte_length = 0;
				glGetShaderInfoLog(shader,sizeof(info_log_buffer) - 1,&msg_byte_length,info_log_buffer);
				glDeleteShader(shader);
				throw Runtime_Exception(info_log_buffer);
			}
			return shader;
		};

		{
			char vertex_shader_formatted_source[4096] = {};
			int format_result = std::snprintf(vertex_shader_formatted_source,sizeof(vertex_shader_formatted_source) - 1,
											  Vertex_Shader_Source_Format,data.object_data_uniform_buffer_size / sizeof(Object_Data));
			if(format_result < 0) throw Runtime_Exception("Couldn't preprocess the vertex shader source.");

			GLuint vertex_shader = create_shader(GL_VERTEX_SHADER,"GL_VERTEX_SHADER",vertex_shader_formatted_source,sizeof(vertex_shader_formatted_source) - 1);
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
				static char info_log_buffer[4096];
				GLsizei msg_byte_length = 0;
				glGetProgramInfoLog(data.shader_program,sizeof(info_log_buffer) - 1,&msg_byte_length,info_log_buffer);
				glDeleteProgram(data.shader_program);
				throw Runtime_Exception(info_log_buffer);
			}
		}
		glUseProgram(data.shader_program);
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(glGetUniformLocation(data.shader_program,"sprite_textures"),0);
		data.time_uniform_location = glGetUniformLocation(data.shader_program,"time");

		GLint position_location = glGetAttribLocation(data.shader_program,"position");
		GLint tex_coords_location = glGetAttribLocation(data.shader_program,"tex_coords");
		
		{
			glGenBuffers(1,&data.sprite_buffer_id);
			glBindBuffer(GL_ARRAY_BUFFER,data.sprite_buffer_id);

			static constexpr Vertex Sprite_Vertices[] = {
				{{-0.5f,-0.5f},{0.0f,0.0f}},
				{{ 0.5f,-0.5f},{1.0f,0.0f}},
				{{ 0.5f, 0.5f},{1.0f,1.0f}},
				{{-0.5f,-0.5f},{0.0f,0.0f}},
				{{ 0.5f, 0.5f},{1.0f,1.0f}},
				{{-0.5f, 0.5f},{0.0f,1.0f}},
			};

			glBufferData(GL_ARRAY_BUFFER,sizeof(Sprite_Vertices),Sprite_Vertices,GL_STATIC_DRAW);

			GLint64 actual_buffer_size = 0;
			glGetBufferParameteri64v(GL_ARRAY_BUFFER,GL_BUFFER_SIZE,&actual_buffer_size);
			if(std::size_t(actual_buffer_size) != sizeof(Sprite_Vertices)) throw Runtime_Exception("Couldn't allocate quad vertex buffer memory.");

			glGenVertexArrays(1,&data.sprite_vertex_array_id);
			glBindVertexArray(data.sprite_vertex_array_id);

			glEnableVertexAttribArray(position_location);
			glVertexAttribPointer(position_location,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(0 * sizeof(float)));

			glEnableVertexAttribArray(tex_coords_location);
			glVertexAttribPointer(tex_coords_location,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(3 * sizeof(float)));
		}
		adjust_viewport();
		data.font_sprite = sprite_atlas("./assets/font_16x16.bmp",16);
		{
			static constexpr const char* Font_Info_File_Path = "./assets/font_16x16.txt";
			auto file = std::ifstream(Font_Info_File_Path,std::ios::binary);
			if(!file.is_open()) throw File_Open_Exception(Font_Info_File_Path);

			std::size_t file_size = std::filesystem::file_size(Font_Info_File_Path);

			std::vector<char> text{};
			text.resize(file_size + 1); //Add 1 for the null-terminator.

			if(!file.read(text.data(),file_size)) throw File_Read_Exception(Font_Info_File_Path,file_size);
			text[text.size() - 1] = '\0';
			file.close();

			data.font_character_infos.reserve(128);
			for(std::size_t i = 0;i < text.size();i += 1) {
				if(std::isdigit(static_cast<unsigned char>(text[i]))) {
					Font_Character_Info info = {};
					int count = std::sscanf(&text[i],"%" PRIu32 " %" PRIu32 " %" PRIu32,&info.x_offset,&info.y_baseline_offset,&info.x_advance);
					if(count < 0) throw File_Exception(Font_Info_File_Path,"Invalid format.");
					i += std::size_t(count);
					if(data.font_largest_y_baseline_offset < info.y_baseline_offset) data.font_largest_y_baseline_offset = info.y_baseline_offset;
					data.font_character_infos.push_back(info);
				}
				for(std::size_t j = i;j < text.size();j += 1) {
					if(text[j] == '\n') {
						i = j;
						goto end_of_loop;
					}
				}
				i = text.size() - 1;
			end_of_loop:;
			}
		}
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

	void Renderer::begin(float delta_time,Vec3 color) {
		Renderer_Internal_Data& data = *std::launder(reinterpret_cast<Renderer_Internal_Data*>(data_buffer));
		if(platform->window_resized()) adjust_viewport();
		glClearColor(color.x,color.y,color.z,1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		data.time += delta_time;
		glUniform1f(data.time_uniform_location,data.time);
	}

	void Renderer::end() {
		Renderer_Internal_Data& data = *std::launder(reinterpret_cast<Renderer_Internal_Data*>(data_buffer));
		for(auto& sprite : data.sprites) {
			if(!sprite.has_value || sprite.current_object_data_index == 0) continue;
			glBindBufferBase(GL_UNIFORM_BUFFER,0,sprite.scene_data_uniform_buffer);
			glBufferSubData(GL_UNIFORM_BUFFER,0,sprite.current_object_data_index * sizeof(Object_Data),sprite.object_datas.data());
			glBindTexture(GL_TEXTURE_2D_ARRAY,sprite.texture_id);
			glDrawArraysInstanced(GL_TRIANGLES,0,6,GLsizei(sprite.current_object_data_index));
			sprite.current_object_data_index = 0;
		}
	}

	void Renderer::draw_sprite(Vec3 position,Vec2 size,float rotation,const Sprite_Index& sprite_index,std::uint32_t sprite_layer_index) {
		draw_sprite(position,size,rotation,{1,1,1,1},false,sprite_index,sprite_layer_index);
	}

	void Renderer::draw_sprite(Vec3 position,Vec2 size,float rotation,Vec4 color,bool rainbow_effect,const Sprite_Index& sprite_index,std::uint32_t sprite_layer_index) {
		Renderer_Internal_Data& data = *std::launder(reinterpret_cast<Renderer_Internal_Data*>(data_buffer));
#if defined(DEBUG_BUILD)
		if(sprite_index.index >= data.sprites.size()) {
			std::cerr << "[Rendering] Invalid sprite index (index: " << sprite_index.index << ", generation: " << sprite_index.generation << ")." << std::endl;
			return;
		}
		Sprite& sprite = data.sprites[sprite_index.index];
		if(sprite.generation != sprite_index.generation || !sprite.has_value) {
			std::cerr << "[Rendering] Invalid sprite index (index: " << sprite_index.index << ", generation: " << sprite_index.generation << ")." << std::endl;
			return;
		}
		if(sprite_layer_index >= sprite.array_layers) {
			std::cerr << "[Rendering] Invalid sprite tile index (" << sprite_layer_index << ")." << std::endl;
			return;
		}
#else
		if(sprite_index.index >= data.sprites.size()) throw Runtime_Exception("Invalid sprite index (out of bounds).");
		Sprite& sprite = data.sprites[sprite_index.index];
		if(sprite.generation != sprite_index.generation || !sprite.has_value) throw Runtime_Exception("Invalid sprite index (outdated).");
		if(sprite_layer_index >= sprite.array_layers) throw Runtime_Exception("Invalid sprite tile index.");
#endif

		//We render sprites by putting their transformation and texture data inside an uniform buffer designated for the texture we use in rendering.
		//When that buffers is full or at the end of a frame, we render all of the sprites that use a particular texture at once using instancing.
		if((sprite.current_object_data_index + 1) >= sprite.object_datas.size()) {
			glBindBufferBase(GL_UNIFORM_BUFFER,0,sprite.scene_data_uniform_buffer);
			glBufferSubData(GL_UNIFORM_BUFFER,0,sprite.current_object_data_index * sizeof(Object_Data),sprite.object_datas.data());
			glBindTexture(GL_TEXTURE_2D_ARRAY,sprite.texture_id);
			glDrawArraysInstanced(GL_TRIANGLES,0,6,GLsizei(sprite.current_object_data_index));
			sprite.current_object_data_index = 0;
		}

		auto& object_data = sprite.object_datas[sprite.current_object_data_index];
		object_data.matrix = core::translate(position.x,position.y,position.z) * core::rotate(rotation) * core::scale(size.x,size.y,1);
		object_data.texture_index = sprite_layer_index;
		object_data.multiply_color = color;
		object_data.effect_id = std::uint32_t(rainbow_effect);
		sprite.current_object_data_index += 1;
	}

	void Renderer::draw_text(Vec3 position,Vec2 char_size,Vec3 color,const char* text) {
		Renderer_Internal_Data& data = *std::launder(reinterpret_cast<Renderer_Internal_Data*>(data_buffer));
		float layer_size = float(data.sprites[data.font_sprite.index].layer_size);
		float font_largest_y_baseline_offset = (float(data.font_largest_y_baseline_offset) / layer_size) * char_size.y;

		Vec3 cur_pos = position;
		for(;*text;text += 1) {
			char c = *text;
			if(c == '\n') {
				cur_pos.x = position.x;
				cur_pos.y += char_size.y + font_largest_y_baseline_offset;
				continue;
			}
			auto& info = data.font_character_infos[std::size_t(c)];
			float y_baseline_offset = (float(info.y_baseline_offset) / layer_size) * char_size.y;

			Vec3 new_pos = cur_pos;
			new_pos.x -= (float(info.x_offset) / layer_size) * char_size.x;
			new_pos.y += y_baseline_offset;
			draw_sprite(new_pos,char_size,0.0f,{color.x,color.y,color.z,1.0f},false,data.font_sprite,std::uint32_t(c));
			cur_pos.x += (float(info.x_advance) / layer_size) * char_size.x + 1.0f / layer_size;
		}
	}

	Rect Renderer::compute_text_dims(Vec3 position,Vec2 char_size,const char* text) {
		Renderer_Internal_Data& data = *std::launder(reinterpret_cast<Renderer_Internal_Data*>(data_buffer));
		float layer_size = float(data.sprites[data.font_sprite.index].layer_size);
		float layer_size_reci = 1.0f / layer_size;
		float font_largest_y_baseline_offset = (float(data.font_largest_y_baseline_offset) / layer_size) * char_size.y;

		Rect dims = {position.x,position.y,0,char_size.y};
		float current_x = position.x;
		std::uint32_t newline_count = 0;
		float potential_height_increment = 0.0f;
		for(;*text;text += 1) {
			char c = *text;
			if(c == '\n') {
				newline_count += 1;
				current_x = position.x;
				dims.height = newline_count * (char_size.y + font_largest_y_baseline_offset) + char_size.y;
				potential_height_increment = 0.0f;
				continue;
			}
			auto& info = data.font_character_infos[std::size_t(c)];

			current_x += (float(info.x_advance) / layer_size) * char_size.x + layer_size_reci;
			float diff = current_x - position.x;
			if(diff > dims.width) dims.width = diff;

			float y_baseline_offset = (float(info.y_baseline_offset) / layer_size) * char_size.y;
			if(potential_height_increment < y_baseline_offset) potential_height_increment = y_baseline_offset;
		}
		dims.height += potential_height_increment;
		return dims;
	}

	[[nodiscard]] static std::vector<std::uint8_t> load_bitmap_from_file(const char* file_path,std::uint32_t* out_width,std::uint32_t* out_height) {
		static char static_file_path[2048];
		std::strcpy(static_file_path,file_path);

		auto file = std::ifstream(file_path,std::ios::binary);
		if(!file.is_open()) throw File_Open_Exception(static_file_path);

		auto read = [&]<typename T>() {
			T value = T();
			if(!file.read(reinterpret_cast<char*>(&value),sizeof(value))) throw File_Read_Exception(static_file_path,sizeof(value));
			return value;
		};

		char magic_bytes[2] = {};
		if(!file.read(magic_bytes,2)) throw File_Read_Exception(static_file_path,2);
		if(magic_bytes[0] != 'B' || magic_bytes[1] != 'M') throw File_Exception(static_file_path,"Invalid magic bytes at the beginning");

		[[maybe_unused]] auto bitmap_file_size = read.operator()<std::uint32_t>();
		[[maybe_unused]] auto reserved_data = read.operator()<std::uint32_t>();
		auto pixel_data_offset = read.operator()<std::uint32_t>();

		auto info_header_size = read.operator()<std::uint32_t>();
		if(info_header_size != 124) throw File_Exception(static_file_path,"Only BITMAPV5HEADER is supported");

		auto width = read.operator()<std::int32_t>();
		auto height = read.operator()<std::int32_t>();
		if(width <= 0) throw Runtime_Exception("Width must be > 0");
		if(height <= 0) throw Runtime_Exception("Height must be > 0");

		auto plane_count = read.operator()<std::uint16_t>();
		if(plane_count != 1) throw File_Exception(static_file_path,"Plane count must be 1");

		auto bit_count = read.operator()<std::uint16_t>();
		if(bit_count != 32) throw File_Exception(static_file_path,"Bit count must be 32");

		auto compression_method_index = read.operator()<std::uint32_t>();
		if(compression_method_index != 3) throw File_Exception(static_file_path,"Compression must be BI_BITFIELDS");

		[[maybe_unused]] auto image_size = read.operator()<std::uint32_t>();
		[[maybe_unused]] auto hor_resolution = read.operator()<std::int32_t>();
		[[maybe_unused]] auto ver_resolution = read.operator()<std::int32_t>();
		[[maybe_unused]] auto palatte_used_color_count = read.operator()<std::uint32_t>();
		[[maybe_unused]] auto palatte_important_color_count = read.operator()<std::uint32_t>();

		auto red_mask = read.operator()<std::uint32_t>();
		auto green_mask = read.operator()<std::uint32_t>();
		auto blue_mask = read.operator()<std::uint32_t>();
		auto alpha_mask = read.operator()<std::uint32_t>();

		//BITMAPV5HEADER has more fields, but we are ignoring them for simplicity.
		if(!file.seekg(pixel_data_offset,std::ios::beg)) throw File_Seek_Exception(static_file_path,pixel_data_offset);

		std::vector<std::uint8_t> pixels{};
		pixels.resize(std::size_t(width) * height * 4);
		//BMP files are stored fliped around the X axis so we need to read it backwards.
		for(std::uint32_t y = 0;y < std::uint32_t(height);y += 1)
			if(!file.read(reinterpret_cast<char*>(&pixels[(std::size_t(height) - y - 1) * width * 4]),std::size_t(width) * 4))
				throw File_Read_Exception(static_file_path,std::size_t(width) * 4);

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

	Sprite_Index Renderer::sprite(const char* file_path) {
		Renderer_Internal_Data& data = *std::launder(reinterpret_cast<Renderer_Internal_Data*>(data_buffer));

		std::uint32_t width = 0;
		std::uint32_t height = 0;
		std::vector<std::uint8_t> pixels = core::load_bitmap_from_file(file_path,&width,&height);
#if defined(DEBUG_BUILD)
		std::cout << "[Rendering] Loading an image from file \"" << file_path << "\" (width: " << width << ", height: " << height << ")." << std::endl;
#endif

		//We use 'GL_TEXTURE_2D_ARRAY' instead of 'GL_TEXTURE_2D' to simplify shaders.
		GLuint texture_id = 0;
		glGenTextures(1,&texture_id);
		glBindTexture(GL_TEXTURE_2D_ARRAY,texture_id);
		glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_S,GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_T,GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexImage3D(GL_TEXTURE_2D_ARRAY,0,GL_RGBA,width,height,1,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());

		//Each texture has its own uniform buffer for storing data related to quads rendered with the texture.
		GLuint uniform_buffer_id = 0;
		glGenBuffers(1,&uniform_buffer_id);
		glBindBuffer(GL_UNIFORM_BUFFER,uniform_buffer_id);
		glBufferData(GL_UNIFORM_BUFFER,data.object_data_uniform_buffer_size,nullptr,GL_DYNAMIC_DRAW);

		try {
			GLint64 actual_size = 0;
			glGetBufferParameteri64v(GL_UNIFORM_BUFFER,GL_BUFFER_SIZE,&actual_size);
			if(data.object_data_uniform_buffer_size != std::size_t(actual_size)) throw Runtime_Exception("Couldn't allocate an uniform buffer.");
			return insert_sprite(texture_id,uniform_buffer_id,1,0);
		}
		catch(...) {
			glDeleteBuffers(1,&uniform_buffer_id);
			glDeleteTextures(1,&texture_id);
			throw;
		}
	}

	Sprite_Index Renderer::sprite_atlas(const char* file_path,std::uint32_t tile_dimension) {
		Renderer_Internal_Data& data = *std::launder(reinterpret_cast<Renderer_Internal_Data*>(data_buffer));

		std::uint32_t width = 0;
		std::uint32_t height = 0;
		std::vector<std::uint8_t> pixels = core::load_bitmap_from_file(file_path,&width,&height);
		if((width % tile_dimension) != 0 || (height % tile_dimension) != 0) throw Runtime_Exception("Invalid sprite atlas tile size.");
#if defined(DEBUG_BUILD)
		std::cout << "[Rendering] Loading an image from file \"" << file_path << "\" (width: " << width << ", height: " << height << ")." << std::endl;
#endif

		GLuint texture_id = 0;
		glGenTextures(1,&texture_id);
		glBindTexture(GL_TEXTURE_2D_ARRAY,texture_id);
		glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_S,GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_T,GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,GL_NEAREST);

		std::uint32_t tile_count_x = width / tile_dimension;
		std::uint32_t tile_count_y = height / tile_dimension;
		glTexImage3D(GL_TEXTURE_2D_ARRAY,0,GL_RGBA,tile_dimension,tile_dimension,tile_count_x * tile_count_y,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);

		GLuint uniform_buffer_id = 0;
		try {
			std::vector<std::uint8_t> tmp_pixels = {};
			tmp_pixels.resize(std::size_t(tile_dimension) * tile_dimension * 4);

			//The code below extracts sprites from an atlas and each sprite is put as a seperate array layer.
			for(std::uint32_t base_y = 0;base_y < height;base_y += tile_dimension) {
				for(std::uint32_t x = 0;x < width;x += tile_dimension) {
					std::uint32_t offset = 0;
					for(std::uint32_t y = 0;y < tile_dimension;y += 1) {
						std::memcpy(&tmp_pixels[offset],&pixels[((std::size_t(base_y) + y) * width + x) * 4],std::size_t(tile_dimension) * 4);
						offset += tile_dimension * 4;
					}
					std::uint32_t index = (base_y / tile_dimension) * tile_count_x + (x / tile_dimension);
					glTexSubImage3D(GL_TEXTURE_2D_ARRAY,0,0,0,index,tile_dimension,tile_dimension,1,GL_RGBA,GL_UNSIGNED_BYTE,tmp_pixels.data());
				}
			}

			glGenBuffers(1,&uniform_buffer_id);
			glBindBuffer(GL_UNIFORM_BUFFER,uniform_buffer_id);
			glBufferData(GL_UNIFORM_BUFFER,data.object_data_uniform_buffer_size,nullptr,GL_DYNAMIC_DRAW);

			GLint64 actual_size = 0;
			glGetBufferParameteri64v(GL_UNIFORM_BUFFER,GL_BUFFER_SIZE,&actual_size);
			if(data.object_data_uniform_buffer_size != std::size_t(actual_size)) throw Runtime_Exception("Couldn't allocate an uniform buffer.");
			return insert_sprite(texture_id,uniform_buffer_id,tile_count_x * tile_count_y,tile_dimension);
		}
		catch(...) {
			if(glIsBuffer(uniform_buffer_id)) glDeleteBuffers(1,&uniform_buffer_id);
			glDeleteTextures(1,&texture_id);
			throw;
		}
	}

	Sprite_Index Renderer::insert_sprite(unsigned int texture_id,unsigned int uniform_buffer_id,std::uint32_t array_layers,std::uint32_t layer_size) {
		Renderer_Internal_Data& data = *std::launder(reinterpret_cast<Renderer_Internal_Data*>(data_buffer));

		for(std::size_t i = 0;i < data.sprites.size();i += 1) {
			auto& sprite = data.sprites[i];
			if(!sprite.has_value) {
				sprite.has_value = true;
				sprite.generation += 1;
				sprite.texture_id = texture_id;
				sprite.scene_data_uniform_buffer = uniform_buffer_id;
				sprite.array_layers = array_layers;
				sprite.layer_size = layer_size;
				sprite.current_object_data_index = 0;
				return {i,sprite.generation};
			}
		}

		Sprite sprite{};
		sprite.has_value = true;
		sprite.generation = 1;
		sprite.texture_id = texture_id;
		sprite.scene_data_uniform_buffer = uniform_buffer_id;
		sprite.array_layers = array_layers;
		sprite.layer_size = layer_size;
		sprite.current_object_data_index = 0;
		sprite.object_datas.resize(data.object_data_uniform_buffer_size / sizeof(Object_Data));
		data.sprites.push_back(std::move(sprite));
		return {data.sprites.size() - 1,1};
	}

	void Renderer::adjust_viewport() {
		Renderer_Internal_Data& data = *std::launder(reinterpret_cast<Renderer_Internal_Data*>(data_buffer));

		//Since the game is always in 4/3 aspect ratio, this piece of code calculates an appropriate offset from either the left side or the top side depending on current window resolution.
		//There will be black (or any other color being used as a clear color) horizontal or vertical bars depending on the aspect ratio of the window.
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
		data.render_rect = {offset_x,offset_y,dims.width,dims.height};
		glViewport(offset_x,offset_y,dims.width,dims.height);

		Mat4 matrix = core::orthographic(0,float(Background_Tile_Count_X),0,float(Background_Tile_Count_Y),-1,1);
		glUniformMatrix4fv(glGetUniformLocation(data.shader_program,"projection_matrix"),1,GL_FALSE,&matrix(0,0));
	}

	Irect Renderer::render_client_rect_dimensions() const noexcept {
		const Renderer_Internal_Data& data = *std::launder(reinterpret_cast<const Renderer_Internal_Data*>(data_buffer));
		return data.render_rect;
	}
}
