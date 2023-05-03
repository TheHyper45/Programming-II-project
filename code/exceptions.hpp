#ifndef EXCEPTIONS_HPP
#define EXCEPTIONS_HPP

namespace core {
	class Runtime_Exception {
	public:
		explicit Runtime_Exception(const char* _ptr) noexcept : ptr(_ptr) {}
		[[nodiscard]] const char* message() const noexcept { return ptr; }
	private:
		const char* ptr;
	};

	class File_Open_Exception {
	public:
		explicit File_Open_Exception(const char* _file_path_ptr) noexcept : file_path_ptr(_file_path_ptr) {}
		[[nodiscard]] const char* file_path() const noexcept { return file_path_ptr; }
	private:
		const char* file_path_ptr;
	};

	class File_Read_Exception {
	public:
		File_Read_Exception(const char* _file_path_ptr,std::size_t _data_byte_count) noexcept : file_path_ptr(_file_path_ptr),data_byte_count(_data_byte_count) {}
		[[nodiscard]] const char* file_path() const noexcept { return file_path_ptr; }
		[[nodiscard]] std::size_t byte_count() const noexcept { return data_byte_count; }
	private:
		const char* file_path_ptr;
		std::size_t data_byte_count;
	};

	class File_Seek_Exception {
	public:
		File_Seek_Exception(const char* _file_path_ptr,std::size_t _data_offset) noexcept : file_path_ptr(_file_path_ptr),data_offset(_data_offset) {}
		[[nodiscard]] const char* file_path() const noexcept { return file_path_ptr; }
		[[nodiscard]] std::size_t byte_offset() const noexcept { return data_offset; }
	private:
		const char* file_path_ptr;
		std::size_t data_offset;
	};

	class File_Exception {
	public:
		File_Exception(const char* _file_path_ptr,const char* _message_string_ptr) noexcept : file_path_ptr(_file_path_ptr),message_string_ptr(_message_string_ptr) {}
		[[nodiscard]] const char* file_path() const noexcept { return file_path_ptr; }
		[[nodiscard]] const char* message() const noexcept { return message_string_ptr; }
	private:
		const char* file_path_ptr;
		const char* message_string_ptr;
	};

	class Object_Creation_Exception {
	public:
		explicit Object_Creation_Exception(const char* _object_type_string) noexcept : object_type_string(_object_type_string) {}
		[[nodiscard]] const char* object_type() const noexcept { return object_type_string; }
	private:
		const char* object_type_string;
	};
}

#endif
