#include <new>
#include <cstdio>
#include <vector>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include "defer.hpp"
#include "opengl.hpp"
#include "renderer.hpp"
#include "platform.hpp"
#include "exceptions.hpp"

namespace core {
	static constexpr std::size_t Max_Simultaneous_Textures = 4;
	static char info_log_buffer[4096];

	struct Object_Data {
		alignas(16) Mat4 transformation;
		alignas(16) std::uint32_t texture_index;
	};

	struct Renderer_Internal_Data {
		GLuint shader_program;
		GLuint vertex_array;
		GLuint quad_buffer_id;
		std::size_t max_object_data_buffer_length;
		GLuint object_data_buffer_id;
		void* object_data_buffer_ptr;
		std::size_t object_data_buffer_current_index;
		std::size_t auto_texture_unit_index;
		GLuint bound_texture_ids[Max_Simultaneous_Textures];
		std::size_t bound_texture_ids_length;
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

	static constexpr const char Vertex_Shader_Source_Format[] = R"xxx(
		#version 420 core
		in vec3 position;
		in vec2 tex_coords;
		out vec2 out_tex_coords;
		out flat uint out_texture_index;
		uniform mat4 matrix;
		struct Object_Data {
			mat4 transformation;
			uint texture_index;
		};
		layout(std140) uniform Scene_Data {
			Object_Data[%zu] object_datas;
		};
		void main() {
			gl_Position = matrix * object_datas[gl_InstanceID].transformation * vec4(position,1.0);
			out_tex_coords = tex_coords;
			out_texture_index = object_datas[gl_InstanceID].texture_index;
		}
	)xxx";

