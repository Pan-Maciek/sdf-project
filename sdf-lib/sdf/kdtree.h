#pragma once
#include "pch.h"

namespace sdf {

struct kd_node {
	float split;
	int flags;

	void init_leaf(int *pnums, int pcount, std::vector<int> &pindices);
	void init_interior(int axis, int above_child, float split);
};

#define kd_leaf 3
#define kd_isleaf(node) ((node.flags & 3) == 3)

#define kd_split(node) node.split
#define kd_split_axis(node) (node.flags & 3)
#define kd_above_child(node) (node.flags >> 2)

#define kd_pcount(node) (node.flags >> 2)
#define kd_poffset(node) glm::floatBitsToInt(node.split)

}
