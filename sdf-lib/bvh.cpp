#include "pch.h"
#include "bvh.h"
#include "io.h"

using namespace sdf;

void bvh::load(std::string filename) {
	io::read(filename, data, io::ply);
	
	int i = 0;
	for (int y = 0; y < data.primitive_count;y++) {
		uvec3 f = data.primitives[y];
		vec3 p1 = data.vertices[f.x];
		vec3 p2 = data.vertices[f.y];
		vec3 p3 = data.vertices[f.z];
		primitivesInfo.push_back(primitiveInfo(i, bbox(p1, p2, p3)));
		i++;
	}

}

void bvhNode::initLeaf(int first, int n, const bbox& b) {
	firstPrimitiveOffset = first;
	numPrimitives = n;
	bounds = b;
	children[0] = children[1] = nullptr;
}

void bvhNode::initInterior(int axis, bvhNode* n0, bvhNode* n1) {
	children[0] = n0;
	children[1] = n1;
	bounds = n0->bounds.opU(n1->bounds);
	splitAxis = axis;
	numPrimitives = 0;
}

bvhNode* bvh::recursiveBuild(std::vector<primitiveInfo>& primitivesInfo, int start, int end, int* totalNodes, std::vector<vec3>& orderedPrimitives) {
	(*totalNodes)++;
	bbox bounds;
	bounds = primitivesInfo[start].bounds;
	for (int i = start; i < end; ++i)
		bounds = bounds.opU(primitivesInfo[i].bounds);

	bvhNode* node = new bvhNode();
	int nPrimitives = end - start;
	if (nPrimitives == 1) {
			int firstPrimOffset = orderedPrimitives.size();
		for (int i = start; i < end; ++i) {
			int primNum = primitivesInfo[i].primitiveNumber;
			orderedPrimitives.push_back(data.primitives[primNum]);

		}
		node->initLeaf(firstPrimOffset, nPrimitives, bounds);
		return node;

	}
	else {
		bbox centroidBounds;
		centroidBounds.min = vec3(INT32_MAX);
		centroidBounds.max = vec3(INT32_MIN);
		for (int i = start; i < end; ++i)
			centroidBounds = centroidBounds.opU(primitivesInfo[i].centroid);
		int dim = centroidBounds.max_extent();
		int mid = (start + end) / 2;

		if (centroidBounds.max[dim] == centroidBounds.min[dim]) {
			int firstPrimOffset = orderedPrimitives.size();
			for (int i = start; i < end; ++i) {
				int primNum = primitivesInfo[i].primitiveNumber;
				orderedPrimitives.push_back(data.primitives[primNum]);

			}
			node->initLeaf(firstPrimOffset, nPrimitives, bounds);
			return node;
		}
		else {

			mid = (start + end) / 2;
			std::nth_element(&primitivesInfo[start], &primitivesInfo[mid],
				&primitivesInfo[end - 1] + 1,
				[dim](const primitiveInfo& a, const primitiveInfo& b) {
					return a.centroid[dim] < b.centroid[dim];
				});

				node->initInterior(dim,
					recursiveBuild(primitivesInfo, start, mid,
						totalNodes, orderedPrimitives),
					recursiveBuild(primitivesInfo, mid, end,
						totalNodes, orderedPrimitives));
		}

	}
	return node;
}

void bvh::buildBvh() {
	root = recursiveBuild(primitivesInfo, 0, data.primitive_count,&totalNodes, orderedPrimitives);

}

int bvh::recursiveFlatten(bvhNode* node, int* offset) {
	bvhNodeWrite* linearNode = &wnodes[*offset];
	linearNode->bounds = node->bounds;
	int myOffset = (*offset)++;
	if (node->numPrimitives > 0) {
		linearNode->primitivesOffset = node->firstPrimitiveOffset;
		linearNode->numPrimitives = node->numPrimitives;
	}
	else {
		linearNode->axis = node->splitAxis;
		linearNode->numPrimitives = 0;
		recursiveFlatten(node->children[0], offset);
		linearNode->secondChildOffset =recursiveFlatten(node->children[1], offset);
	}
	return myOffset;
}

void bvh::write(std::string filename) {
	wnodes = new bvhNodeWrite[totalNodes];
	int offset = 0;
	recursiveFlatten(root, &offset);

	auto file = std::fstream(filename, std::ios::out | std::ios::binary);
	file.write_bytes(totalNodes);
	file.write_bytes(data.vertex_count);
	file.write_bytes(data.primitive_count);

	file.write_nbytes(&wnodes[0], totalNodes);
	file.write_nbytes(&data.vertices[0], data.vertex_count);
	file.write_nbytes(&orderedPrimitives[0], data.primitive_count);

	file.close();
}


void io::write(const std::string &path, bvh &acc) {
	acc.write(path);
}

void io::read(const std::string &path, bvh &out) {
}

