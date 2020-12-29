#include "pch.h"
#include "mesh.h"
#include <fstream>

using namespace std;
using namespace glm;

namespace sdf {

mesh sdf::load_ply(const string path) {
	ifstream file(path, ios::binary);
	string line, tag;

	getline(file, line);
	if (line != "ply")
		return {nullptr, nullptr, 0, 0};

	getline(file, line);
	if (line != "format binary_little_endian 1.0")
		return {nullptr, nullptr, 0, 0};

	int vcount = 0, pcount = 0;

	do {
		file >> tag;
		if (tag == "comment") getline(file, line);
		else if (tag == "element") {
			file >> tag;
			if (tag == "vertex") file >> vcount;
			else if (tag == "face") file >> pcount;
		}
	} while (tag != "end_header");
	file.get(); // skip \n

	assert(vcount > 0);
	assert(pcount > 0);

	int vbytes = vcount * sizeof(vec3);
	vec3 *vertices = (vec3*) malloc(vbytes);
	file.read((char*) vertices, vbytes);

	int pbytes = pcount * sizeof(uvec3);
	uvec3 *primitives = (uvec3*) malloc(pbytes);
	for (int i = 0; i < pcount; ++i) {
		int n = file.get();
		assert(n == 3);
		file.read((char*) (primitives + i), sizeof(uvec3));
	}

	return {vertices, primitives, vcount, pcount};
}

}
