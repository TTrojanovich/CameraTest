#define tag3
#ifdef tag3

#include <limits>
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include "geometry.h"

#define PI 3.1415
using namespace std;


struct Light
{
	Light(const Vec3f &p, const float &i) : position(p), intensity(i) {}
	Vec3f position;
	float intensity;
};


struct Material
{
	Material(const Vec3f &color) : diffuse_color(color) {}
	Material() : diffuse_color() {}
	Vec3f diffuse_color;
};


struct Sphere
{
	Vec3f center;
	float radius;
	Material material;

	Sphere(const Vec3f &c, const float &r, const Material &m) : center(c), radius(r), material(m) {}

	bool ray_intersect(const Vec3f &orig, const Vec3f &dir, float &VecDownToHit) const
	{
		Vec3f center2 = center - orig;
		float VecDownToProj = center2 * dir; // нашли длину нижнего вектора orig -> pc
		if (VecDownToProj < 0) return false;
		float PerpLen = center2 * center2 - VecDownToProj * VecDownToProj; // длина перпендикул€ра из центра сферы в pc (нужно вз€ть корень)
		if (PerpLen > radius * radius) return false;
		float HitProjLen = sqrtf(radius * radius - PerpLen); // нашли кусочек между первой точкой пересечени€ i1 и pc
		VecDownToHit = VecDownToProj - HitProjLen; // длина вектора от orig до i1
		return true;
	}
};


bool scene_intersect(const Vec3f &orig, const Vec3f &dir, const vector<Sphere> &spheres, Vec3f &hit, Vec3f &Normal, Material &material)
{
	float maxLimit = numeric_limits<float>::max();
	float spheres_dist = maxLimit;
	for (unsigned int i = 0; i < spheres.size(); i++)
	{
		float dist_i;
		if (spheres[i].ray_intersect(orig, dir, dist_i) && dist_i < spheres_dist)
		{
			spheres_dist = dist_i;
			hit = orig + dir * dist_i; // видимо перва€ точка пересечени€ сферы лучом
			Normal = (hit - spheres[i].center).normalize(); // нормаль к поверхности
			material = spheres[i].material; // если текуща€ рассматриваема€ сфера ближе предыдущей, мен€ем цвет окраса на ее
		}
	}

	float plane_dist = maxLimit;

	float dist = -(orig.y + 4) / dir.y;
	Vec3f hit_temp = orig + dir * dist;

	if (dist > 0 && dist < spheres_dist && fabs(hit_temp.x) < 10 && hit_temp.z < 10 && hit_temp.z > -30)
	{
		plane_dist = dist;
		hit = hit_temp;
		Normal = Vec3f(0, 1, 0);

		int c1 = int(hit.x + 10);
		int c2 = int(hit.z + 30);

		if (c1 % 2 != 0) c1--;
		if (c2 % 2 != 0) c2--;

		if ((((c1 / 2) & 1) == 0) && (((c2 / 2) & 1) == 0)) material.diffuse_color = Vec3f(1, 1, 1);
		else if (((c1 / 2) & 1) && ((c2 / 2) & 1)) material.diffuse_color = Vec3f(1, 1, 1);
		else material.diffuse_color = Vec3f(1.0f, 0.7f, 0.3f);
	}

	return min(spheres_dist, plane_dist) < maxLimit;
}


Vec3f cast_ray(const Vec3f &orig, const Vec3f &dir, const vector<Sphere> &spheres, const vector<Light> &lights)
{
	Vec3f hit, Normal;
	Material material;

	if (!scene_intersect(orig, dir, spheres, hit, Normal, material)) return Vec3f(0.2f, 0.7f, 0.8f);

	float diffuse_light_intensity = 0;

	for (unsigned int i = 0; i < lights.size(); i++)
	{
		Vec3f light_dir = (lights[i].position - hit).normalize();
		diffuse_light_intensity += lights[i].intensity * max(0.0f, light_dir * Normal);
	}

	return material.diffuse_color * diffuse_light_intensity;
}


Vec3f reflect(const Vec3f &I, const Vec3f &N)
{
	return I - N*2.f*(I*N);
}


float pos = 6.5, rad = 3;
Sphere mirror{ Vec3f(0, 0, 0), rad, Vec3f(1.0f, 1.0f, 1.0f) };

Vec3f reflection(const Vec3f &orig, const Vec3f &dir, const vector<Sphere> &spheres, const vector<Light> &lights)
{
	Vec3f hit, Normal;
	float dist_i;
	float maxLimit = numeric_limits<float>::max();

	if (mirror.ray_intersect(orig, dir, dist_i) && (dist_i < maxLimit))
	{
		hit = orig + dir * dist_i;
		Normal = (hit - mirror.center).normalize();

		Vec3f reflect_dir = reflect(dir, Normal).normalize();
		Vec3f reflect_orig = reflect_dir * Normal < 0 ? hit - Normal * (float)1e-3 : hit + Normal * (float)1e-3;
		Vec3f reflect_color = cast_ray(reflect_orig, reflect_dir, spheres, lights);

		return reflect_color;
	}
	else return Vec3f(0.0f, 0.0f, 0.0f);
}


