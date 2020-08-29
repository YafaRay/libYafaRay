#include <yafray_constants.h>

#include <core_api/ray.h>
#include <core_api/color.h>
#include <core_api/volume.h>
#include <core_api/bound.h>
#include <core_api/surface.h>
#include <core_api/texture.h>
#include <core_api/environment.h>
#include <core_api/params.h>
#include <utilities/mcqmc.h>

#include <fstream>
#include <cstdlib>

BEGIN_YAFRAY

struct RenderState;
struct PSample;

class GridVolume final : public DensityVolume
{
	public:
		GridVolume(Rgb sa, Rgb ss, Rgb le, float gg, Point3 pmin, Point3 pmax)
		{
			b_box_ = Bound(pmin, pmax);
			s_a_ = sa;
			s_s_ = ss;
			l_e_ = le;
			g_ = gg;
			have_s_a_ = (s_a_.energy() > 1e-4f);
			have_s_s_ = (s_s_.energy() > 1e-4f);
			have_l_e_ = (l_e_.energy() > 1e-4f);

			std::ifstream input_stream;
			input_stream.open("/home/public/3dkram/cloud2_3.df3");
			if(!input_stream) Y_ERROR << "GridVolume: Error opening input stream" << YENDL;

			input_stream.seekg(0, std::ios_base::beg);
			std::ifstream::pos_type begin_pos = input_stream.tellg();
			input_stream.seekg(0, std::ios_base::end);
			int file_size = static_cast<int>(input_stream.tellg() - begin_pos);
			file_size -= 6;
			input_stream.seekg(0, std::ios_base::beg);

			int dim[3];
			for(int i = 0; i < 3; ++i)
			{
				short i_0 = 0, i_1 = 0;
				input_stream.read((char *)&i_0, 1);
				input_stream.read((char *)&i_1, 1);
				Y_VERBOSE << "GridVolume: " << i_0 << " " << i_1 << YENDL;
				dim[i] = (((unsigned short)i_0 << 8) | (unsigned short)i_1);
			}

			int size_per_voxel = file_size / (dim[0] * dim[1] * dim[2]);

			Y_VERBOSE << "GridVolume: " << dim[0] << " " << dim[1] << " " << dim[2] << " " << file_size << " " << size_per_voxel << YENDL;

			size_x_ = dim[0];
			size_y_ = dim[1];
			size_z_ = dim[2];

			/*
			sizeX = 60;
			sizeY = 60;
			sizeZ = 60;
			*/

			grid_ = (float ***)malloc(size_x_ * sizeof(float));
			for(int x = 0; x < size_x_; ++x)
			{
				grid_[x] = (float **)malloc(size_y_ * sizeof(float));
				for(int y = 0; y < size_y_; ++y)
				{
					grid_[x][y] = (float *)malloc(size_z_ * sizeof(float));
				}
			}

			for(int z = 0; z < size_z_; ++z)
			{
				for(int y = 0; y < size_y_; ++y)
				{
					for(int x = 0; x < size_x_; ++x)
					{
						int voxel = 0;
						input_stream.read((char *)&voxel, 1);
						grid_[x][y][z] = voxel / 255.f;
						/*
						float r = sizeX / 2.f;
						float r2 = r*r;
						float dist = sqrt((x-r)*(x-r) + (y-r)*(y-r) + (z-r)*(z-r));
						grid[x][y][z] = (dist < r) ? 1.f-dist/r : 0.0f;
						*/
					}
				}
			}

			Y_VERBOSE << "GridVolume: Vol.[" << s_a_ << ", " << s_s_ << ", " << l_e_ << "]" << YENDL;
		}

		~GridVolume()
		{
			Y_VERBOSE << "GridVolume: Freeing grid data" << YENDL;

			for(int x = 0; x < size_x_; ++x)
			{
				for(int y = 0; y < size_y_; ++y)
				{
					free(grid_[x][y]);
				}
				free(grid_[x]);
			}
			free(grid_);
		}

		virtual float density(Point3 p);

		static VolumeRegion *factory(ParamMap &params, RenderEnvironment &render);

	private:
		float ***grid_ = nullptr;
		int size_x_, size_y_, size_z_;
};

inline float min__(float a, float b) { return (a > b) ? b : a; }
inline float max__(float a, float b) { return (a < b) ? b : a; }


float GridVolume::density(Point3 p)
{
	float x = (p.x_ - b_box_.a_.x_) / b_box_.longX() * size_x_ - .5f;
	float y = (p.y_ - b_box_.a_.y_) / b_box_.longY() * size_y_ - .5f;
	float z = (p.z_ - b_box_.a_.z_) / b_box_.longZ() * size_z_ - .5f;

	int x_0 = max__(0, floor(x));
	int y_0 = max__(0, floor(y));
	int z_0 = max__(0, floor(z));

	int x_1 = min__(size_x_ - 1, ceil(x));
	int y_1 = min__(size_y_ - 1, ceil(y));
	int z_1 = min__(size_z_ - 1, ceil(z));

	float xd = x - x_0;
	float yd = y - y_0;
	float zd = z - z_0;

	float i_1 = grid_[x_0][y_0][z_0] * (1 - zd) + grid_[x_0][y_0][z_1] * zd;
	float i_2 = grid_[x_0][y_1][z_0] * (1 - zd) + grid_[x_0][y_1][z_1] * zd;
	float j_1 = grid_[x_1][y_0][z_0] * (1 - zd) + grid_[x_1][y_0][z_1] * zd;
	float j_2 = grid_[x_1][y_1][z_0] * (1 - zd) + grid_[x_1][y_1][z_1] * zd;

	float w_1 = i_1 * (1 - yd) + i_2 * yd;
	float w_2 = j_1 * (1 - yd) + j_2 * yd;

	float dens = w_1 * (1 - xd) + w_2 * xd;

	return dens;
}

VolumeRegion *GridVolume::factory(ParamMap &params, RenderEnvironment &render)
{
	float ss = .1f;
	float sa = .1f;
	float le = .0f;
	float g = .0f;
	float min[] = {0, 0, 0};
	float max[] = {0, 0, 0};
	params.getParam("sigma_s", ss);
	params.getParam("sigma_a", sa);
	params.getParam("l_e", le);
	params.getParam("g", g);
	params.getParam("minX", min[0]);
	params.getParam("minY", min[1]);
	params.getParam("minZ", min[2]);
	params.getParam("maxX", max[0]);
	params.getParam("maxY", max[1]);
	params.getParam("maxZ", max[2]);

	GridVolume *vol = new GridVolume(Rgb(sa), Rgb(ss), Rgb(le), g,
									 Point3(min[0], min[1], min[2]), Point3(max[0], max[1], max[2]));
	return vol;
}

extern "C"
{
	YAFRAYPLUGIN_EXPORT void registerPlugin__(RenderEnvironment &render)
	{
		render.registerFactory("GridVolume", GridVolume::factory);
	}
}

END_YAFRAY
