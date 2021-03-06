#include "pch.h"
#include "bbox.h"

#define minmax_update(min, max, val) {if (val < min) min = val; else if (val > max) max = val;}

using namespace std;
using namespace glm;

namespace sdf {

bbox::bbox(const vector<vec3> &verticies) {
	min = max = verticies[0];
	for (int i = 0, size = verticies.size(); i < size; ++i) {
		minmax_update(min.x, max.x, verticies[i].x);
		minmax_update(min.y, max.y, verticies[i].y);
		minmax_update(min.z, max.z, verticies[i].z);
	}
}

bbox::bbox(vec3 *begin, vec3 *end) {
	min = max = *begin;
	for (vec3* it = begin + 1; it != end; ++it) {
		minmax_update(min.x, max.x, it->x);
		minmax_update(min.y, max.y, it->y);
		minmax_update(min.z, max.z, it->z);
	}
}

bbox::bbox(const std::vector<glm::vec3> &verticies, const glm::uvec3 &primitive) {
	min = max = verticies[primitive[0]];
	for (int i = 1; i < 3; ++i) {
		minmax_update(min.x, max.x, verticies[primitive[i]].x);
		minmax_update(min.y, max.y, verticies[primitive[i]].y);
		minmax_update(min.z, max.z, verticies[primitive[i]].z);
	}
}

bbox::bbox(const glm::vec3 *verticies, const glm::uvec3 &primitive) {
	min = max = verticies[primitive[0]];
	for (int i = 1; i < 3; ++i) {
		minmax_update(min.x, max.x, verticies[primitive[i]].x);
		minmax_update(min.y, max.y, verticies[primitive[i]].y);
		minmax_update(min.z, max.z, verticies[primitive[i]].z);
	}
}

bbox::bbox(const glm::vec3 v1, const glm::vec3 v2, const glm::vec3 v3) {
	min = max = v1;
	min = glm::min(min, v2);
	min = glm::min(min, v3);
	max = glm::max(max, v2);
	max = glm::max(max, v3);
}

float bbox::area() const {
	vec3 dt = max - min;
	return 2 * (dt.x * dt.y + dt.x * dt.z + dt.y * dt.z);
}

int bbox::max_extent() const {
	vec3 dt = max - min;
	if (dt.x > dt.y && dt.x > dt.z) return 0;
	else if (dt.y > dt.z) return 1;
	else return 2;
}

bbox bbox::opU(bbox b2) const {
	bbox b3;
	b3.min = glm::min(min, b2.min);
	b3.max = glm::max(max, b2.max);
	return b3;
}

bbox bbox::opU(vec3 p2) const {
	bbox b2;
	b2.min = glm::min(min, p2);
	b2.max = glm::max(max, p2);
	return b2;
}

vec3 bbox::offset(vec3 p) const {
	vec3 o = p - min;
	if (max.x > min.x) o.x /= max.x - min.x;
	if (max.y > min.y) o.y /= max.y - min.y;
	if (max.z > min.z) o.z /= max.z - min.z;
	return o;
}

}


