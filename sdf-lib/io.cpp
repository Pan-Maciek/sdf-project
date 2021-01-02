#include "pch.h"
#include "io.h"

using namespace std;

namespace sdf {

void io::write(const string &path, const kd_acc &acc) {
	ofstream file(path, ios::binary);

	file << "kd";
	file.write_bytes(acc.node_count);
	file.write_bytes(acc.index_count);
	file.write_bytes(acc.mesh.vertex_count);
	file.write_bytes(acc.mesh.primitive_count);

	assert(acc.nodes != nullptr);
	file.write_nbytes(acc.nodes, acc.node_count);

	assert(acc.indices != nullptr);
	file.write_nbytes(acc.indices, acc.index_count);

	assert(acc.mesh.vertices != nullptr);
	file.write_nbytes(acc.mesh.vertices, acc.mesh.vertex_count);

	assert(acc.mesh.primitives != nullptr);
	file.write_nbytes(acc.mesh.primitives, acc.mesh.primitive_count);

	file.close();
}

void io::read(const string &path, kd_acc &out) {
	ifstream file(path, ios::binary);

	char magic_bytes[2];
	file.read_nbytes(magic_bytes, 2);
	assert(strncmp("kd", magic_bytes, 2) == 0);

	file.read_bytes(out.node_count);
	assert(out.node_count > 0);

	file.read_bytes(out.index_count);
	assert(out.index_count > 0);

	file.read_bytes(out.mesh.vertex_count);
	assert(out.mesh.vertex_count > 0);

	file.read_bytes(out.mesh.primitive_count);
	assert(out.mesh.primitive_count > 0);

	out.nodes = (kd_node*) malloc(out.node_count * sizeof(kd_node));
	file.read_nbytes(out.nodes, out.node_count);

	out.indices = (int*) malloc(out.index_count * sizeof(int));
	file.read_nbytes(out.indices, out.index_count);

	out.mesh.vertices = (vec3*) malloc(out.mesh.vertex_count * sizeof(vec3));
	file.read_nbytes(out.mesh.vertices, out.mesh.vertex_count);

	out.mesh.primitives = (uvec3*) malloc(out.mesh.primitive_count * sizeof(uvec3));
	file.read_nbytes(out.mesh.primitives, out.mesh.primitive_count);

	file.close();
}

void io::write(const string &path, bvh &acc) {
	acc.wnodes = new bvhNodeWrite[acc.totalNodes];
	int offset = 0;
	acc.recursiveFlatten(acc.root, &offset);

	ofstream file(path, ios::binary);
	file << "bvh";
	file.write_bytes(acc.totalNodes);
	file.write_bytes(acc.data.vertex_count);
	file.write_bytes(acc.data.primitive_count);

	file.write_nbytes(acc.wnodes, acc.totalNodes);
	file.write_nbytes(acc.vertices, acc.data.vertex_count);
	file.write_nbytes(&acc.orderedPrimitives[0], acc.data.primitive_count*3);

	file.close();
}

void io::read(const std::string &path, bvh &out) {
	ifstream file(path, ios::binary);

	char magic_bytes[3];
	file.read_nbytes(magic_bytes, 3);
	assert(strncmp("bvh", magic_bytes, 3) == 0);

	int total_nodes, vertex_count, primitive_count;
	file.read_bytes(total_nodes);
	assert(total_nodes > 0);

	file.read_bytes(vertex_count);
	assert(vertex_count > 0);

	file.read_bytes(primitive_count);
	assert(primitive_count > 0);

	out.totalNodes = total_nodes;
	out.data.vertex_count = vertex_count;
	out.data.primitive_count = primitive_count;

	out.wnodes = (bvhNodeWrite*) malloc(total_nodes * sizeof(bvhNodeWrite));
	file.read_nbytes(out.wnodes, total_nodes);

	out.vertices = (vec4*) malloc(vertex_count * sizeof(vec4));
	file.read_nbytes(out.vertices, vertex_count);

	out.orderedPrimitives.resize(primitive_count*3);
	file.read_nbytes(&out.orderedPrimitives[0], primitive_count*3);

	file.close();
}

void io::read(const string &path, mesh &out, ply_format _) {
	ifstream file(path, ios::binary);
	string line, tag;

	getline(file, line);
	assert(line == "ply");

	getline(file, line);
	assert(line == "format binary_little_endian 1.0");

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
	file.read_nbytes(vertices, vcount);

	int pbytes = pcount * sizeof(uvec3);
	uvec3 *primitives = (uvec3*) malloc(pbytes);
	for (int i = 0; i < pcount; ++i) {
		int n = file.get();
		assert(n == 3);
		file.read_bytes(primitives[i]);
	}

	out = {vertices, primitives, vcount, pcount};
}

format io::detect(const std::string &path) {
	ifstream file(path);

	char magic_bytes[3];
	file.read(magic_bytes, 3);
	
	if (strncmp("ply", magic_bytes, 3) == 0)
		return format::ply;
	if (strncmp("kd", magic_bytes, 2) == 0)
		return format::kd;
	if (strncmp("bvh", magic_bytes, 3) == 0)
		return format::bvh;

	return format::unknown;
}

const ply_format io::ply = {};

}