	static constexpr const char Fragment_Shader_Source_Format[] = R"xxx(
		#version 420 core
		in vec2 out_tex_coords;
		in flat uint out_texture_index;
		out vec4 out_color;
		uniform sampler2D[%zu] texs;
		void main() {
			switch(out_texture_index) {
				%s
				default: out_color = vec4(1.0,1.0,1.0,1.0);
			}
		}
	)xxx";

	Renderer::Renderer(Platform* _platform) : platform(_platform),data_buffer() {
		static_assert(sizeof(Renderer_Internal_Data) <= sizeof(data_buffer));
		Renderer_Internal_Data& data = *new(data_buffer) Renderer_Internal_Data();

		{
			GLint64 desired_uniform_buffer_size = 65536;
			GLint64 max_uniform_buffer_size = 0;
			glGetInteger64v(GL_MAX_UNIFORM_BLOCK_SIZE,&max_uniform_buffer_size);
			if(max_uniform_buffer_size < desired_uniform_buffer_size) desired_uniform_buffer_size = max_uniform_buffer_size;
			data.max_object_data_buffer_length = desired_uniform_buffer_size / sizeof(Object_Data);
		}

		auto dims = platform->window_client_dimensions();
		glViewport(0,0,dims.width,dims.height);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_TRUE);
		glClearDepth(1.0f);
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
			char shader_source[4096] = {};
			int shader_source_byte_length = std::snprintf(shader_source,sizeof(shader_source) - 1,Vertex_Shader_Source_Format,data.max_object_data_buffer_length);
			if(shader_source_byte_length < 0) throw Runtime_Exception("Error during processing vertex shader string.");

			GLuint vertex_shader = create_shader(GL_VERTEX_SHADER,shader_source,std::size_t(shader_source_byte_length));
			defer[&]{glDeleteShader(vertex_shader);};

			char fragment_shader_switch_case_strings[4096] = {};
			std::size_t fragment_shader_switch_case_string_byte_length = 0;
			for(std::size_t i = 0;i < Max_Simultaneous_Textures;i += 1) {
				int count = std::snprintf(&fragment_shader_switch_case_strings[fragment_shader_switch_case_string_byte_length],
										  sizeof(fragment_shader_switch_case_strings) - 1 - fragment_shader_switch_case_string_byte_length,
										  "case %zu: out_color = texture(texs[%zu],out_tex_coords); break;\n",i,i);
				if(count < 0) throw Runtime_Exception("Error during processing fragment shader string.");
				fragment_shader_switch_case_string_byte_length += std::size_t(count);
			}

			shader_source_byte_length = std::snprintf(shader_source,sizeof(shader_source) - 1,Fragment_Shader_Source_Format,Max_Simultaneous_Textures,fragment_shader_switch_case_strings);
			if(shader_source_byte_length < 0) throw Runtime_Exception("Error during processing fragment shader string.");
			
			GLuint fragment_shader = create_shader(GL_FRAGMENT_SHADER,shader_source,std::size_t(shader_source_byte_length));
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
		GLint position_location = glGetAttribLocation(data.shader_program,"position");
		GLint tex_coords_location = glGetAttribLocation(data.shader_program,"tex_coords");
		
		{
			glGenVertexArrays(1,&data.vertex_array);
			glBindVertexArray(data.vertex_array);

			glGenBuffers(1,&data.quad_buffer_id);
			glBindBuffer(GL_ARRAY_BUFFER,data.quad_buffer_id);

			Vertex quad_data[] = {
				{{0,0,0},{0.0f,0.0f}},
				{{1,0,0},{1.0f,0.0f}},
				{{1,1,0},{1.0f,1.0f}},
				{{0,0,0},{0.0f,0.0f}},
				{{1,1,0},{1.0f,1.0f}},
				{{0,1,0},{0.0f,1.0f}},
			};
			glBufferData(GL_ARRAY_BUFFER,sizeof(quad_data),quad_data,GL_STATIC_DRAW);
			GLint64 actual_buffer_size = 0;
			glGetBufferParameteri64v(GL_ARRAY_BUFFER,GL_BUFFER_SIZE,&actual_buffer_size);
			if(actual_buffer_size != sizeof(quad_data)) throw Runtime_Exception("Couldn't allocate quad buffer memory.");

			glEnableVertexAttribArray(position_location);
			glVertexAttribPointer(position_location,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(0));

			glEnableVertexAttribArray(tex_coords_location);
			glVertexAttribPointer(tex_coords_location,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(3 * sizeof(float)));
		}
		{
			std::vector<GLint> sampler_ids{};
			sampler_ids.resize(Max_Simultaneous_Textures);
			for(std::size_t i = 0;i < Max_Simultaneous_Textures;i += 1) {
				glActiveTexture(GL_TEXTURE0 + i);
				sampler_ids[i] = i;
			}
			glUniform1iv(glGetUniformLocation(data.shader_program,"texs"),Max_Simultaneous_Textures,sampler_ids.data());
		}
		
		Mat4 matrix = core::orthographic(0,dims.width,0,dims.height,-1,1);
		glUniformMatrix4fv(glGetUniformLocation(data.shader_program,"matrix"),1,GL_FALSE,&matrix(0,0));

		{
			glGenBuffers(1,&data.object_data_buffer_id);
			glBindBuffer(GL_UNIFORM_BUFFER,data.object_data_buffer_id);
			std::size_t desired_buffer_size = data.max_object_data_buffer_length * sizeof(Object_Data);
			glBufferData(GL_UNIFORM_BUFFER,desired_buffer_size,nullptr,GL_DYNAMIC_DRAW);

			GLint64 actual_buffer_size = 0;
			glGetBufferParameteri64v(GL_UNIFORM_BUFFER,GL_BUFFER_SIZE,&actual_buffer_size);
			if(actual_buffer_size != desired_buffer_size) throw Runtime_Exception("Couldn't allocate scene data buffer memory.");

			GLint scene_data_uniform_index = glGetUniformBlockIndex(data.shader_program,"Scene_Data");
			if(scene_data_uniform_index == GL_INVALID_INDEX) throw Runtime_Exception("There is no 'Scene_Data' uniform variable in the shader program.");

			glUniformBlockBinding(data.shader_program,scene_data_uniform_index,0);
			glBindBufferBase(GL_UNIFORM_BUFFER,0,data.object_data_buffer_id);
		}
	};

	Renderer::~Renderer() {
		Renderer_Internal_Data& data = *std::launder(reinterpret_cast<Renderer_Internal_Data*>(data_buffer));
		glDeleteBuffers(1,&data.object_data_buffer_id);
		glDeleteBuffers(1,&data.quad_buffer_id);
		glDeleteVertexArrays(1,&data.vertex_array);
		glDeleteProgram(data.shader_program);
	}

	void Renderer::begin(float r,float g,float b) {
		Renderer_Internal_Data& data = *std::launder(reinterpret_cast<Renderer_Internal_Data*>(data_buffer));

		if(platform->window_resized()) {
			auto dims = platform->window_client_dimensions();
			glViewport(0,0,dims.width,dims.height);
			Mat4 matrix = core::orthographic(0,dims.width,0,dims.height,-1,1);
			glUniformMatrix4fv(glGetUniformLocation(data.shader_program,"matrix"),1,GL_FALSE,&matrix(0,0));
		}

		glClearColor(r,g,b,1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		data.bound_texture_ids_length = 0;
		data.object_data_buffer_current_index = 0;
		data.object_data_buffer_ptr = glMapBuffer(GL_UNIFORM_BUFFER,GL_WRITE_ONLY);
		if(!data.object_data_buffer_ptr) throw Runtime_Exception("Couldn't map uniform buffer memory.");
	}

	void Renderer::end() {
		Renderer_Internal_Data& data = *std::launder(reinterpret_cast<Renderer_Internal_Data*>(data_buffer));

		glUnmapBuffer(GL_UNIFORM_BUFFER);
		glDrawArraysInstanced(GL_TRIANGLES,0,6,data.object_data_buffer_current_index);
	}

	void Renderer::draw_quad(Vec3 position,Vec2 size,const Texture& texture) {
		Renderer_Internal_Data& data = *std::launder(reinterpret_cast<Renderer_Internal_Data*>(data_buffer));

		if((data.object_data_buffer_current_index + 1) >= data.max_object_data_buffer_length) {
			glUnmapBuffer(GL_UNIFORM_BUFFER);
			glDrawArraysInstanced(GL_TRIANGLES,0,6,data.object_data_buffer_current_index);
			data.bound_texture_ids_length = 0;
			data.object_data_buffer_current_index = 0;
			data.object_data_buffer_ptr = glMapBuffer(GL_UNIFORM_BUFFER,GL_WRITE_ONLY);
			if(!data.object_data_buffer_ptr) throw Runtime_Exception("Couldn't map uniform buffer memory.");
		}

		std::uint32_t bound_id = 0;
		bool is_bound = false;
		for(std::size_t i = 0;i < data.bound_texture_ids_length;i += 1) {
			if(data.bound_texture_ids[i] == texture.texture_id) {
				is_bound = true;
				bound_id = i;
				break;
			}
		}
		if(!is_bound) {
			if((data.bound_texture_ids_length + 1) >= Max_Simultaneous_Textures) throw Runtime_Exception("Too many textures.");
			bound_id = data.bound_texture_ids_length;
			glActiveTexture(GL_TEXTURE0 + bound_id);
			data.bound_texture_ids[bound_id] = texture.texture_id;
			glBindTexture(GL_TEXTURE_2D,texture.texture_id);
			data.bound_texture_ids_length += 1;
		}

		Object_Data object_data = {};
		object_data.transformation = core::translate(position.x,position.y,position.z) * core::scale(size.x,size.y,1);
		object_data.texture_index = bound_id;
		std::memcpy(static_cast<char*>(data.object_data_buffer_ptr) + data.object_data_buffer_current_index * sizeof(Object_Data),&object_data,sizeof(Object_Data));
		data.object_data_buffer_current_index += 1;
	}

	Texture Renderer::create_texture(std::uint32_t width,std::uint32_t height,const std::uint8_t* pixels) {
		return Texture(this,width,height,pixels);
	}
}
