#ifndef PLATFORM_HPP
#define PLATFORM_HPP

#include <cstddef>
#include <cstdint>

namespace core {
	struct Dimensions {
		std::uint32_t width;
		std::uint32_t height;
	};

	class Platform {
	public:
		Platform() noexcept;
		Platform(const Platform&) = delete;
		Platform& operator=(const Platform&) = delete;
		~Platform();
		void create_main_window(const char* title,std::uint32_t width,std::uint32_t height);
		[[nodiscard]] bool window_closed() noexcept;
		[[nodiscard]] bool window_resized() noexcept;
		[[nodiscard]] Dimensions window_client_dimensions() noexcept;
		void process_events();
		void swap_window_buffers();
		void error_message_box(const char* title);
	private:
		alignas(std::max_align_t) unsigned char data_buffer[64];
	};
}

#endif
