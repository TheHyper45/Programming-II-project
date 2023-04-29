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
}

#endif
