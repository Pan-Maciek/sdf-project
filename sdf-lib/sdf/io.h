#pragma once
#include "pch.h"
#include "kdtree.h"
#include "bvh.h"
#include "mesh.h"

namespace sdf {

#define write_bytes(x) write((char*) &x, sizeof x)
#define write_nbytes(x,n) write((char*) x, sizeof *x *n)

#define read_bytes(x) read((char*) &x, sizeof x)
#define read_nbytes(x, n) read((char*) x, sizeof *x *n)

struct ply_format { };
enum class format { unknown = 0, ply, bvh, kd };

struct io {

static void read(const std::string &path, sdf::mesh &out, ply_format _);
static void read(const std::string &path, sdf::kd_acc &out);
static void read(const std::string &path, sdf::bvh &out);

static void write(const std::string &path, const sdf::kd_acc &in);
static void write(const std::string &path, sdf::bvh &in);

static format detect(const std::string &path);

static const ply_format ply;
};

}