void render(const vector<Sphere> &spheres, const vector<Light> &lights)
{
	int koe = 1;
	const int width = 128 * 4 * koe;
	const int height = 128 * 3 * koe;
	const int fov = (int)(PI / 2);

	vector<Vec3f> framebuffer_front(width * height);
	vector<Vec3f> framebuffer_back(width * height);
	vector<Vec3f> framebuffer_right(width * height);
	vector<Vec3f> framebuffer_left(width * height);

	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			float x_f = -(2 * (i + 0.5f) / (float)width - 1) * tan(fov / 2.0f) * width / (float)height;
			float y_f = -(2 * (j + 0.5f) / (float)height - 1) * tan(fov / 2.0f);

			float x_b = (2 * (i + 0.5f) / (float)width - 1) * tan(fov / 2.0f) * width / (float)height;
			float y_b = -(2 * (j + 0.5f) / (float)height - 1) * tan(fov / 2.0f);

			float z_r = -(2 * (i + 0.5f) / (float)width - 1) * tan(fov / 2.0f) * width / (float)height;
			float y_r = -(2 * (j + 0.5f) / (float)height - 1) * tan(fov / 2.0f);

			float z_l = (2 * (i + 0.5f) / (float)width - 1) * tan(fov / 2.0f) * width / (float)height;
			float y_l = -(2 * (j + 0.5f) / (float)height - 1) * tan(fov / 2.0f);

			Vec3f dir_front = Vec3f(x_f, y_f, -1).normalize();
			Vec3f dir_back = Vec3f(x_b, y_b, 1).normalize();
			Vec3f dir_right = Vec3f(1, y_r, z_r).normalize();
			Vec3f dir_left = Vec3f(-1, y_l, z_l).normalize();

			mirror.center = { 0, 0, -pos };
			framebuffer_front[i + j * width] = reflection(Vec3f(0, 0, 0), dir_front, spheres, lights);
			mirror.center = { 0, 0, pos };
			framebuffer_back[i + j * width] = reflection(Vec3f(0, 0, 0), dir_back, spheres, lights);
			mirror.center = { pos, 0, 0 };
			framebuffer_right[i + j * width] = reflection(Vec3f(0, 0, 0), dir_right, spheres, lights);
			mirror.center = { -pos, 0, 0 };
			framebuffer_left[i + j * width] = reflection(Vec3f(0, 0, 0), dir_left, spheres, lights);
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	ofstream imgFront;
	ofstream imgBack;
	ofstream imgRight;
	ofstream imgLeft;
	imgFront.open("pics/2_front.ppm");
	imgBack.open("pics/4_back.ppm");
	imgRight.open("pics/3_right.ppm");
	imgLeft.open("pics/1_left.ppm");
	imgFront << "P3\n" << width << " " << height << "\n255\n";
	imgBack << "P3\n" << width << " " << height << "\n255\n";
	imgRight << "P3\n" << width << " " << height << "\n255\n";
	imgLeft << "P3\n" << width << " " << height << "\n255\n";

	for (int i = 0; i < height * width; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			if (framebuffer_front[i][j] > 1) framebuffer_front[i][j] = 1;
			if (framebuffer_back[i][j] > 1) framebuffer_back[i][j] = 1;
			if (framebuffer_right[i][j] > 1) framebuffer_right[i][j] = 1;
			if (framebuffer_left[i][j] > 1) framebuffer_left[i][j] = 1;
			imgFront << (int)(255 * framebuffer_back[i][j]) << " ";
			imgBack << (int)(255 * framebuffer_front[i][j]) << " ";
			imgRight << (int)(255 * framebuffer_left[i][j]) << " ";
			imgLeft << (int)(255 * framebuffer_right[i][j]) << " ";
		}
		imgFront << endl;
		imgBack << endl;
		imgRight << endl;
		imgLeft << endl;
	}
	imgFront.close();
	imgBack.close();
	imgRight.close();
	imgLeft.close();
	////////////////////////////////////////////////////////////////////////////////////////////////////
}

int main()
{
	Material ivory(Vec3f(0.4f, 0.4f, 0.3f));
	Material red_rubber(Vec3f(0.3f, 0.1f, 0.1f));
	Material yellow(Vec3f(1.0f, 1.0f, 0.3f));
	Material green(Vec3f(0.0f, 1.0f, 0.0f));
	Material blue(Vec3f(0.0f, 0.0f, 1.0f));

	vector<Sphere> spheres;
	spheres.push_back(Sphere(Vec3f(-3, 0, -16), 2, ivory));
	spheres.push_back(Sphere(Vec3f(-1.0, -1.5, -12), 2, red_rubber));
	spheres.push_back(Sphere(Vec3f(1.0, -0.5, -18), 3, red_rubber));
	spheres.push_back(Sphere(Vec3f(7, 5, -18), 4, ivory));

	spheres.push_back(Sphere(Vec3f(-10, 2, 0), 2, yellow));
	spheres.push_back(Sphere(Vec3f(10, 2, 0), 2, green));
	spheres.push_back(Sphere(Vec3f(0, 2, 20), 2, blue));

	vector<Light> lights;
	lights.push_back(Light(Vec3f(-20, 20, 15), 1.3f));

	render(spheres, lights);
}


#endif










