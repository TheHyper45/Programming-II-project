#include <cmath>
#include "math.hpp"

#if defined(_MSC_VER) && !defined(__clang__)
	#include <intrin.h>
#endif

namespace core {
	Mat4::Mat4(float v) noexcept : data() {
		operator()(0,0) = v;
		operator()(1,1) = v;
		operator()(2,2) = v;
		operator()(3,3) = v;
	}

	float& Mat4::operator()(std::size_t x,std::size_t y) noexcept {
		return data[x * 4 + y];
	}

	const float& Mat4::operator()(std::size_t x,std::size_t y) const noexcept {
		return data[x * 4 + y];
	}

	Mat4 operator*(const Mat4& a,const Mat4& b) noexcept {
		Mat4 result{};
		for(std::size_t x = 0;x < 4;x += 1) {
			for(std::size_t y = 0;y < 4;y += 1) {
				result(x,y) = a(0,y) * b(x,0) + a(1,y) * b(x,1) + a(2,y) * b(x,2) + a(3,y) * b(x,3);
			}
		}
		return result;
	}

	Mat4 orthographic(float left,float right,float top,float bottom,float near,float far) noexcept {
		Mat4 result{};
		result(0,0) = 2.0f / (right - left);
		result(3,0) = -(right + left) / (right - left);
		result(1,1) = 2.0f / (top - bottom);
		result(3,1) = -(top + bottom) / (top - bottom);
		result(2,2) = -2.0f / (far - near);
		result(3,2) = -(far + near) / (far - near);
		result(3,3) = 1.0f;
		return result;
	}

	Mat4 translate(float x,float y,float z) noexcept {
		Mat4 result{1.0f};
		result(3,0) = x;
		result(3,1) = y;
		result(3,2) = z;
		return result;
	}

	Mat4 rotate(float z) noexcept {
		Mat4 result{};
		result(0,0) = std::cos(z);
		result(1,0) = -std::sin(z);
		result(0,1) = -result(1,0);
		result(1,1) = result(0,0);
		result(2,2) = 1.0f;
		result(3,3) = 1.0f;
		return result;
	}

	Mat4 scale(float x,float y,float z) noexcept {
		Mat4 result{};
		result(0,0) = x;
		result(1,1) = y;
		result(2,2) = z;
		result(3,3) = 1.0f;
		return result;
	}

	std::uint32_t leading_zeroes(std::uint32_t value) {
#if defined(_MSC_VER) && !defined(__clang__)
		unsigned long result = 0;
		_BitScanForward(&result,value);
		return result;
#else
		return __builtin_ctzl(value);
#endif
	}
}
