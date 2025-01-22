#include "accelerate.hh"
#include <iostream>
#include "contains.hh"
#include "vector_utilities.hh"
using namespace std;

static inline void minvec3(vector3&v, const vector3&vin)
{
	vsFmin(3, (float*)&v, (float*)&vin, (float*)&v);
}
static inline void maxvec3(vector3&v, const vector3&vin)
{
	vsFmax(3, (float*)&v, (float*)&vin, (float*)&v);
}
enum class Axis
{
	X, Y, Z
};
static int accelerate2(vector<Triangle>&triangles, AlignedBox& box, uint max_depth, uint depth=0)
{
	static bool gen;	static unsigned char objectid=1;

	vector3 size = box.max +- box.min;
	Axis largest_axis = Axis::X;
	if (size.y > size.x && size.y > size.z)
		largest_axis = Axis::Y;
	else if (size.z > size.x && size.z > size.y)
		largest_axis = Axis::Z;

	vector<AlignedBox*> newboxes;
	
	vector3 _min1 = box.min, _min2 = box.min, _max1 = box.max, _max2 = box.max;
	switch (largest_axis)
	{
	case Axis::X:
		_min2.x = _max1.x = (box.min.x + box.max.x) / 2;
		break;
	case Axis::Y:
		_min2.y = _max1.y = (box.min.y + box.max.y) / 2;
		break;
	case Axis::Z:
		_min2.z = _max1.z = (box.min.z + box.max.z) / 2;
		break;
	}
	vector3 min = {INFINITY, INFINITY, INFINITY};
	vector3 max = {-INFINITY, -INFINITY, -INFINITY};

	newboxes.push_back(new AlignedBox(objectid++, min, max, Material()));
	newboxes.back()->_min = _min1;
	newboxes.back()->_max = _max1;
	newboxes.back()->start_indice = 0;
	newboxes.back()->end_indice = triangles.size();

	newboxes.push_back(new AlignedBox(objectid++, min, max, Material()));
	newboxes.back()->_min = _min2;
	newboxes.back()->_max = _max2;
	newboxes.back()->start_indice = 0;
	newboxes.back()->end_indice = triangles.size();

	uint children=0;
	int maxdepth=depth;
	for (auto&newbox : newboxes)
	{
		vector<int> box_triangle_indices;
		for (int i = box.start_indice; i < box.end_indice; i++)	//This supposedly decrease the complexity, assuming that consecutive triangles are usually also stored consecutively
		{
			auto& triangle = triangles[i];
			//if (newbox->_box.contains(triangle.vertex1))
			if (contains(newbox->_min, newbox->_max, triangle.vertex1))
			{
				//cout << "triangle " << i << " is in box " << newbox->getid() << endl;
				newbox->embrace(triangle,i);
				box_triangle_indices.push_back(i);
				//cout << "triangle " << i << " is in box " << newbox->getid() << endl;
			}
			//else cout << newbox->_min << " " << newbox->_max << " does not contain " << triangle.vertex1 << endl;
		}
		if (!newbox->isEmpty())
		{
			//cout << "box " << newbox->getid() << endl;
			children++;
			box.children.push_back(newbox);
			//if (newbox->box.sizes().norm() >= box.box.sizes().norm() || depth+1>=max_depth)
			if (norm(newbox->max +- newbox->min) >= norm(box.max +- box.min) || depth+1>=max_depth || box_triangle_indices.size() <= 1)
			{
				for (auto&indice: box_triangle_indices)
					newbox->triangles.push_back(triangles[indice]);
				/* cout << "box " << newbox->getid() << " has " << newbox->triangles.size() << " triangles" << endl;
				cout << "indices size: " << box_triangle_indices.size() << endl;
				cout << "depth: " << depth << endl;
				cout << "new min: " << newbox->min << endl;
				cout << "new max: " << newbox->max << endl;
				cout << "old min: " << box.min << endl;
				cout << "old max: " << box.max << endl; */
			}
			else
				maxdepth = std::max(maxdepth, accelerate2(triangles, *newbox, max_depth, depth+1));
		}
		//else cout << newbox->min << " " << newbox->max << " is empty" << endl;
	}
	return maxdepth;
}

int accelerate1(std::vector<Triangle>&triangles, AlignedBox& bounding_box)
{
	static bool gen;
	vector3 min_vertex = {INFINITY, INFINITY, INFINITY};
	vector3 max_vertex = {-INFINITY, -INFINITY, -INFINITY};
	for (auto& triangle : triangles)
	{
		minvec3(min_vertex, triangle.vertex1);
		minvec3(min_vertex, triangle.vertex1 + triangle.vertex2delta);
		minvec3(min_vertex, triangle.vertex1 + triangle.vertex3delta);

		maxvec3(max_vertex, triangle.vertex1);
		maxvec3(max_vertex, triangle.vertex1 + triangle.vertex2delta);
		maxvec3(max_vertex, triangle.vertex1 + triangle.vertex3delta);
	}
	bounding_box = AlignedBox(0, min_vertex, max_vertex, Material());
	bounding_box.start_indice = 0;
	bounding_box.end_indice = triangles.size();

	return accelerate2(triangles, bounding_box, log2(triangles.size()));
}


