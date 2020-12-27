#include "pch.h"
#include "kdtree.h"

namespace sdf {

void kd_node::init_leaf(int *pnums, int pcount, std::vector<int> &pindices) {
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
	split = split;
}

}

