#pragma once
#include "pch.h"

namespace sdf {

struct bbox {
	glm::vec3 min, max;

	bbox() = default;
	bbox(const std::vector<glm::vec3> &verticies);
	bbox(glm::vec3 *begin, glm::vec3 *end);
	bbox(const std::vector<glm::vec3> &verticies, const glm::uvec3& primitive);
	bbox(const glm::vec3 *verticies, const glm::uvec3& primitive);

	float area() const;
	int max_extent() const;
};

}
