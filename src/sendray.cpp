#include "sendray.hh"
#include <iostream>
#include "intersect.hh"
#include "vector_utilities.hh"
#include "sphere2d.hh"
#include "test.hh"
#include "Light.hh"
using namespace std;

static float cosclamp(const vector3& a, vector3 b)
{
	float result = cblas_sdot(3, (float*)&a, 1, (float*)&b, 1);
	if (result > 0)	return result/norm(a)/norm(b);
	return 0;
}
static vector3 perp_dir(const vector3& vec)	//returns a vector perpendicular to vec
{
	vector3 result(vec.y, -vec.x, 0);
	if (vec.x == 0 && vec.y == 0)	result.x=1;
	return result;
}

vector3 sendray(Scene&scene, Ray ray, bool verbose, uint depth, float pathlength, vector3 pixel, Xsystem*xsystem, Path*tree)
{
	vector3 ambient, diffuse, specular, mirror, dielectric, pathtracing;
	ambient=diffuse=specular=mirror=dielectric=pathtracing = {0, 0, 0};

	vector3 irradiance;
	struct Intersection first = intersect(scene,ray,verbose), second;
	float specularcos, objecttolight;
	pathlength += norm(first.position +- ray.start);

	vector3 correction, wo = ray.start +- first.position, wi, h, reflection, mirrorwo, d, normal, attenuation = {1,1,1};
	Ray reflectionray, refractionray;
	bool entering;
	float nratio, costeta, cosphi, rparallel, rperpendicular, n1,n2, fr,ft;

	vector3 k_diffuse;

	vector3 shadow_start;

	switch (first.intersecting)
	{
	case false:	//ray did not hit any object
		if (verbose)
			cout << "ray did not hit any object" << endl;
		switch (scene.background_type)
		{
		case Scene::color:
			if (verbose)
				cout << "Scene does not have background texture." << endl;
			return scene.background_color;
		case Scene::latlong:	//latlong image
			if (verbose)
				cout << "Scene has background texture." << endl;
			{
				/* float u,v;
				float&xz=ray.xzangl;
				float&y =ray.yangl;
				u = fmod(fmod(xz/M_PI, 2)+2, 2) / 2;
				v = fmod(fmod(y/M_PI, 1)+1, 1); //to make it positive */
				auto coords = sphere2dlatlong(ray.start, ray.start + ray.getdirection(), 1);
				float u = coords.x, v = coords.y;
				int resx = scene.images[scene.background.image_indice][0].size()-1, resy = scene.images[scene.background.image_indice].size()-1;	// -1 because the last index is the size of the array; this could be done better by mapping the texture better.
				if (rint(v * resy) < 0 || rint(v * resy) >= scene.images[scene.background.image_indice].size() || rint(u * resx) < 0 || rint(u * resx) >= scene.images[scene.background.image_indice][0].size())
				{
					cout << "Texture coordinates out of bounds" << endl;
					cout << "coords: " << u << " " << v << endl;
					cout << rint(u * resx) << " " << rint(v * resy) << endl;
					exit(1);
				}
				vector3 color = scene.images[scene.background.image_indice][rint(v * resy)][rint(u * resx)];
				if (verbose)
					cout << "background color: " << color << endl;
				return color;
			}
			break;
		case Scene::probe:	//probe image
			if (verbose)
				cout << "Scene has background texture." << endl;
			{
				/* float u,v;
				float&xz=ray.xzangl;
				float&y =ray.yangl;
				float modxz = fmod(fmod(xz, 2*M_PI)+2*M_PI, 2*M_PI);
				float sy = sin(y), cy = cos(y);
				u = cy + modxz / 2 / M_PI * (1 - 2 * cy);
				v = sy/2+0.5; */
				auto coords = sphere2dprobe(ray.start, ray.start + ray.getdirection(), 1);
				float u = coords.x, v = coords.y;

				int resx = scene.images[scene.background.image_indice][0].size()-1, resy = scene.images[scene.background.image_indice].size()-1;	// -1 because the last index is the size of the array; this could be done better by mapping the texture better.
				if (rint(v * resy) < 0 || rint(v * resy) >= scene.images[scene.background.image_indice].size() || rint(u * resx) < 0 || rint(u * resx) >= scene.images[scene.background.image_indice][0].size())
				{
					cout << "Texture coordinates out of bounds" << endl;
					cout << "coords: " << u << " " << v << endl;
					cout << rint(u * resx) << " " << rint(v * resy) << endl;
					exit(1);
				}
				vector3 color = scene.images[scene.background.image_indice][rint(v * resy)][rint(u * resx)];
				if (verbose)
					cout << "background color: " << color << endl;
				return color;
			}
			case Scene::replace:
			{
				if (pixel.z==0)	//ray is not primary
					return {0,0,0};
				auto&image=scene.images[scene.background.image_indice];
				if (verbose)
				{
					cout << "background is replaced" << endl;
					cout << pixel.x << " " << pixel.y << endl;
					cout << image.size() << " " << image[0].size() << endl;
					cout << image[pixel.x][pixel.y] << endl;
				}
				auto pixelx = pixel.x * image[0].size() / scene.current_camera->image_resolution[0];
				auto pixely = pixel.y * image.size() / scene.current_camera->image_resolution[1];
				return image[pixel.y][pixel.x];
			}
			break;
		}
	case true:	//ray hit an object
		if (verbose)
		{
			string material;
			if (first.getobject()->getmaterial().is_dielectric())	material += "dielectric";
			if (first.getobject()->getmaterial().is_mirror())	material += "mirror";
			if (material == "")	material = "diffuse";
			cout << "ray hit " << material << " " << first.getobject()->say() << " " << first.getobject()->getid() << " at " << first.position << endl;
			cout << "object has " << first.gettextures().size() << " textures" << endl;

			xsystem->mirror_path.push_back(shadow_start = test(*xsystem, first.position));
			tree->position = shadow_start;
		}

		correction = first.normal * scene.shadow_ray_epsilon;

		if (!scene.pathtracing)
		vsMul(3, (float*)&first.getobject()->getmaterial().ambient, (float*)&scene.ambient_light, (float*)&ambient);
		
		k_diffuse = first.getobject()->getmaterial().diffuse;

		for (auto& texture : first.gettextures())
		{
			switch (texture.type)
			{
			case Texture::bump:
				{
					if (verbose)
						cout << "bump map" << endl;
					vector3 coords = first.get2dcoordinates();
					//cout << "bump map coords: " << coords << endl;
					int resx = scene.images[texture.image_index][0].size()-1, resy = scene.images[texture.image_index].size()-1;	// -1 because the last index is the size of the array; this could be done better by mapping the texture better.
					if (rint(coords.y * resy) < 0 || rint(coords.y * resy) >= scene.images[texture.image_index].size() || rint(coords.x * resx) < 0 || rint(coords.x * resx) >= scene.images[texture.image_index][0].size())
					{
						cout << "Texture coordinates out of bounds" << endl;
						cout << "coords: " << coords << endl;
						cout << rint(coords.y * resy) << " " << rint(coords.x * resx) << endl;
						exit(0);
					}
					first.normal = first.normal + scene.images[texture.image_index][rint(coords.y * resy)][rint(coords.x * resx)];
					//exit(0);
				}
				break;
			case Texture::all:
				{
					if (verbose)
						cout << "all texture" << endl;
					vector3 coords = first.get2dcoordinates();
					if (coords.x < 0 || coords.x > 1 || coords.y < 0 || coords.y > 1)
					{
						cout << "Texture coordinates out of bounds" << endl;
						cout << "coords: " << coords << endl;
						exit(1);
					}
					float posxf = (scene.images[texture.image_index][0].size()-1) * coords.x;
					float posyf = (scene.images[texture.image_index].size()-1) * coords.y;
					vector3 color(0,0,0);
					auto& image = scene.images[texture.image_index];
					switch (texture.interpolation)
					{
					case Texture::bilinear:
						{
						//bool coin = rand() % 2;
						float dx = fmodf(posxf, 1), dy = fmodf(posyf, 1);
						color = color + image[floor(posyf)][floor(posxf)] * (1-dx) * (1-dy);
						color = color + image[floor(posyf)][ceil(posxf)] * dx * (1-dy);
						color = color + image[ceil(posyf)][floor(posxf)] * (1-dx) * dy;
						color = color + image[ceil(posyf)][ceil(posxf)] * dx * dy;
						}
						break;
					
					default:
						break;
					}
					if (verbose)
					{
						cout << "texture.all: " << color << endl;
						cout << "coords: " << coords << endl;
					}
					return color;
				}
				break;
			case Texture::kd_replace:
				{
					if (verbose)
					{
						cout << "kd_replace texture" << endl;
					}
					vector3 coords = first.get2dcoordinates();
					if (coords.x < 0 || coords.x > 1 || coords.y < 0 || coords.y > 1)
					{
						cout << "Texture coordinates out of bounds" << endl;
						cout << "coords: " << coords << endl;
						exit(1);
					}
					float posxf = (scene.images[texture.image_index][0].size()-1) * coords.x;
					float posyf = (scene.images[texture.image_index].size()-1) * coords.y;
					vector3 color = texture.getcolor(coords, scene.images);
					if (verbose)
					{
						cout << "texture.kd: " << color << endl;
						cout << "coords: " << coords << endl;
					}
					k_diffuse = color / texture.normalizer;
				}
				break;
			case Texture::kd_blend:
				{
					if (verbose)
					{
						cout << "kd_blend texture" << endl;
					}
					vector3 coords = first.get2dcoordinates();
					if (coords.x < 0 || coords.x > 1 || coords.y < 0 || coords.y > 1)
					{
						cout << "Texture coordinates out of bounds" << endl;
						cout << "coords: " << coords << endl;
						exit(1);
					}
					float posxf = (scene.images[texture.image_index][0].size()-1) * coords.x;
					float posyf = (scene.images[texture.image_index].size()-1) * coords.y;
					vector3 color = texture.getcolor(coords, scene.images);
					if (verbose)
					{
						cout << "texture.kd: " << color << endl;
						cout << "coords: " << coords << endl;
					}
					k_diffuse = color / texture.normalizer / 2 + k_diffuse / 2;
				}
				break;
			default:
				{
					static bool printed = false;
					if (!printed)
						cout << "Texture type not implemented" << texture.type << endl,
						printed = true;
				}
				break;
			}
		}

		if (first.getobject()->getmaterial().is_mirror())
		{
			if (depth == scene.max_recursion_depth)	goto label;
			mirrorwo = ray.start +- first.position;
			reflection = mirrorwo + (first.normal * dot(mirrorwo, first.normal) +- mirrorwo) * 2.f;
			if (verbose)
			{
				cout << "normal: " << first.normal << endl;
				cout << first.position + correction;
				cout << "reflection: " << reflection << endl;
			}
			reflectionray = {first.position + correction, reflection};
			//mirror = piecewise(first.object_pointer->getmaterial().mirror, sendray(reflectionray, verbose, depth + 1, pathlength));
			if (verbose)
			{
				cout << "sending mirror ray, depth: " << depth << endl;
			}
			mirror = sendray(scene, reflectionray, verbose, depth + 1, pathlength, {0,0,false}, xsystem);
			vsMul(3, (float*)&first.getobject()->getmaterial().mirror, (float*)&mirror, (float*)&mirror);
		}
		if (first.getobject()->getmaterial().is_dielectric())
		{
			if (depth == scene.max_recursion_depth)	goto label;
			costeta = dot(ray.getdirection(), first.normal) / norm(ray.getdirection());
			entering = costeta < 0;
			costeta = fabs(costeta);

			if (!entering)
				attenuation = first.getobject()->getmaterial().AbsorptionCoefficient * norm(ray.start +- first.position),
				attenuation = {exp(-attenuation.x), exp(-attenuation.y), exp(-attenuation.z)};

			if (depth == scene.max_recursion_depth)	goto label;

			if (entering)	nratio = 1 / first.getobject()->getmaterial().refraction_index, normal = first.normal, n1 = 1, n2 = first.getobject()->getmaterial().refraction_index;
			else	nratio = first.getobject()->getmaterial().refraction_index, normal = -first.normal, n1 = first.getobject()->getmaterial().refraction_index, n2 = 1;

			correction = -normal * scene.shadow_ray_epsilon;

			cosphi = 1 - powf(nratio, 2) * (1 - powf(costeta, 2));
			if (verbose)
			{
				//cout << "cosphi: " << cosphi << endl;
				if (entering)	cout << "Ray is entering the object" << endl;
				else if (cosphi < 0)	cout << "Ray is redlecting inside the object" << endl;
				else	cout << "Ray is exiting the object" << endl;
			}
			if (cosphi < 0)
			{
				correction = normal * scene.shadow_ray_epsilon;

				mirrorwo = ray.start +- first.position;
				reflection = mirrorwo + (first.normal * dot(mirrorwo, first.normal) +- mirrorwo) * 2.f;
				reflectionray = {first.position + correction, reflection};

				mirror = sendray(scene, reflectionray, verbose, depth + 1, pathlength, {0,0,false}, xsystem);
				goto label;
			}
			cosphi = sqrt(cosphi);

			rparallel = (n2*costeta - n1 * cosphi) / (n2 * costeta + n1 * cosphi);
			rperpendicular = (n1 * costeta - n2 * cosphi) / (n1 * costeta + n2 * cosphi);
			fr = (powf(rparallel, 2) + powf(rperpendicular, 2)) / 2;	ft = 1 - fr;
			
			d = ray.getdirection().normalize();	//I think this is normalized by default, so no need to normalize again
			refractionray = {first.position + correction , (d + normal * costeta) * nratio +- normal * cosphi};
			dielectric = sendray(scene, refractionray, verbose, depth + 1, pathlength, {0,0,false}, xsystem) * ft;
			//dielectric = piecewise(sendray(refractionray, verbose, depth + 1, pathlength), attenuation) * ft;
			vsMul(3, (float*)&attenuation, (float*)&dielectric, (float*)&dielectric);

			mirrorwo = ray.start +- first.position;
			reflection = mirrorwo + (first.normal * dot(mirrorwo, first.normal) +- mirrorwo) * 2.f;
			reflectionray = {first.position +- correction, reflection};
			mirror = sendray(scene, reflectionray, verbose, depth + 1, pathlength, {0,0,false}, xsystem) * fr;
		}

		label:

		if (scene.pathtracing)
		{
			if (first.getobject()->islightsource())
			{
				//wi = ray.start +- first.position;
				//vector3 radiance = first.getobject()->getradiance();
				return first.getobject()->getradiance();	// / pow(norm(first.position-ray.start), 2) * abs(dot(first.normal, normalized(ray.getdirection())));
			}
			else if (depth<scene.max_recursion_depth)
			{
				int nee_count=0;
				if (scene.current_camera->nee)
				{
					for (auto&pointlight:scene.point_lights)
					{
						_PointLight light(pointlight.position, pointlight.intensity);
						Ray newray = light.sample_ray(first.position + correction);
						second = intersect(scene, newray);
						if (!second.intersecting)
						{
							irradiance = light.getradiance(newray);
							wi = newray.getend() +- first.position;
							{
								vector3 diff = k_diffuse*cosclamp(first.normal, wi);
								vsMul(3, (float*)&irradiance, (float*)&diff, (float*)&diff);
								diffuse = diffuse + diff;	//vsAdd could be used here
							}
							normalized(wi, wi);
							normalized(wo, wo);
							h = (wi+wo);
							specularcos = cosclamp(first.normal, h);
							{
								vector3 spec = first.getobject()->getmaterial().specular * powf(specularcos, first.getobject()->getmaterial().phong_exponent);
								vsMul(3, (float*)&irradiance, (float*)&spec, (float*)&spec);
								vsAdd(3, (float*)&specular, (float*)&spec, (float*)&specular);
							}
							//tree->paths.emplace_back(light.position);
						}
						//else	path->paths.emplace_back(second.position);
						nee_count++;
					}
					for (auto&mesh:scene.meshes) if (mesh.is_lightsource)
					{
						Ray newray(first.position + correction, mesh.center - first.position);
						if (verbose)
						{
							cout << "sending environment ray, depth: " << depth << endl;
						}

						vector3 radiance = sendray(scene, newray, verbose, depth+1, pathlength, pixel, xsystem) / M_PI;
						wi = newray.getdirection().normalize();
						{
							vector3 diff = k_diffuse*cosclamp(first.normal, wi);
							vsMul(3, (float*)&radiance, (float*)&diff, (float*)&diff);
							diffuse = diffuse + diff;	//vsAdd could be used here
						}
						nee_count++;
					}
				}

				int splitcount;		if (depth)	splitcount=1;	else	splitcount=scene.current_camera->splitCount;
				for (int splitindex=0; splitindex<splitcount; splitindex++)
				{
					//uniformly sample the hemisphere
					vector3 w = first.normal, u = perp_dir(w).normalize(), v = w.cross(u).normalize();
					float xz = drand48() * 2 * M_PI, h = drand48(), r = sqrt(1-h*h);
					vector3 d = u * cos(xz) * r + v * sin(xz) * r + w * h;
					Ray newray(first.position + correction, d);

					Path*path = nullptr;
					if (verbose)
					{
						cout << "sending environment ray, depth: " << depth << endl;
						path = &tree->paths.emplace_back(test(*xsystem, first.position), 0x0000FF);
					}
					
					vector3 radiance = sendray(scene, newray, verbose, depth+1, pathlength, pixel, xsystem, path) / M_PI;
					wi = newray.getdirection().normalize();
					{
						vector3 diff = k_diffuse*cosclamp(first.normal, wi);
						vsMul(3, (float*)&radiance, (float*)&diff, (float*)&diff);
						diffuse = diffuse + diff;	//vsAdd could be used here
					}
				}

				return diffuse / (splitcount + nee_count) + mirror + dielectric;
			}
			else return {0,0,0};
		}

		if (!scene.pathtracing)
		for (auto &pointlight : scene.point_lights)
		{
			_PointLight light(pointlight.position, pointlight.intensity);
			Ray newray = light.sample_ray(first.position + correction);

			second = intersect(scene, newray);
			switch (second.intersecting)
			{
			case false:	//object is not in shadow
				if (verbose)
				{
					cout << "object is not in shadow" << endl;
					auto pixel2 = test(*xsystem, newray.getend());
					XSetForeground(xsystem->display, xsystem->gc, 0x00FF00);
					XDrawLine(xsystem->display, xsystem->window, xsystem->gc, shadow_start.x, shadow_start.y, pixel2.x, pixel2.y);
				}
				//pathlength += objecttolight;
				irradiance = light.getradiance(newray);
				wi = newray.getend() +- first.position;
				{
					vector3 diff = k_diffuse*cosclamp(first.normal, wi);
					vsMul(3, (float*)&irradiance, (float*)&diff, (float*)&diff);
					diffuse = diffuse + diff;	//vsAdd could be used here
				}
				normalized(wi, wi);
				normalized(wo, wo);
				h = (wi+wo);
				specularcos = cosclamp(first.normal, h);
				{
					vector3 spec = first.getobject()->getmaterial().specular * powf(specularcos, first.getobject()->getmaterial().phong_exponent);
					vsMul(3, (float*)&irradiance, (float*)&spec, (float*)&spec);
					vsAdd(3, (float*)&specular, (float*)&spec, (float*)&specular);
				}

				break;
			case true:	//object is in shadow
				//Do nothing, basically (since we are not adding anything to the color)
				if (verbose)
				{
					std::cout << "object is in shadow of " << second.getobject()->say() << " " << second.getobject()->getid() << endl;
					auto pixel2 = test(*xsystem, second.position);
					XSetForeground(xsystem->display, xsystem->gc, 0xFF0000);
					XDrawLine(xsystem->display, xsystem->window, xsystem->gc, shadow_start.x, shadow_start.y, pixel2.x, pixel2.y);
				}
			}
		}
		
		if (!scene.pathtracing)
		for (auto &arealight : scene.area_lights)
		{
			_AreaLight light(arealight.position, arealight.normal, arealight.size, arealight.radiance);

			Ray newray = light.sample_ray(first.position + correction);

			vector3 vectortolight = newray.getdirection();
			second = intersect(scene, newray);
			switch (second.intersecting)
			{
			case false:	//object is not in shadow
				if (verbose)
				{
					cout << "object is not in shadow" << endl;
					cout << "first.normal: " << first.normal << endl;
					auto pixel2 = test(*xsystem, newray.getend());
					XSetForeground(xsystem->display, xsystem->gc, 0x00FF00);
					XDrawLine(xsystem->display, xsystem->window, xsystem->gc, shadow_start.x, shadow_start.y, pixel2.x, pixel2.y);
				}
				irradiance = light.getradiance(newray);

				vectortolight.normalize();
				wi = vectortolight;
				{
					vector3 diff = k_diffuse*cosclamp(first.normal, wi);
					vsMul(3, (float*)&irradiance, (float*)&diff, (float*)&diff);
					diffuse = diffuse + diff;	//vsAdd could be used here
				}
				normalized(wi, wi);
				normalized(wo, wo);
				h = (wi+wo);
				specularcos = cosclamp(first.normal, h);
				{
					vector3 spec = first.getobject()->getmaterial().specular * powf(specularcos, first.getobject()->getmaterial().phong_exponent);
					vsMul(3, (float*)&irradiance, (float*)&spec, (float*)&spec);
					vsAdd(3, (float*)&specular, (float*)&spec, (float*)&specular);
				}
				break;
			case true:	//object is in shadow
				//Do nothing, basically (since we are not adding anything to the color)
				if (verbose)
				{
					std::cout << "object is in shadow of " << second.getobject()->say() << " " << second.getobject()->getid() << endl;
					auto pixel2 = test(*xsystem, second.position);
					XSetForeground(xsystem->display, xsystem->gc, 0xFF0000);
					XDrawLine(xsystem->display, xsystem->window, xsystem->gc, shadow_start.x, shadow_start.y, pixel2.x, pixel2.y);
				}
			}
		}
		
		if (!scene.pathtracing)
		for (auto &dirlight : scene.directional_lights)
		{
			_DirectionalLight light(dirlight.xzangl, dirlight.yangl, dirlight.radiance);
			Ray sray = light.sample_ray(first.position + correction);
			second = intersect(scene, sray);
			switch (second.intersecting)
			{
			case false:	//object is not in shadow
				if (verbose)
				{
					cout << "object is not in shadow" << endl;
				}
				irradiance = light.getradiance(sray);
				wi = sray.getdirection();
				{
					vector3 diff = k_diffuse*cosclamp(first.normal, wi);
					vsMul(3, (float*)&irradiance, (float*)&diff, (float*)&diff);
					diffuse = diffuse + diff;	//vsAdd could be used here
				}
				normalized(wi, wi);
				normalized(wo, wo);
				h = (wi+wo);
				specularcos = cosclamp(first.normal, h);
				{
					vector3 spec = first.getobject()->getmaterial().specular * powf(specularcos, first.getobject()->getmaterial().phong_exponent);
					vsMul(3, (float*)&irradiance, (float*)&spec, (float*)&spec);
					vsAdd(3, (float*)&specular, (float*)&spec, (float*)&specular);
				}
				break;
			case true:	//object is in shadow
				//Do nothing, basically (since we are not adding anything to the color)
				if (verbose)
				{
					std::cout << "object is in shadow of " << second.getobject()->say() << " " << second.getobject()->getid() << endl;
				}
			}
		}
		
		if (!scene.pathtracing)
		for (auto &spotlight : scene.spot_lights)
		{
			_SpotLight light(spotlight.position, spotlight.direction, spotlight.coverage_angle, spotlight.falloff_angle, spotlight.intensity);
			Ray shadowray = light.sample_ray(first.position + correction);

			second = intersect(scene, shadowray);

			float cos_spot_angle = cos_vv(spotlight.direction, -shadowray.getdirection());
			float multiplier = 1;

			switch (second.intersecting)
			{
			case false:	//object is not in shadow
				if (verbose)
					cout << "object is not in shadow" << endl;
				irradiance = light.getradiance(shadowray);
				wi = spotlight.position +- first.position;
				{
					vector3 diff = k_diffuse*cosclamp(first.normal, wi);
					vsMul(3, (float*)&irradiance, (float*)&diff, (float*)&diff);
					diffuse = diffuse + diff;	//vsAdd could be used here
				}
				normalized(wi, wi);
				normalized(wo, wo);
				h = (wi+wo);
				specularcos = cosclamp(first.normal, h);
				{
					vector3 spec = first.getobject()->getmaterial().specular * powf(specularcos, first.getobject()->getmaterial().phong_exponent);
					vsMul(3, (float*)&irradiance, (float*)&spec, (float*)&spec);
					vsAdd(3, (float*)&specular, (float*)&spec, (float*)&specular);
				}
				break;
			case true:	//object is in shadow
				//Do nothing, basically (since we are not adding anything to the color)
				if (second.getobject()->getmaterial().is_dielectric())
				{
					if (verbose)
					{
						std::cout << "object is in shadow of dielectric " << second.getobject()->say() << endl
								  << "second.position: " << second.position << endl;
					}
				}
				else if (verbose)
				{
					std::cout << "object is in shadow of " << second.getobject()->say() << " " << second.getobject()->getid() << endl;
				}
			}
		}
		
		if (!scene.pathtracing)
		if (scene.background_type != Scene::color)
		{
			auto normal = first.normal;
			auto xz = drand48() * 2 * M_PI, y = drand48();
			y *= 1-y;	y*=M_PI/2;
			vector3 perp = perp_dir(normal);
			Ray shadowray = {first.position + correction, first.position + first.normal * cos(y) + perp * cos(xz) * sin(y) + normal.cross(perp) * sin(xz) * sin(y)};
			second = intersect(scene, shadowray);
			vector3 coords;
			float u,v;
			int resx = scene.images[scene.background.image_indice][0].size()-1, resy = scene.images[scene.background.image_indice].size()-1;	// -1 because the last index is the size of the array; this could be done better by mapping the texture better.
			switch (second.intersecting)
			{
			case false:
				if (verbose)
					cout << "object is not in shadow" << endl;
				
				if (scene.background_type == Scene::latlong)
					coords = sphere2dlatlong(first.position, first.position + shadowray.getdirection(), 1);
				else if (scene.background_type == Scene::probe)
					coords = sphere2dprobe(first.position, first.position + shadowray.getdirection(), 1);
				u = coords.x, v = coords.y;
				if (rint(v * resy) < 0 || rint(v * resy) >= scene.images[scene.background.image_indice].size() || rint(u * resx) < 0 || rint(u * resx) >= scene.images[scene.background.image_indice][0].size())
				{
					cout << "Texture coordinates out of bounds" << endl;
					cout << "coords: " << u << " " << v << endl;
					cout << rint(u * resx) << " " << rint(v * resy) << endl;
					exit(1);
				}
				irradiance = scene.images[scene.background.image_indice][rint(v * resy)][rint(u * resx)] * 2*M_PI;

				wi = shadowray.getdirection();
				{
					vector3 diff = k_diffuse*cosclamp(first.normal, wi);
					vsMul(3, (float*)&irradiance, (float*)&diff, (float*)&diff);
					diffuse = diffuse + diff;	//vsAdd could be used here
				}

				normalized(wi, wi);
				normalized(wo, wo);
				h = (wi+wo);
				specularcos = cosclamp(first.normal, h);

				//specular = specular + piecewise(first.object_pointer->getmaterial().specular, irradiance * powf(specularcos, first.object_pointer->getmaterial().phong_exponent));
				{
					vector3 spec = first.getobject()->getmaterial().specular * powf(specularcos, first.getobject()->getmaterial().phong_exponent);
					vsMul(3, (float*)&irradiance, (float*)&spec, (float*)&spec);
					vsAdd(3, (float*)&specular, (float*)&spec, (float*)&specular);
				}
				break;
			case true:
				if (verbose)
					std::cout << "object is in shadow of " << second.getobject()->say() << " " << second.getobject()->getid() << endl;
			}
		}

		if (verbose)
		{
			cout << "ambient: " << ambient << endl;
			cout << "diffuse: " << diffuse << endl;
			cout << "specular: " << specular << endl;
			cout << "mirror: " << mirror << endl;
			cout << "dielectric: " << dielectric << endl;
			//cout << "object is " << objecttolight << "meters to light" << endl;
			cout << "irradiance: " << irradiance << endl;
		}

		return ambient + diffuse + specular + mirror + dielectric;
	}
	cout << "This should not be reached" << endl;
	exit(1);
	return {0,0,0};
}
vector3 clamp(vector3 color)
{
	static const vector3 maxcolor{255,255,255};
	vector3 result;
	vsFmin(3, (float*)&maxcolor, &color, &result);
	return result;
}