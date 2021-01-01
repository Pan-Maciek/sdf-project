#pragma once
#include "pch.h"
#include "mesh.h"
#include "bbox.h"

using namespace glm;

namespace sdf {

	class bvhNode {
	public:
		bvhNode() {};
		~bvhNode() {delete children[0];  delete children[1];};
		bbox bounds;
		bvhNode* children[2];
		int splitAxis, firstPrimitiveOffset, numPrimitives;
		void initLeaf(int first, int n, const bbox& b);
		void initInterior(int axis, bvhNode* n0, bvhNode* n1);
	};

	struct bvhNodeWrite {//44b
		bbox4 bounds; //32b
		union {
			uint primitivesOffset;    // leaf
			uint secondChildOffset;   // interior
		};//4b
		uint numPrimitives; //4b
		uint axis;//4b
	};

	struct primitiveInfo {
		primitiveInfo(int primitiveNumber, const bbox& bounds)
			:primitiveNumber(primitiveNumber), bounds(bounds),
			centroid(.5f * bounds.min + .5f * bounds.max) {}

		int primitiveNumber;
		bbox bounds;
		vec3 centroid;
	};

	class bvh {
	public:
		bvh() {};
		~bvh() { 
			delete[] wnodes; 
			delete[] vertices;
			delete root;
		};
		void load(std::string filename);
		void buildBvh();
		uint getNodeNum() const { return totalNodes; }
		uint getVertexNum() const { return data.vertex_count; }
		uint getPrimitiveNum() const { return data.primitive_count; }
		auto getVertices() const { return vertices; }
		auto getPrimitives() const { return &orderedPrimitives[0]; }
		auto getNodes() const { return &wnodes[0]; }
		
	private:
		int totalNodes = 0;
		bvhNode* root;
		bvhNodeWrite* wnodes;
		vec4* vertices;
		mesh data;
		std::vector<uint> orderedPrimitives;
		std::vector<primitiveInfo> primitivesInfo;
		int recursiveFlatten(bvhNode* node, int* offset);
		bvhNode* recursiveBuild(std::vector<primitiveInfo>& primitivesInfo, int start, int end, int* totalNodes, std::vector<uint>& orderedPrimitives);
		friend struct io;
	};
}