#ifndef __NOISE_H
#define __NOISE_H

#include <core_api/vector3d.h>
#include <core_api/color.h>

#include <yafray_config.h>	

__BEGIN_YAFRAY

class YAFRAYPLUGIN_EXPORT noiseGenerator_t
{
public:
	noiseGenerator_t() {}
	virtual ~noiseGenerator_t() {}
	virtual PFLOAT operator() (const point3d_t &pt) const=0;
	// offset only added by blendernoise
	virtual point3d_t offset(const point3d_t &pt) const { return pt; }
};

//---------------------------------------------------------------------------
// Improved Perlin noise, based on Java reference code by Ken Perlin himself.
class YAFRAYPLUGIN_EXPORT newPerlin_t : public noiseGenerator_t
{
public:
	newPerlin_t() {}
	virtual ~newPerlin_t() {}
	virtual PFLOAT operator() (const point3d_t &pt) const;
private:
	PFLOAT fade(PFLOAT t) const { return t*t*t*(t*(t*6 - 15) + 10); }
	PFLOAT grad(int hash, PFLOAT x, PFLOAT y, PFLOAT z) const
	{
		int h = hash & 15;                     // CONVERT LO 4 BITS OF HASH CODE
		PFLOAT u = h<8 ? x : y,                // INTO 12 GRADIENT DIRECTIONS.
					 v = h<4 ? y : h==12||h==14 ? x : z;
		return ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
	}
};

//---------------------------------------------------------------------------
// Standard Perlin noise.
class YAFRAYPLUGIN_EXPORT stdPerlin_t : public noiseGenerator_t
{
public:
	stdPerlin_t() {}
	virtual ~stdPerlin_t() {}
	virtual PFLOAT operator() (const point3d_t &pt) const;
};

// Blender noise, similar to Perlin's
class YAFRAYPLUGIN_EXPORT blenderNoise_t : public noiseGenerator_t
{
public:
	blenderNoise_t() {}
	virtual ~blenderNoise_t() {}
	virtual PFLOAT operator() (const point3d_t &pt) const;
	// offset texture point coordinates by one
	virtual point3d_t offset(const point3d_t &pt) const { return pt+point3d_t(1.0, 1.0, 1.0); }
};

//---------------------------------------
// Voronoi, a.k.a. Worley/cellular basis

typedef PFLOAT (*distMetricFunc)(PFLOAT x, PFLOAT y, PFLOAT z, PFLOAT e);
// distance metrics as functors
/*struct distanceMetric_t
{
	virtual PFLOAT operator() (PFLOAT x, PFLOAT y, PFLOAT z, PFLOAT e)=0;
};

struct dist_Real : public distanceMetric_t
{
	virtual PFLOAT operator() (PFLOAT x, PFLOAT y, PFLOAT z, PFLOAT e) { return sqrt(x*x + y*y + z*z); }
};

struct dist_Squared : public distanceMetric_t
{
	virtual PFLOAT operator() (PFLOAT x, PFLOAT y, PFLOAT z, PFLOAT e) { return (x*x + y*y + z*z); }
};

struct dist_Manhattan : public distanceMetric_t
{
	virtual PFLOAT operator() (PFLOAT x, PFLOAT y, PFLOAT z, PFLOAT e) { return (std::fabs(x) + std::fabs(y) + std::fabs(z)); }
};

struct dist_Chebychev : public distanceMetric_t
{
	virtual PFLOAT operator() (PFLOAT x, PFLOAT y, PFLOAT z, PFLOAT e)
	{
		x = std::fabs(x);
		y = std::fabs(y);
		z = std::fabs(z);
		PFLOAT t = (x>y)?x:y;
		return ((z>t)?z:t);
	}
};

// minkovsky preset exponent 0.5
struct dist_MinkovskyH : public distanceMetric_t
{
	virtual PFLOAT operator() (PFLOAT x, PFLOAT y, PFLOAT z, PFLOAT e)
	{
		PFLOAT d = sqrt(std::fabs(x)) + sqrt(std::fabs(y)) + sqrt(std::fabs(z));
		return (d*d);
	}
};

// minkovsky preset exponent 4
struct dist_Minkovsky4 : public distanceMetric_t
{
	virtual PFLOAT operator() (PFLOAT x, PFLOAT y, PFLOAT z, PFLOAT e)
	{
		x *= x;
		y *= y;
		z *= z;
		return sqrt(sqrt(x*x + y*y + z*z));
	}
};

// Minkovsky, general case, slow
struct dist_Minkovsky : public distanceMetric_t
{
	virtual PFLOAT operator() (PFLOAT x, PFLOAT y, PFLOAT z, PFLOAT e)
	{
		return pow(pow(std::fabs(x), e) + pow(std::fabs(y), e) + pow(std::fabs(z), e), (PFLOAT)1.0/e);
	}
};
*/
class YAFRAYPLUGIN_EXPORT voronoi_t : public noiseGenerator_t
{
public:
	enum voronoiType {V_F1, V_F2, V_F3, V_F4, V_F2F1, V_CRACKLE};
	enum dMetricType {DIST_REAL, DIST_SQUARED, DIST_MANHATTAN, DIST_CHEBYCHEV,
				DIST_MINKOVSKY_HALF, DIST_MINKOVSKY_FOUR, DIST_MINKOVSKY};
	voronoi_t(voronoiType vt=V_F1, dMetricType dm=DIST_REAL, PFLOAT mex=2.5);
	virtual ~voronoi_t()
	{
		//if (distfunc) { delete distfunc;  distfunc=NULL; }
	}
	virtual PFLOAT operator() (const point3d_t &pt) const;
	PFLOAT getDistance(int x, PFLOAT da[4]) const { return da[x & 3]; }
	point3d_t getPoint(int x, point3d_t pa[4]) const { return pa[x & 3]; }
	void setMinkovskyExponent(PFLOAT me) { mk_exp=me; }
	void getFeatures(const point3d_t &pt, PFLOAT da[4], point3d_t pa[4]) const;
	void setDistM(dMetricType dm);
protected:
	voronoiType vType;
	dMetricType dmType;
	PFLOAT mk_exp, w1, w2, w3,w4;
//	distanceMetric_t* distfunc; //test...replace functors
	distMetricFunc distfunc2;
//	mutable PFLOAT da[4];			// distance array
//	mutable point3d_t pa[4];	// feature point array
};

