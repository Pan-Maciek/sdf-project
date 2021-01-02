#pragma once
#include "pch.h"
#include<vector>
namespace sdf {

struct bbox {
	glm::vec3 min, max;

	bbox() { min = glm::vec3(INT_MAX);  max = glm::vec3(INT_MIN); };
	bbox(const std::vector<glm::vec3> &verticies);
	bbox(glm::vec3 *begin, glm::vec3 *end);
	bbox(const std::vector<glm::vec3> &verticies, const glm::uvec3& primitive);
	bbox(const glm::vec3 *verticies, const glm::uvec3& primitive);
	bbox(const glm::vec3 v1, const glm::vec3 v2, const glm::vec3 v3);

	float area() const;
	int max_extent() const;
	bbox opU(bbox b2) const;
	bbox opU(glm::vec3 p2) const;
	glm::vec3 offset(glm::vec3 p) const;
};

struct bbox4 {
	glm::vec4 dim;
	glm::vec4 center;
	bbox4() {};
	bbox4(bbox b) { dim = glm::vec4(b.max - b.min, 0.); center = glm::vec4(b.min, 0.) + dim * 0.5f; };
};

}
