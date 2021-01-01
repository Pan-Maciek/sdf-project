#pragma once
#include "pch.h"
#include "mesh.h"
#include "bbox.h"
#include <glm/gtc/packing.hpp>
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

	struct bvhNodeWrite {//48b->20b
		bbox4 bounds; //32b->12b
		//uint min;
		//uint minMax;
		//uint max;
		union {
			uint primitivesOffset;    // leaf
			uint secondChildOffset;   // interior
		};//4b
		uint numPrimitives; //4b //4b
		uint axis;//4b           //
		uint pad;//pad to 48b
		//unsigned int offset;
		//unsigned int numPrimAxis;//30 bit numPrim 2 bit axis
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
		auto getNodes() const { return wnodes; }
		
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