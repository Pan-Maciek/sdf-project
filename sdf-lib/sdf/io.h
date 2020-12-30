#pragma once
#include "pch.h"
#include "kdtree.h"
#include "bvh.h"
#include "mesh.h"

namespace sdf {
namespace io {

#define write_bytes(x) write((char*) &x, sizeof x)
#define write_nbytes(x,n) write((char*) x, sizeof *x *n)

#define read_nbytes(x, bytes) read((char*) x, bytes)

struct ply_format { };
const ply_format ply;

void read(const std::string &path, sdf::mesh &out, ply_format _);
void read(const std::string &path, sdf::kd_acc &out);
void read(const std::string &path, sdf::bvh &out);

void write(const std::string &path, const sdf::kd_acc &in);
void write(const std::string &path, sdf::bvh &in);

}
}

