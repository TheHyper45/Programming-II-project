#ifndef EXCEPTIONS_HPP
#define EXCEPTIONS_HPP

namespace core {
	class Runtime_Exception {
	public:
		explicit Runtime_Exception(const char* _ptr) : ptr(_ptr) {}
		[[nodiscard]] const char* message() const noexcept { return ptr; }
	private:
		const char* ptr;
	};

	template<typename Type>
	class Tagged_Runtime_Exception {
	public:
		Tagged_Runtime_Exception(const char* _ptr,Type _object) : ptr(_ptr),object(_object) {}
		[[nodiscard]] const char* message() const noexcept { return ptr; }
		[[nodiscard]] Type value() const noexcept { return object; }
	private:
		const char* ptr;
		Type object;
	};
}

#endif
