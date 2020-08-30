/****************************************************************************
 *      This is part of the libYafaRay package
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <core_api/volume.h>
#include <core_api/logging.h>
#include <core_api/ray.h>
#include <core_api/color.h>

BEGIN_YAFRAY

Rgb DensityVolume::tau(const Ray &ray, float step_size, float offset)
{
	float t_0 = -1, t_1 = -1;

	// ray doesn't hit the BB
	if(!intersect(ray, t_0, t_1))
	{
		return Rgb(0.f);
	}

	if(ray.tmax_ < t_0 && !(ray.tmax_ < 0)) return Rgb(0.f);

	if(ray.tmax_ < t_1 && !(ray.tmax_ < 0)) t_1 = ray.tmax_;

	if(t_0 < 0.f) t_0 = 0.f;

	// distance travelled in the volume
	float step = step_size; // length between two sample points along the ray
	float pos = t_0 + offset * step;
	Rgb tau_val(0.f);


	Rgb tau_prev(0.f);

	bool adaptive = false;

	while(pos < t_1)
	{
		Rgb tau_tmp = sigmaT(ray.from_ + (ray.dir_ * pos), ray.dir_);


		if(adaptive)
		{

			float epsilon = 0.01f;

			if(std::fabs(tau_tmp.energy() - tau_prev.energy()) > epsilon && step > step_size / 50.f)
			{
				pos -= step;
				step /= 2.f;
			}
			else
			{
				tau_val += tau_tmp * step;
				tau_prev = tau_tmp;
			}
		}
		else
		{
			tau_val += tau_tmp * step;
		}

		pos += step;
	}

	return tau_val;
}

inline float min__(float a, float b) { return (a > b) ? b : a; }
inline float max__(float a, float b) { return (a < b) ? b : a; }

inline double cosInter__(double y_1, double y_2, double mu)
{
	double mu_2;

	mu_2 = (1.0f - fCos__(mu * M_PI)) / 2.0f;
	return y_1 * (1.0f - mu_2) + y_2 * mu_2;
}

float VolumeRegion::attenuation(const Point3 p, Light *l)
{
	if(attenuation_grid_map_.find(l) == attenuation_grid_map_.end())
	{
		Y_WARNING << "VolumeRegion: Attenuation Map is missing" << YENDL;
	}

	float *attenuation_grid = attenuation_grid_map_[l];

	float x = (p.x_ - b_box_.a_.x_) / b_box_.longX() * att_grid_x_ - 0.5f;
	float y = (p.y_ - b_box_.a_.y_) / b_box_.longY() * att_grid_y_ - 0.5f;
	float z = (p.z_ - b_box_.a_.z_) / b_box_.longZ() * att_grid_z_ - 0.5f;

	//Check that the point is within the bounding box, return 0 if outside the box
	if(x < -0.5f || y < -0.5f || z < -0.5f) return 0.f;
	else if(x > (att_grid_x_ - 0.5f) || y > (att_grid_y_ - 0.5f) || z > (att_grid_z_ - 0.5f)) return 0.f;

	// cell vertices in which p lies
	int x_0 = max__(0, floor(x));
	int y_0 = max__(0, floor(y));
	int z_0 = max__(0, floor(z));

	int x_1 = min__(att_grid_x_ - 1, ceil(x));
	int y_1 = min__(att_grid_y_ - 1, ceil(y));
	int z_1 = min__(att_grid_z_ - 1, ceil(z));

	// offsets
	float xd = x - x_0;
	float yd = y - y_0;
	float zd = z - z_0;

	// trilinear combination
	float i_1 = attenuation_grid[x_0 + y_0 * att_grid_x_ + att_grid_y_ * att_grid_x_ * z_0] * (1 - zd) + attenuation_grid[x_0 + y_0 * att_grid_x_ + att_grid_y_ * att_grid_x_ * z_1] * zd;
	float i_2 = attenuation_grid[x_0 + y_1 * att_grid_x_ + att_grid_y_ * att_grid_x_ * z_0] * (1 - zd) + attenuation_grid[x_0 + y_1 * att_grid_x_ + att_grid_y_ * att_grid_x_ * z_1] * zd;
	float j_1 = attenuation_grid[x_1 + y_0 * att_grid_x_ + att_grid_y_ * att_grid_x_ * z_0] * (1 - zd) + attenuation_grid[x_1 + y_0 * att_grid_x_ + att_grid_y_ * att_grid_x_ * z_1] * zd;
	float j_2 = attenuation_grid[x_1 + y_1 * att_grid_x_ + att_grid_y_ * att_grid_x_ * z_0] * (1 - zd) + attenuation_grid[x_1 + y_1 * att_grid_x_ + att_grid_y_ * att_grid_x_ * z_1] * zd;

	float w_1 = i_1 * (1 - yd) + i_2 * yd;
	float w_2 = j_1 * (1 - yd) + j_2 * yd;

	float att = w_1 * (1 - xd) + w_2 * xd;

	return att;
}

END_YAFRAY

