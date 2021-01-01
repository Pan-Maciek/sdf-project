#include "pch.h"
#include "bvh.h"
#include "io.h"

using namespace sdf;

void bvh::load(std::string filename) {
	io::read(filename, data, io::ply);
	
	vertices = new vec4[data.vertex_count];
	for (int i = 0; i < data.vertex_count; i++)
		vertices[i] = vec4(data.vertices[i], 0.);

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

bvhNode* bvh::recursiveBuild(std::vector<primitiveInfo>& primitivesInfo, int start, int end, int* totalNodes, std::vector<uint>& orderedPrimitives) {
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
			orderedPrimitives.push_back(data.primitives[primNum].x);
			orderedPrimitives.push_back(data.primitives[primNum].y);
			orderedPrimitives.push_back(data.primitives[primNum].z);

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
		if (centroidBounds.max[dim] - centroidBounds.min[dim]==0) {
			int firstPrimOffset = orderedPrimitives.size();
			for (int i = start; i < end; ++i) {
				int primNum = primitivesInfo[i].primitiveNumber;
				orderedPrimitives.push_back(data.primitives[primNum].x);
				orderedPrimitives.push_back(data.primitives[primNum].y);
				orderedPrimitives.push_back(data.primitives[primNum].z);

			}
			node->initLeaf(firstPrimOffset, nPrimitives, bounds);
			return node;
		}
		else {
			/*
			float pmid = (centroidBounds.min[dim] + centroidBounds.max[dim]) / 2;
			primitiveInfo* midPtr =
				std::partition(&primitivesInfo[start], &primitivesInfo[end - 1] + 1,
					[dim, pmid](const primitiveInfo& pi) {
						return pi.centroid[dim] < pmid;
					});
			mid = midPtr - &primitivesInfo[0];
			//if (mid != start && mid != end)
				//break;*/


			/*mid = (start + end) / 2;
			std::nth_element(&primitivesInfo[start], &primitivesInfo[mid],
				&primitivesInfo[end - 1] + 1,
				[dim](const primitiveInfo& a, const primitiveInfo& b) {
					return a.centroid[dim] < b.centroid[dim];
				});*/
			uint maxPrimsInNode = 16;
			if (nPrimitives <= 4) {
					mid = (start + end) / 2;
				std::nth_element(&primitivesInfo[start], &primitivesInfo[mid],
					&primitivesInfo[end - 1] + 1,
					[dim](const primitiveInfo& a, const primitiveInfo& b) {
						return a.centroid[dim] < b.centroid[dim];
					});

			}
			else {
					constexpr int nBuckets = 12;
				struct BucketInfo {
					int count = 0;
					bbox bounds;
				};
				BucketInfo buckets[nBuckets];

					for (int i = start; i < end; ++i) {
						int b = nBuckets *
							centroidBounds.offset(primitivesInfo[i].centroid)[dim];
						if (b == nBuckets) b = nBuckets - 1;
						buckets[b].count++;
						buckets[b].bounds = buckets[b].bounds.opU(primitivesInfo[i].bounds);
					}

					float cost[nBuckets - 1];
				for (int i = 0; i < nBuckets - 1; ++i) {
					bbox b0, b1;
					int count0 = 0, count1 = 0;
					for (int j = 0; j <= i; ++j) {
						b0 = b0.opU(buckets[j].bounds);
						count0 += buckets[j].count;
					}
					for (int j = i + 1; j < nBuckets; ++j) {
						b1 = b1.opU(buckets[j].bounds);
						count1 += buckets[j].count;
					}
					cost[i] = .125f + (count0 * b0.area() +
						count1 * b1.area()) / bounds.area();
				}

					float minCost = cost[0];
				int minCostSplitBucket = 0;
				for (int i = 1; i < nBuckets - 1; ++i) {
					if (cost[i] < minCost) {
						minCost = cost[i];
						minCostSplitBucket = i;
					}
				}

					float leafCost = nPrimitives;
				if (nPrimitives > maxPrimsInNode || minCost < leafCost) {
					primitiveInfo* pmid = std::partition(&primitivesInfo[start],
						&primitivesInfo[end - 1] + 1,
						[=](const primitiveInfo& pi) {
							int b = nBuckets * centroidBounds.offset(pi.centroid)[dim];
							if (b == nBuckets) b = nBuckets - 1;
							return b <= minCostSplitBucket;
						});
					mid = pmid - &primitivesInfo[0];
				}
				else {
						int firstPrimOffset = orderedPrimitives.size();
					for (int i = start; i < end; ++i) {
						int primNum = primitivesInfo[i].primitiveNumber;
						orderedPrimitives.push_back(data.primitives[primNum].x);
						orderedPrimitives.push_back(data.primitives[primNum].y);
						orderedPrimitives.push_back(data.primitives[primNum].z);

					}
					node->initLeaf(firstPrimOffset, nPrimitives, bounds);
					return node;

				}

			}

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
		//linearNode->offset = node->firstPrimitiveOffset;
		linearNode->numPrimitives = node->numPrimitives;
		//linearNode->numPrimAxis = node->numPrimitives << 2;
	}
	else {
		linearNode->axis = node->splitAxis;
		//linearNode->numPrimAxis= node->splitAxis;
		linearNode->numPrimitives = 0;
		recursiveFlatten(node->children[0], offset);
		linearNode->secondChildOffset =recursiveFlatten(node->children[1], offset);
		//linearNode->offset = recursiveFlatten(node->children[1], offset);
	}
	return myOffset;
}

