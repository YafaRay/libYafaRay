#ifndef Y_BASICTEX_H
#define Y_BASICTEX_H

#include <core_api/texture.h>
#include <core_api/environment.h>
#include <textures/noise.h>
#include <utilities/buffer.h>

__BEGIN_YAFRAY

class textureClouds_t : public texture_t
{
	public:
		textureClouds_t(int dep, float sz, bool hd,
				const color_t &c1, const color_t &c2,
				const std::string &ntype, const std::string &btype);
		virtual ~textureClouds_t();
		virtual colorA_t getColor(const point3d_t &p, bool from_postprocessed=false) const;
		virtual float getFloat(const point3d_t &p) const;
		
		virtual void getInterpolationStep(float &step) const { step = size; };

		static texture_t *factory(paraMap_t &params,renderEnvironment_t &render);
	protected:
		int depth, bias;
		float size;
		bool hard;
		color_t color1, color2;
		noiseGenerator_t* nGen;
};


class textureMarble_t : public texture_t
{
	public:
		textureMarble_t(int oct, float sz, const color_t &c1, const color_t &c2,
				float _turb, float shp, bool hrd, const std::string &ntype, const std::string &shape);
		virtual ~textureMarble_t()
		{
			if (nGen) {
				delete nGen;
				nGen = nullptr;
			}
		}

		virtual colorA_t getColor(const point3d_t &p, bool from_postprocessed=false) const;
		virtual float getFloat(const point3d_t &p) const;
		
		virtual void getInterpolationStep(float &step) const { step = size; };

		static texture_t *factory(paraMap_t &params,renderEnvironment_t &render);
	protected:
		int octaves;
		color_t color1, color2;
		float turb, sharpness, size;
		bool hard;
		noiseGenerator_t* nGen;
		enum {SIN, SAW, TRI} wshape;
};

class textureWood_t : public texture_t
{
	public:
		textureWood_t(int oct, float sz, const color_t &c1, const color_t &c2, float _turb,
				bool hrd, const std::string &ntype, const std::string &wtype, const std::string &shape);
		virtual ~textureWood_t()
		{
			if (nGen) {
				delete nGen;
				nGen = nullptr;
			}
		}

		virtual colorA_t getColor(const point3d_t &p, bool from_postprocessed=false) const;
		virtual float getFloat(const point3d_t &p) const;
		
		virtual void getInterpolationStep(float &step) const { step = size; };

		static texture_t *factory(paraMap_t &params,renderEnvironment_t &render);
	protected:
		int octaves;
		color_t color1, color2;
		float turb, size;
		bool hard, rings;
		noiseGenerator_t* nGen;
		enum {SIN, SAW, TRI} wshape;
};

class textureVoronoi_t : public texture_t
{
	public:
		textureVoronoi_t(const color_t &c1, const color_t &c2,
				int ct,
				float _w1, float _w2, float _w3, float _w4,
				float mex, float sz,
				float isc, const std::string &dname);
		virtual ~textureVoronoi_t() {}

		virtual colorA_t getColor(const point3d_t &p, bool from_postprocessed=false) const;
		virtual float getFloat(const point3d_t &p) const;
		
		virtual void getInterpolationStep(float &step) const { step = size; };

		static texture_t *factory(paraMap_t &params, renderEnvironment_t &render);
	protected:
		color_t color1, color2;
		float w1, w2, w3, w4;	// feature weights
		float aw1, aw2, aw3, aw4;	// absolute value of above
		float size;
		int coltype;	// color return type
		float iscale;	// intensity scale
		voronoi_t vGen;
};

class textureMusgrave_t : public texture_t
{
	public:
		textureMusgrave_t(const color_t &c1, const color_t &c2,
				float H, float lacu, float octs, float offs, float gain,
				float _size, float _iscale,
				const std::string &ntype, const std::string &mtype);
		virtual ~textureMusgrave_t();

		virtual colorA_t getColor(const point3d_t &p, bool from_postprocessed=false) const;
		virtual float getFloat(const point3d_t &p) const;
		
		virtual void getInterpolationStep(float &step) const { step = size; };

		static texture_t *factory(paraMap_t &params, renderEnvironment_t &render);

	protected:
		color_t color1, color2;
		float size, iscale;
		noiseGenerator_t* nGen;
		musgrave_t* mGen;
};

class textureDistortedNoise_t : public texture_t
{
	public:
		textureDistortedNoise_t(const color_t &c1, const color_t &c2,
					float _distort, float _size,
					const std::string &noiseb1, const std::string noiseb2);
		virtual ~textureDistortedNoise_t();
		
		virtual colorA_t getColor(const point3d_t &p, bool from_postprocessed=false) const;
		virtual float getFloat(const point3d_t &p) const;
		
		virtual void getInterpolationStep(float &step) const { step = size; };

		static texture_t *factory(paraMap_t &params, renderEnvironment_t &render);

	protected:
		color_t color1, color2;
		float distort, size;
		noiseGenerator_t *nGen1, *nGen2;
};

/*! RGB cube.
	basically a simple visualization of the RGB color space,
	just goes r in x, g in y and b in z inside the unit cube. */

class rgbCube_t : public texture_t
{
	public:
		rgbCube_t(){}
		virtual colorA_t getColor(const point3d_t &p, bool from_postprocessed=false) const;
		virtual float getFloat(const point3d_t &p) const;

		static texture_t *factory(paraMap_t &params,renderEnvironment_t &render);
};

enum blendProgressionType_t
{
	TEX_BLEND_LINEAR,
	TEX_BLEND_QUADRATIC,
	TEX_BLEND_EASING,
	TEX_BLEND_DIAGONAL,
	TEX_BLEND_SPHERICAL,
	TEX_BLEND_QUADRATIC_SPHERE,
	TEX_BLEND_RADIAL,
};

class textureBlend_t : public texture_t
{
	public:
		textureBlend_t(const std::string &stype, bool use_flip_axis);
		virtual ~textureBlend_t();
		
		virtual colorA_t getColor(const point3d_t &p, bool from_postprocessed=false) const;
		virtual float getFloat(const point3d_t &p) const;
		
		static texture_t *factory(paraMap_t &params, renderEnvironment_t &render);
		
	protected:
		int blendProgressionType = TEX_BLEND_LINEAR;
		bool m_use_flip_axis = false;
};	
__END_YAFRAY

#endif // Y_BASICTEX_H
