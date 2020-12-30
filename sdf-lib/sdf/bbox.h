#pragma once
#include "pch.h"
#include<vector>
namespace sdf {

struct bbox {
	glm::vec3 min, max;

	bbox() = default;
	bbox(const std::vector<glm::vec3> &verticies);
	bbox(glm::vec3 *begin, glm::vec3 *end);
	bbox(const std::vector<glm::vec3> &verticies, const glm::uvec3& primitive);
	bbox(const glm::vec3 *verticies, const glm::uvec3& primitive);
	bbox(const glm::vec3 v1, const glm::vec3 v2, const glm::vec3 v3);

	float area() const;
	int max_extent() const;
	bbox opU(bbox b2) const;
	bbox opU(vec3 p2) const;
};

}
