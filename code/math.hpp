#ifndef MATH_HPP
#define MATH_HPP

#include <cstdint>
#include <cstddef>

namespace core {
	static inline constexpr float PI = 3.1415927f;
	struct Vec2 {
		float x;
		float y;

		[[nodiscard]] Vec2 operator+(Vec2 v) {
			return {x + v.x,y + v.y};
		}
		[[nodiscard]] Vec2 operator-(Vec2 v) {
			return {x - v.x,y - v.y};
		}
		[[nodiscard]] Vec2 operator*(float v) {
			return {x * v,y * v};
		}
		[[nodiscard]] Vec2 operator/(float v) {
			return {x / v,y / v};
		}

		Vec2& operator+=(const Vec2& v) {
			x += v.x;
			y += v.y;
			return *this;
		}
		Vec2& operator-=(const Vec2& v) {
			x -= v.x;
			y -= v.y;
			return *this;
		}
	};
	struct Vec3 {
		float x;
		float y;
		float z;
		[[nodiscard]] explicit operator Vec2() {
			return { x,y };
		}
	};
	struct Vec4 {
		float x;
		float y;
		float z;
		float w;
	};
	struct Rect {
		float x;
		float y;
		float width;
		float height;
		[[nodiscard]] bool point_inside(Vec2 point) const noexcept {
			return point.x >= x && point.x <= (x + width) && point.y >= y && point.y <= (y + height);
		}
		[[nodiscard]] bool overlaps(const Rect& other) const noexcept {
			return (x + width) > other.x && x < (other.x + other.width) && (y + height) > other.y && y < (other.y + other.height);
		}
	};
	struct Point {
		std::uint32_t x;
		std::uint32_t y;
	};
	struct Ipoint {
		std::int32_t x;
		std::int32_t y;
	};
	struct Irect {
		std::int32_t x;
		std::int32_t y;
		std::int32_t width;
		std::int32_t height;
		[[nodiscard]] bool point_inside(Ipoint point) const noexcept {
			return point.x > x && point.x < (x + width) && point.y > y && point.y < (y + height);
		}
		[[nodiscard]] bool point_inside_inclusive(Ipoint point) const noexcept {
			return point.x >= x && point.x <= (x + width) && point.y >= y && point.y <= (y + height);
		}
	};
	struct Urect {
		std::uint32_t x;
		std::uint32_t y;
		std::uint32_t width;
		std::uint32_t height;
		[[nodiscard]] bool point_inside(Point point) const noexcept {
			return point.x > x && point.x < (x + width) && point.y > y && point.y < (y + height);
		}
		[[nodiscard]] bool point_inside_inclusive(Point point) const noexcept {
			return point.x >= x && point.x <= (x + width) && point.y >= y && point.y <= (y + height);
		}
	};
	struct Dimensions {
		std::uint32_t width;
		std::uint32_t height;
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
	[[nodiscard]] Mat4 rotate(float z) noexcept;
	[[nodiscard]] Mat4 scale(float x,float y,float z) noexcept;

	[[nodiscard]] float magnitude(Vec2 v);
	[[nodiscard]] Vec2 normalize(Vec2 v);
	[[nodiscard]] float dot(Vec2 a,Vec2 b);
	[[nodiscard]] float distance(Vec2 a,Vec2 b);
	[[nodiscard]] std::uint32_t leading_zeroes(std::uint32_t value);
}

#endif
