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
		textureClouds_t(int dep, PFLOAT sz, bool hd,
				const color_t &c1, const color_t &c2,
				const std::string &ntype, const std::string &btype);
		virtual ~textureClouds_t();
		virtual colorA_t getColor(const point3d_t &p) const;
		virtual CFLOAT getFloat(const point3d_t &p) const;

		static texture_t *factory(paraMap_t &params,renderEnvironment_t &render);
	protected:
		int depth, bias;
		PFLOAT size;
		bool hard;
		color_t color1, color2;
		noiseGenerator_t* nGen;
};


class textureMarble_t : public texture_t
{
	public:
		textureMarble_t(int oct, PFLOAT sz, const color_t &c1, const color_t &c2,
				PFLOAT _turb, PFLOAT shp, bool hrd, const std::string &ntype, const std::string &shape);
		virtual ~textureMarble_t()
		{
			if (nGen) {
				delete nGen;
				nGen = NULL;
			}
		}

		virtual colorA_t getColor(const point3d_t &p) const;
		virtual CFLOAT getFloat(const point3d_t &p) const;

		static texture_t *factory(paraMap_t &params,renderEnvironment_t &render);
	protected:
		int octaves;
		color_t color1, color2;
		PFLOAT turb, sharpness, size;
		bool hard;
		noiseGenerator_t* nGen;
		enum {SIN, SAW, TRI} wshape;
};

class textureWood_t : public texture_t
{
	public:
		textureWood_t(int oct, PFLOAT sz, const color_t &c1, const color_t &c2, PFLOAT _turb,
				bool hrd, const std::string &ntype, const std::string &wtype, const std::string &shape);
		virtual ~textureWood_t()
		{
			if (nGen) {
				delete nGen;
				nGen = NULL;
			}
		}

		virtual colorA_t getColor(const point3d_t &p) const;
		virtual CFLOAT getFloat(const point3d_t &p) const;

		static texture_t *factory(paraMap_t &params,renderEnvironment_t &render);
	protected:
		int octaves;
		color_t color1, color2;
		PFLOAT turb, size;
		bool hard, rings;
		noiseGenerator_t* nGen;
		enum {SIN, SAW, TRI} wshape;
};

class textureVoronoi_t : public texture_t
{
	public:
		textureVoronoi_t(const color_t &c1, const color_t &c2,
				int ct,
				CFLOAT _w1, CFLOAT _w2, CFLOAT _w3, CFLOAT _w4,
				PFLOAT mex, PFLOAT sz,
				CFLOAT isc, const std::string &dname);
		virtual ~textureVoronoi_t() {}

		virtual colorA_t getColor(const point3d_t &p) const;
		virtual CFLOAT getFloat(const point3d_t &p) const;

		static texture_t *factory(paraMap_t &params, renderEnvironment_t &render);
	protected:
		color_t color1, color2;
		CFLOAT w1, w2, w3, w4;	// feature weights
		CFLOAT aw1, aw2, aw3, aw4;	// absolute value of above
		PFLOAT size;
		int coltype;	// color return type
		CFLOAT iscale;	// intensity scale
		voronoi_t vGen;
};

class textureMusgrave_t : public texture_t
{
	public:
		textureMusgrave_t(const color_t &c1, const color_t &c2,
				PFLOAT H, PFLOAT lacu, PFLOAT octs, PFLOAT offs, PFLOAT gain,
				PFLOAT _size, CFLOAT _iscale,
				const std::string &ntype, const std::string &mtype);
		virtual ~textureMusgrave_t();

		virtual colorA_t getColor(const point3d_t &p) const;
		virtual CFLOAT getFloat(const point3d_t &p) const;

		static texture_t *factory(paraMap_t &params, renderEnvironment_t &render);

	protected:
		color_t color1, color2;
		PFLOAT size, iscale;
		noiseGenerator_t* nGen;
		musgrave_t* mGen;
};

class textureDistortedNoise_t : public texture_t
{
	public:
		textureDistortedNoise_t(const color_t &c1, const color_t &c2,
					PFLOAT _distort, PFLOAT _size,
					const std::string &noiseb1, const std::string noiseb2);
		virtual ~textureDistortedNoise_t();
		
		virtual colorA_t getColor(const point3d_t &p) const;
		virtual CFLOAT getFloat(const point3d_t &p) const;

		static texture_t *factory(paraMap_t &params, renderEnvironment_t &render);

	protected:
		color_t color1, color2;
		PFLOAT distort, size;
		noiseGenerator_t *nGen1, *nGen2;
};

/*! RGB cube.
	basically a simple visualization of the RGB color space,
	just goes r in x, g in y and b in z inside the unit cube. */

class rgbCube_t : public texture_t
{
	public:
		rgbCube_t(){}
		virtual colorA_t getColor(const point3d_t &p) const;
		virtual CFLOAT getFloat(const point3d_t &p) const;

		static texture_t *factory(paraMap_t &params,renderEnvironment_t &render);
};


__END_YAFRAY

#endif // Y_BASICTEX_H
