#include "pch.h"
#include "kdtree.h"
#include "io.h"

using namespace std;
using namespace glm;

#define kd_leaf 3

namespace sdf {

enum class edge_type { start = true, end = false };
struct bound_edge {
	bound_edge() = default;
	bound_edge(float t, int pnum, edge_type type) : t(t), pnum(pnum), type(type) {}
	int pnum;
	float t;
	edge_type type;
};

void kd_node::init_leaf(int *pnums, int pcount, vector<int> &pindices) {
	flags = kd_leaf | (pcount << 2);
	switch (pcount) {
		case 0: split = 0; break;
		case 1: split = glm::intBitsToFloat(pnums[0]); break;
		default:
			split = glm::intBitsToFloat(pindices.size());
			for (int i = 0; i < pcount; ++i)
				pindices.push_back(pnums[i]);
	}
}

void kd_node::init_interior(int axis, int above_child, float split_value) {
	flags = axis | (above_child << 2);
	split = split_value;
}

kd_builder::kd_builder(
	int isect_cost,
	int traversal_cost,
	float empty_bonus,
	int max_prims,
	int max_depth
) : isect_cost(isect_cost), traversal_cost(traversal_cost),
	empty_bonus(empty_bonus), max_primitives(max_prims),
	max_depth(max_depth), allocated_nodes(0), next_free_node(0) { }

kd_acc kd_builder::build(sdf::mesh mesh) {
	bbox bounds(mesh.vertices, mesh.vertices + mesh.vertex_count);
	int depth = max_depth <= 0 ? std::round(1.3 * std::log2l(mesh.primitive_count)) : max_depth;
	next_free_node = allocated_nodes = 0;

	int pcount = mesh.primitive_count;
	// pre-compute bounds per primitive
	auto pbounds = make_unique<bbox[]>(pcount);
	for (int i = 0; i < pcount; ++i)
		pbounds[i] = bbox(mesh.vertices, mesh.primitives[i]);

	bound_edge* edges[3];
	for (int i = 0; i < 3; ++i)
		edges[i] = (bound_edge*) malloc(2 * pcount * sizeof(bound_edge));

	auto prims0 = make_unique<int[]>(pcount);
	auto prims1 = make_unique<int[]>((depth + 1) * pcount);

	auto pnums = make_unique<int[]>(pcount);
	for (int i = 0; i < pcount; ++i)
		pnums[i] = i;

	vector<int> pind;
	kd_node *nodes = nullptr;

	build(0, bounds, pbounds.get(), pnums.get(), pcount, depth, edges, prims0.get(), prims1.get(), nodes, pind, 0);
	for (int i = 0; i < 3; ++i)
		free(edges[i]);

	int *indices = new int[pind.size()];
	memcpy(indices, pind.data(), pind.size() * sizeof(int));

	return {mesh, next_free_node, (int) pind.size(), nodes, indices};
}

void kd_builder::build(
	int node_num, 
	const bbox &bounds, 
	bbox *pbounds, 
	int *pnums, 
	int pcount, 
	int depth, 
	bound_edge* edges[3], 
	int *prims0, 
	int *prims1, 
	kd_node* &nodes,
	vector<int> &pind, 
	int bad_refines
) {
	assert(node_num == next_free_node);
	if (next_free_node == allocated_nodes) {
		int new_allocated_nodes = std::max(2 * allocated_nodes, 512000);
		int to_allocate = new_allocated_nodes * sizeof(kd_node);
		kd_node *new_nodes = (kd_node*) malloc(to_allocate);
		if (allocated_nodes > 0) {
			memcpy(new_nodes, nodes, to_allocate);
			free(nodes);
		}
		nodes = new_nodes;
		allocated_nodes = new_allocated_nodes;
	}
	++next_free_node;;

	if (pcount <= max_primitives || depth == 0) {
		nodes[node_num].init_leaf(pnums, pcount, pind);
		return;
	}

	int best_axis = -1;
	int best_offset = -1;
	float best_cost = INFINITY;
	float old_cost = isect_cost * pcount;
	float total_sa = bounds.area();
	float inv_total_sa = 1 / total_sa;

	vec3 d = bounds.max - bounds.min;
	int axis = bounds.max_extent();
	int retries = 0;

retry_split:
	for (int i = 0; i < pcount; ++i) {
		int pn = pnums[i];
		const bbox &bounds = pbounds[pn];
		edges[axis][2 * i] = bound_edge(bounds.min[axis], pn, edge_type::start);
		edges[axis][2 * i + 1] = bound_edge(bounds.max[axis], pn, edge_type::end);
	}
	
	sort(&edges[axis][0], &edges[axis][2 * pcount], 
	[](const bound_edge &e0, const bound_edge &e1) -> bool {
		return e0.t == e1.t ? (int) e0.type < (int) e1.type : e0.t < e1.t;
	});

	int n_below = 0, n_above = pcount;
	for (int i = 0; i < 2 * pcount; ++i) {
		if (edges[axis][i].type == edge_type::end) --n_above;
		float edge_t = edges[axis][i].t;
		if (bounds.min[axis] < edge_t  && edge_t < bounds.max[axis]) {
			int otherAxis0 = (axis + 1) % 3, otherAxis1 = (axis + 2) % 3;
			float below_sa = 2 * (d[otherAxis0] * d[otherAxis1] + (edge_t - bounds.min[axis]) * (d[otherAxis0] + d[otherAxis1]));
			float above_sa = 2 * (d[otherAxis0] * d[otherAxis1] + (bounds.max[axis] - edge_t) * (d[otherAxis0] + d[otherAxis1]));
			float pbelow = below_sa * inv_total_sa;
			float pabove = above_sa * inv_total_sa;

			float eb = (n_above == 0 || n_below == 0) ? empty_bonus : 0;
			float cost = traversal_cost + isect_cost * (1 - eb) * (pbelow * n_below + pabove * n_above);

			if (cost < best_cost) {
				best_cost = cost;
				best_axis = axis;
				best_offset = i;
			}
		}
		if (edges[axis][i].type == edge_type::start) ++n_below;
	}
	assert(n_below == pcount && n_above == 0);

	if (best_axis == -1 && retries < 2) {
		++retries;
		axis = (axis + 1) % 3;
		goto retry_split;
	}

	if (best_cost > old_cost) ++bad_refines;
	if ((best_cost > 4 * old_cost && pcount < 16) || best_axis == -1 || bad_refines == 3) {
		nodes[node_num].init_leaf(pnums, pcount, pind);
		return;
	}

	int n0 = 0, n1 = 0;
	for (int i = 0; i < best_offset; ++i)
		if (edges[best_axis][i].type == edge_type::start)
			prims0[n0++] = edges[best_axis][i].pnum;
	for (int i = best_offset + 1; i < 2 * pcount; ++i)
		if (edges[best_axis][i].type == edge_type::end)
			prims1[n1++] = edges[best_axis][i].pnum;

	float tsplit = edges[best_axis][best_offset].t;
	bbox bounds0 = bounds, bounds1 = bounds;
	bounds0.max[best_axis] = bounds1.min[best_axis] = tsplit;

	build(node_num + 1, bounds0, pbounds, prims0, n0, depth - 1, edges, prims0, prims1 + pcount, nodes, pind, bad_refines);
	int above_child = next_free_node;
	nodes[node_num].init_interior(best_axis, above_child, tsplit);
	build(above_child, bounds1, pbounds, prims1, n1, depth - 1, edges, prims0, prims1 + pcount, nodes, pind, bad_refines);
}

}

