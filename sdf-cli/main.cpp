#include "pch.h"

string input, output;
int main(int argc, char** argv) {

	parser cmdl(argc, argv);

	if (!cmdl({"-i", "--in"})) {
		cout << "Input file not provided (use -i flag)" << endl;
		return EXIT_FAILURE;
	}

	if (!cmdl({"-o", "--out"})) {
		cout << "Output file not provided (use -o flag)" << endl;
		return EXIT_FAILURE;
	}

	cmdl({"-i", "--in"}) >> input;
	cmdl({"-o", "--out"}) >> output;

	bool iskd = cmdl[{"-kd", "--kdtree"}], isbvh = cmdl[{"-bvh", "--bvh"}];
	if (iskd && isbvh) {
		cout << "-kd and -bvh both present" << endl;
		return EXIT_FAILURE;
	} else if (!iskd && !isbvh) {
		cout << "-kd or -bvh must be present" << endl;
		return EXIT_FAILURE;
	}

	mesh mesh;
	io::read(input, mesh, io::ply);

	if (iskd) {
		int isect_cost, traversal_cost, max_primitives, max_depth;
		float empty_bonus;
		cmdl({"-ic", "--isect_cost"}, 80) >> isect_cost;
		cmdl({"-tc", "--traversal_cost"}, 1) >> traversal_cost;
		cmdl({"-eb", "--empty_bonus"}, 0.5) >> empty_bonus;
		cmdl({"-mp", "--max_prims", "--max_primitives"}, 1) >> max_primitives;
		cmdl({"-md", "--max_depth"}, 0) >> max_depth;

		kd_builder builder(isect_cost, traversal_cost, empty_bonus, max_primitives, max_depth);
		kd_acc acc = builder.build(mesh);

		io::write(output, acc);
	} else if (isbvh) {
		std::string split;
		int spliti, desiredPrimsInNode,maxDepth,nBuckets,maxPrimsInNode;
		float bboxCollapse;
		cmdl({ "-split", "--split" }, 0) >> split;
		if (split == "sah")
			spliti = 2;
		else if (split == "exact")
			spliti = 1;
		else
			spliti = 0;

		cmdl({ "-dpin", "--desired_prims_in_node" }, 4) >> desiredPrimsInNode;
		cmdl({ "-bc", "--bbox_collapse" }, 10e-3) >> bboxCollapse;
		cmdl({ "-md", "--max_depth" }, 8) >> maxDepth;
		cmdl({ "-nb", "--n_buckets" }, 8) >> nBuckets;
		cmdl({ "-maxp", "--max_prims_in_node" }, 8) >> maxPrimsInNode;

		bvh acc(spliti, desiredPrimsInNode,bboxCollapse, maxDepth, nBuckets, maxPrimsInNode);
		acc.load(input);
		acc.buildBvh();
		io::write(output, acc);
	}

}
