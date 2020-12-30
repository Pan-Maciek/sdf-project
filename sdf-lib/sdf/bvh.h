#pragma once
#include "pch.h"
#include "mesh.h"
#include <fstream>
#include <vector>
#include <algorithm>
using namespace glm;
namespace sdf {

	struct bounds3 {
		vec3 min;
		vec3 max;
	};

	class bvhNode {
	public:
		bounds3 bounds;
		bvhNode* children[2];
		int splitAxis, firstPrimitiveOffset, numPrimitives;
		void initLeaf(int first, int n, const bounds3& b);
		void initInterior(int axis, bvhNode* n0, bvhNode* n1);
	};

	struct bvhNodeWrite {
		bounds3 bounds; //24b
		union {
			int primitivesOffset;    // leaf
			int secondChildOffset;   // interior
		};//4b
		uint16_t numPrimitives; //2b
		uint8_t axis;//1b
		uint8_t pad[1];//1b
	};

	struct primitiveInfo {
		primitiveInfo(int primitiveNumber, const bounds3& bounds)
			:primitiveNumber(primitiveNumber), bounds(bounds),
			centroid(.5f * bounds.min + .5f * bounds.max) {}

		int primitiveNumber;
		bounds3 bounds;
		vec3 centroid;
	};

	class bvh {
	public:
		void load(std::string filename);
		void buildBvh();
		bvhNode* recursiveBuild(std::vector<primitiveInfo>& primitivesInfo, int start, int end, int* totalNodes, std::vector<vec3>& orderedPrimitives);
		void write(std::string filename);
	private:
		int totalNodes = 0;
		bvhNode* root;
		bvhNodeWrite* wnodes;
		mesh data;
		//std::vector<vec3> primitives;
		std::vector<vec3> orderedPrimitives;
		std::vector<primitiveInfo> primitivesInfo;
		int recursiveFlatten(bvhNode* node, int* offset);
	};
}