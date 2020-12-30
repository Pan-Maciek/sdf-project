#include "pch.h"
#include "bvh.h"
using namespace sdf;

bounds3 opU(bounds3 b1, bounds3 b2) {
	bounds3 b3;
	b3.min = min(b1.min, b2.min);
	b3.max = max(b1.max, b2.max);
	return b3;
}

bounds3 opU(bounds3 b1, vec3 p2) {
	bounds3 b2;
	b2.min = min(b1.min, p2);
	b2.max = max(b1.max, p2);
	return b2;
}

int maximumExtent(bounds3 b) {
	vec3 extent = b.max - b.min;
	if (extent.x > extent.y&& extent.x > extent.z) 
		return 0;
	else if (extent.y > extent.z)
		return 1;
	else 
		return 2;
}

bounds3 calcBounds(vec3 p1, vec3 p2, vec3 p3) {
	bounds3 b;
	b.min = p1;
	b.max = p1;
	b.min = min(b.min, p2);
	b.min = min(b.min, p3);
	b.max = max(b.max, p2);
	b.max = max(b.max, p3);
	return b;
}

void bvh::load(std::string filename) {
	data = load_ply(filename);
	
	int i = 0;
	for (int y = 0; y < data.primitive_count;y++) {
		uvec3 f = data.primitives[y];
		vec3 p1 = data.vertices[f.x];
		//primitives.push_back(p1);
		vec3 p2 = data.vertices[f.y];
		//primitives.push_back(p2);
		vec3 p3 = data.vertices[f.z];
		//primitives.push_back(p3);
		primitivesInfo.push_back(primitiveInfo(i, calcBounds(p1, p2, p3)));
		i++;
	}

}

void bvhNode::initLeaf(int first, int n, const bounds3& b) {
	firstPrimitiveOffset = first;
	numPrimitives = n;
	bounds = b;
	children[0] = children[1] = nullptr;
}

void bvhNode::initInterior(int axis, bvhNode* n0, bvhNode* n1) {
	children[0] = n0;
	children[1] = n1;
	bounds = opU(n0->bounds, n1->bounds);
	splitAxis = axis;
	numPrimitives = 0;
}

bvhNode* bvh::recursiveBuild(std::vector<primitiveInfo>& primitivesInfo, int start, int end, int* totalNodes, std::vector<vec3>& orderedPrimitives) {
	(*totalNodes)++;
	bounds3 bounds;
	bounds = primitivesInfo[start].bounds;
	for (int i = start; i < end; ++i)
		bounds = opU(bounds, primitivesInfo[i].bounds);

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
		bounds3 centroidBounds;
		centroidBounds.min = vec3(INT32_MAX);
		centroidBounds.max = vec3(INT32_MIN);
		for (int i = start; i < end; ++i)
			centroidBounds = opU(centroidBounds, primitivesInfo[i].centroid);
		int dim = maximumExtent(centroidBounds);
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
	//primitives.swap(orderedPrimitives);
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
	file.write((char*)&totalNodes, 4);
	file.write((char*)&data.vertex_count, 4);
	file.write((char*)&data.primitive_count, 4);

	file.write((char*)&data.vertices[0], 12*data.vertex_count);
	file.write((char*)&data.primitives[0], 12*data.primitive_count);
	file.write((char*)&wnodes[0], 32 * totalNodes);

	file.close();
	
}

