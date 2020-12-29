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

bbox::bbox(const std::vector<glm::vec3> &verticies, const glm::uvec3 &primitive) {
	min = max = verticies[primitive[0]];
	for (int i = 1; i < 3; ++i) {
		minmax_update(min.x, max.x, verticies[primitive[i]].x);
		minmax_update(min.y, max.y, verticies[primitive[i]].y);
		minmax_update(min.z, max.z, verticies[primitive[i]].z);
	}
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

}