// cell noise
class YAFRAYPLUGIN_EXPORT cellNoise_t : public noiseGenerator_t
{
public:
	cellNoise_t() {}
	virtual ~cellNoise_t() {}
	virtual PFLOAT operator() (const point3d_t &pt) const;
};

//------------------
// Musgrave types

class YAFRAYPLUGIN_EXPORT musgrave_t
{
public:
	musgrave_t() {}
	virtual ~musgrave_t() {}
	virtual PFLOAT operator() (const point3d_t &pt) const=0;
};

class YAFRAYPLUGIN_EXPORT fBm_t : public musgrave_t
{
public:
	fBm_t(PFLOAT _H, PFLOAT _lacu, PFLOAT _octs, const noiseGenerator_t* _nGen)
			: H(_H), lacunarity(_lacu), octaves(_octs), nGen(_nGen) {}
	virtual ~fBm_t() {}
	virtual PFLOAT operator() (const point3d_t &pt) const;
protected:
	PFLOAT H, lacunarity, octaves;
	const noiseGenerator_t* nGen;
};

class YAFRAYPLUGIN_EXPORT mFractal_t : public musgrave_t
{
public:
	mFractal_t(PFLOAT _H, PFLOAT _lacu, PFLOAT _octs, const noiseGenerator_t* _nGen)
			: H(_H), lacunarity(_lacu), octaves(_octs), nGen(_nGen) {}
	virtual ~mFractal_t() {}
	virtual PFLOAT operator() (const point3d_t &pt) const;
protected:
	PFLOAT H, lacunarity, octaves;
	const noiseGenerator_t* nGen;
};

class YAFRAYPLUGIN_EXPORT heteroTerrain_t : public musgrave_t
{
public:
	heteroTerrain_t(PFLOAT _H, PFLOAT _lacu, PFLOAT _octs, PFLOAT _offs, const noiseGenerator_t* _nGen)
			: H(_H), lacunarity(_lacu), octaves(_octs), offset(_offs), nGen(_nGen) {}
	virtual ~heteroTerrain_t() {}
	virtual PFLOAT operator() (const point3d_t &pt) const;
protected:
	PFLOAT H, lacunarity, octaves, offset;
	const noiseGenerator_t* nGen;
};

class YAFRAYPLUGIN_EXPORT hybridMFractal_t : public musgrave_t
{
public:
	hybridMFractal_t(PFLOAT _H, PFLOAT _lacu, PFLOAT _octs, PFLOAT _offs, PFLOAT _gain, const noiseGenerator_t* _nGen)
			: H(_H), lacunarity(_lacu), octaves(_octs), offset(_offs), gain(_gain), nGen(_nGen) {}
	virtual ~hybridMFractal_t() {}
	virtual PFLOAT operator() (const point3d_t &pt) const;
protected:
	PFLOAT H, lacunarity, octaves, offset, gain;
	const noiseGenerator_t* nGen;
};

class YAFRAYPLUGIN_EXPORT ridgedMFractal_t : public musgrave_t
{
public:
	ridgedMFractal_t(PFLOAT _H, PFLOAT _lacu, PFLOAT _octs, PFLOAT _offs, PFLOAT _gain, const noiseGenerator_t* _nGen)
			: H(_H), lacunarity(_lacu), octaves(_octs), offset(_offs), gain(_gain), nGen(_nGen) {}
	virtual ~ridgedMFractal_t() {}
	virtual PFLOAT operator() (const point3d_t &pt) const;
protected:
	PFLOAT H, lacunarity, octaves, offset, gain;
	const noiseGenerator_t* nGen;
};


// basic turbulence, half amplitude, double frequency defaults
// returns value in range (0,1)
CFLOAT YAFRAYPLUGIN_EXPORT turbulence(const noiseGenerator_t* ngen, const point3d_t &pt, int oct, PFLOAT size, bool hard);
// noise cell color (used with voronoi)
colorA_t YAFRAYPLUGIN_EXPORT cellNoiseColor(const point3d_t &pt);

static inline PFLOAT getSignedNoise(const noiseGenerator_t* nGen, const point3d_t &pt)
{
	return (PFLOAT)2.0 * (*nGen)(pt) - (PFLOAT)1.0;
}


__END_YAFRAY
//---------------------------------------------------------------------------
#endif  //__NOISE_H
