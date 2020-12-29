#pragma once
#include "pch.h"

namespace sdf {

struct mesh {
	glm::vec3 *vertices;
	glm::uvec3 *primitives;

	int vertex_count, primitive_count;
};

mesh load_ply(const std::string path);

}