static vector<vector3>corners(vector3 min, vector3 max)
{
	vector<vector3>accumulator;
	accumulator.push_back({min.x, min.y, min.z});
	accumulator.push_back({min.x, min.y, max.z});
	accumulator.push_back({min.x, max.y, min.z});
	accumulator.push_back({min.x, max.y, max.z});
	accumulator.push_back({max.x, min.y, min.z});
	accumulator.push_back({max.x, min.y, max.z});
	accumulator.push_back({max.x, max.y, min.z});
	accumulator.push_back({max.x, max.y, max.z});
	return accumulator;
}
static void extend(vector3&min, vector3&max, const vector3 point)
{
	if (!contains(min, max, point))
		min.x = fmin(min.x, point.x),
		min.y = fmin(min.y, point.y),
		min.z = fmin(min.z, point.z),
		max.x = fmax(max.x, point.x),
		max.y = fmax(max.y, point.y),
		max.z = fmax(max.z, point.z);
}

static void accelerate4(std::vector<Mesh>&meshes, AlignedBox& box, uint depth=0)
{
	static bool gen;	static unsigned char objectid=1;	static uint max_depth = 1;
	vector3 halfsize = box.max +- box.min;
	cblas_sscal(3, .5f, (float*)&halfsize, 1);

	cout << "accelerating box " << box.getid() << " with " << meshes.size() << " meshes" << endl;

	vector<AlignedBox*> newboxes;
	vector3 ijk;
	for (ijk.x = 0; ijk.x < 2; ijk.x++)
	{
		for (ijk.y = 0; ijk.y < 2; ijk.y++)
		{
			for (ijk.z = 0; ijk.z < 2; ijk.z++)
			{
				vector3 _min;
				vsMul(3, (float*)&halfsize, (float*)&ijk, (float*)&_min);
				vsAdd(3, (float*)&_min, (float*)&box.min, (float*)&_min);
				vector3 _max = _min + halfsize;
				vector3 min = {INFINITY, INFINITY, INFINITY};
				vector3 max = {-INFINITY, -INFINITY, -INFINITY};
				newboxes.push_back(new AlignedBox(objectid++, min, max, Material()));
				newboxes.back()->_min = _min;
				newboxes.back()->_max = _max;
				newboxes.back()->start_indice = 0;
				newboxes.back()->end_indice = 0;
			}
		}
	}
	uint children=0;
	for (auto&newbox : newboxes)
	{
		for (auto&mesh : meshes)
			if (contains(newbox->_min, newbox->_max, (mesh.data->min+mesh.data->max)/2))
				for (auto&&corner: corners(mesh.data->min, mesh.data->max))
					extend(newbox->min, newbox->max, corner);
		if (!newbox->isEmpty())
		{
			children++;
			box.children.push_back(newbox);
			if (norm(newbox->max +- newbox->min) >= norm(box.max +- box.min) || depth+1>=max_depth)
			{
				for (auto&mesh: meshes)
					if (contains(newbox->_min, newbox->_max, (mesh.data->min+mesh.data->max)/2))
						newbox->meshes.push_back(&mesh);
			}
			else
			{
				//cout << "box " << newbox->getid() << " has " << newbox->meshes.size() << " meshes" << endl;
				accelerate4(meshes, *newbox, depth+1);
			}
		}
		//cout << "box " << newbox->getid() << " has " << newbox->meshes.size() << " meshes" << endl;
	}
}

void accelerate3(std::vector<Mesh>&meshes, AlignedBox& gigabox)
{
	for (auto&mesh: meshes)
	{
		for (auto&&corner: corners(mesh.data->min, mesh.data->max))
		{
			corner = mesh.transformation * corner;
			extend(gigabox.min, gigabox.max, corner);
		}
	}
	accelerate4(meshes, gigabox);
}

#include <algorithm>
AlignedBox*accelerate5(std::vector<Triangle*>triangles)
{
	static int id=0;
	auto size = triangles.size();
	if (size == 0)
		return nullptr;
	if (size < 3)
	{
		auto box = new AlignedBox(0, {INFINITY, INFINITY, INFINITY}, {-INFINITY, -INFINITY, -INFINITY}, Material());
		for (auto&triangle:triangles)
			box->embrace(triangle);
		return box;
	}
	//
	auto compare = +[](const Triangle* a, const Triangle* b) -> bool { return a->vertex1.x < b->vertex1.x; };
	Axis sort_axis;
	switch (rand()%3)
	{
	case 0: sort_axis = Axis::X; compare = +[](const Triangle* a, const Triangle* b) -> bool { return a->vertex1.x < b->vertex1.x; }; break;
	case 1: sort_axis = Axis::Y; compare = +[](const Triangle* a, const Triangle* b) -> bool { return a->vertex1.y < b->vertex1.y; }; break;
	case 2: sort_axis = Axis::Z; compare = +[](const Triangle* a, const Triangle* b) -> bool { return a->vertex1.z < b->vertex1.z; }; break;
	}
	std::sort(triangles.begin(), triangles.end(), compare);
	auto median = size/2;
	auto left = std::vector<Triangle*>(triangles.begin(), triangles.begin() + median);
	auto right = std::vector<Triangle*>(triangles.begin() + median, triangles.end());
	auto left_box = accelerate5(left);
	auto right_box = accelerate5(right);
	auto box = new AlignedBox(id++, {INFINITY, INFINITY, INFINITY}, {-INFINITY, -INFINITY, -INFINITY}, Material());
	box->embrace(left_box);
	box->embrace(right_box);
	//cout << "box " << box->getid() << " has " << size << " triangles" << endl;
	return box;
}