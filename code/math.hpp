#ifndef MATH_HPP
#define MATH_HPP

#include <cstdint>
#include <cstddef>

namespace core {
	struct Vec2 {
		float x;
		float y;
	};
	struct Vec3 {
		float x;
		float y;
		float z;
	};
	struct Mat4 {
		float data[16];
		Mat4() = default;
		explicit Mat4(float v) noexcept;
		[[nodiscard]] float& operator()(std::size_t x,std::size_t y) noexcept;
		[[nodiscard]] const float& operator()(std::size_t x,std::size_t y) const noexcept;
	};
	[[nodiscard]] Mat4 operator*(const Mat4& a,const Mat4& b) noexcept;
	[[nodiscard]] Mat4 orthographic(float left,float right,float top,float bottom,float near,float far) noexcept;
	[[nodiscard]] Mat4 translate(float x,float y,float z) noexcept;
	[[nodiscard]] Mat4 scale(float x,float y,float z) noexcept;

	[[nodiscard]] std::uint32_t leading_zeroes(std::uint32_t value);
}

#endif
