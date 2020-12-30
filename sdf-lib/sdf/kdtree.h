#pragma once
#include "pch.h"
#include "bbox.h"
#include "mesh.h"

namespace sdf {

struct kd_node {
	float split;
	int flags;

	void init_leaf(int *pnums, int pcount, std::vector<int> &pindices);
	void init_interior(int axis, int above_child, float split);
};

#define kd_isleaf(node) ((node.flags & 3) == 3)

#define kd_split(node) node.split
#define kd_split_axis(node) (node.flags & 3)
#define kd_above_child(node) (node.flags >> 2)

#define kd_pcount(node) (node.flags >> 2)
#define kd_poffset(node) glm::floatBitsToInt(node.split)

struct bound_edge;

struct kd_builder {
	kd_builder(
		sdf::mesh mesh,
		int isect_cost,
		int traversal_cost,
		float empty_bonus,
		int max_primitives,
		int max_depth
	);

	const int isect_cost, traversal_cost, max_primitives, max_depth;
	const float empty_bonus;
	
	const bbox bounds;

	kd_node* nodes;
	std::vector<int> pind;

	void build(
		int node_num,
		const bbox &bounds,
		bbox *pbounds,
		int *pnums,
		int pcount,
		int depth,
		bound_edge* edges[3], 
		int *prims0,
		int *prims1,
		int bad_refines
	);
	
	int next_free_node, allocated_nodes;
};

}
