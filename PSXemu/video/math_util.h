#pragma once
#include <glm/glm.hpp>

static int edge(const glm::ivec2& a, const glm::ivec2& b, const glm::ivec2& c) {
	return ((b.x - a.x) * (c.y - a.y)) - ((b.y - a.y) * (c.x - a.x));
}

static bool is_top_left(const glm::ivec2& a, const glm::ivec2& b) {
	return (b.y == a.y && b.x > a.x) || b.y < a.y;
}

static int double_area(const glm::ivec2& v0, const glm::ivec2& v1, const glm::ivec2& v2) {
	auto e0 = (v1.x - v0.x) * (v1.y + v0.y);
	auto e1 = (v2.x - v1.x) * (v2.y + v1.y);
	auto e2 = (v0.x - v2.x) * (v0.y + v2.y);

	return e0 + e1 + e2;
}

static int min3(int a, int b, int c) {
	if (a <= b && a <= c) return a;
	if (b <= a && b <= c) return b;
	return c;
}

static int max3(int a, int b, int c) {
	if (a >= b && a >= c) return a;
	if (b >= a && b >= c) return b;
	return c;
}