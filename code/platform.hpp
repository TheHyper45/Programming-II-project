#ifndef PLATFORM_HPP
#define PLATFORM_HPP

#include <cstddef>
#include <string_view>

namespace core {
	class Platform {
	public:
		Platform() noexcept;
		~Platform();
		void create_main_window(std::string_view title,std::size_t width,std::size_t height);
		[[nodiscard]] bool window_closed() noexcept;
		void process_events();
		void swap_buffers();
		void error_message_box(std::string_view title);
	private:
		alignas(std::max_align_t) unsigned char data_buffer[64];
	};
}

#endif
