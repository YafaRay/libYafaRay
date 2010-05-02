/****************************************************************************
 *
 *		imagetex.h: Image specific functions
 *      This is part of the yafray package
 *      Copyright (C) 2006  Mathias Wein
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
 *
 */

#ifndef Y_IMAGETEX_H
#define Y_IMAGETEX_H

#include <core_api/texture.h>
#include <core_api/environment.h>
#include <utilities/buffer.h>

__BEGIN_YAFRAY

enum TEX_CLIPMODE {TCL_EXTEND, TCL_CLIP, TCL_CLIPCUBE, TCL_REPEAT, TCL_CHECKER};

class textureImage_t : public texture_t
{
	public:
		enum INTERPOLATE_TYPE {NONE, BILINEAR, BICUBIC};
		textureImage_t(INTERPOLATE_TYPE intp): intp_type(intp){}
		virtual ~textureImage_t(){};
		virtual bool discrete() const { return true; }
		virtual bool isThreeD() const { return false; }
		virtual bool isNormalmap() const { return normalmap; }
		static texture_t *factory(paraMap_t &params,renderEnvironment_t &render);
	protected:
		void setCrop(PFLOAT minx, PFLOAT miny, PFLOAT maxx, PFLOAT maxy);
		bool doMapping(point3d_t &texp) const;
//		bool failed;
		bool use_alpha, calc_alpha, normalmap;
		bool cropx, cropy, checker_odd, checker_even, rot90;
		PFLOAT cropminx, cropmaxx, cropminy, cropmaxy;
		PFLOAT checker_dist;
		int xrepeat, yrepeat;
		int tex_clipmode;
		INTERPOLATE_TYPE intp_type;
};

// ============================================================================
/*! image texture using RGBE format, as in "HDR" (radiance) file format  */
// ============================================================================

class RGBEtexture_t: public textureImage_t
{
	public:
		RGBEtexture_t(gBuf_t<rgbe_t, 1> *im, textureImage_t::INTERPOLATE_TYPE intp, double exposure);
		virtual ~RGBEtexture_t();

		virtual colorA_t getColor(const point3d_t &sp) const;
		virtual colorA_t getColor(int x, int y, int z) const;
		virtual CFLOAT getFloat(const point3d_t &p) const;
		virtual void resolution(int &x, int &y, int &z) const;
		void setExposureAdjust(float ex) { EXPadjust = fPow(2.0f, ex); }
//		static texture_t *factory(paraMap_t &params,renderEnvironment_t &render);
	protected:
		gBuf_t<rgbe_t, 1> *image;
		CFLOAT EXPadjust;
};

class gammaLUT_t;

// ============================================================================
/*! image texture supporting 8bit integer and 32bit float format  */
// ============================================================================

class textureImageIF_t : public textureImage_t
{
	public:
//		textureImageIF_t(const char *filename, const std::string &intp);
		textureImageIF_t(cBuffer_t *im, fcBuffer_t *f_im, INTERPOLATE_TYPE intp);
		virtual ~textureImageIF_t();

		virtual colorA_t getColor(const point3d_t &sp) const;
		virtual colorA_t getColor(int x, int y, int z) const;
		virtual CFLOAT getFloat(const point3d_t &p) const;

		virtual bool loadFailed() const { return failed; }
		virtual bool discrete() const { return true; }
		virtual void resolution(int &x, int &y, int &z) const;
		void setGammaLUT(gammaLUT_t *lut);
//		static texture_t *factory(paraMap_t &params,renderEnvironment_t &render);
	protected:
		cBuffer_t *image;
		fcBuffer_t *float_image;
		bool failed;
		gammaLUT_t *gammaLUT;
};

static inline colorA_t cubicInterpolate(const colorA_t &c1, const colorA_t &c2,
								 const colorA_t &c3, const colorA_t &c4, CFLOAT x)
{
	colorA_t t2(c3-c2);
	colorA_t t1(t2 - (c2-c1));
	t2 = (c4-c3) - t2;
	CFLOAT ix = 1.f-x;
	return x*c3 + ix*c2 + ((4.f*t2 - t1)*(x*x*x-x) + (4.f*t1 - t2)*(ix*ix*ix-ix))*0.06666667f;
}

template<class T, class GetPix>
colorA_t interpolateImage(T image, textureImage_t::INTERPOLATE_TYPE intp, const point3d_t &p, GetPix &getPixel)
{
	int x, y, x2, y2;
	int resx=image->resx(), resy=image->resy();
	CFLOAT xf = ((CFLOAT)resx * (p.x - floor(p.x)));
	CFLOAT yf = ((CFLOAT)resy * (p.y - floor(p.y)));
	if (intp!=textureImage_t::NONE) { xf -= 0.5f;  yf -= 0.5f; }
	if ((x=(int)xf)<0) x = 0;
	if ((y=(int)yf)<0) y = 0;
	if (x>=resx) x = resx-1;
	if (y>=resy) y = resy-1;
	colorA_t c1;
	getPixel( (*image)(x, y), c1);
	if (intp==textureImage_t::NONE) return c1;
	colorA_t c2, c3, c4;
	if ((x2=x+1)>=resx) x2 = resx-1;
	if ((y2=y+1)>=resy) y2 = resy-1;
	getPixel( (*image)(x2, y), c2);
	getPixel( (*image)(x, y2), c3);
	getPixel( (*image)(x2, y2), c4);
	CFLOAT dx=xf-floor(xf), dy=yf-floor(yf);
	if (intp==textureImage_t::BILINEAR) {
		CFLOAT w0=(1-dx)*(1-dy), w1=(1-dx)*dy, w2=dx*(1-dy), w3=dx*dy;
		return colorA_t(w0*c1.getR() + w1*c3.getR() + w2*c2.getR() + w3*c4.getR(),
										w0*c1.getG() + w1*c3.getG() + w2*c2.getG() + w3*c4.getG(),
										w0*c1.getB() + w1*c3.getB() + w2*c2.getB() + w3*c4.getB(),
										w0*c1.getA() + w1*c3.getA() + w2*c2.getA() + w3*c4.getA());
	}
	int x0=x-1, x3=x2+1, y0=y-1, y3=y2+1;
	if (x0<0) x0 = 0;
	if (y0<0) y0 = 0;
	if (x3>=resx) x3 = resx-1;
	if (y3>=resy) y3 = resy-1;
	colorA_t c0, c5, c6, c7, c8, c9, cA, cB, cC, cD, cE, cF;
	getPixel( (*image)(x0, y0), c0);
	getPixel( (*image)(x,  y0), c5);
	getPixel( (*image)(x2, y0), c6);
	getPixel( (*image)(x3, y0), c7);
	getPixel( (*image)(x0, y ), c8);
	getPixel( (*image)(x3, y ), c9);
	getPixel( (*image)(x0, y2), cA);
	getPixel( (*image)(x3, y2), cB);
	getPixel( (*image)(x0, y3), cC);
	getPixel( (*image)(x,  y3), cD);
	getPixel( (*image)(x2, y3), cE);
	getPixel( (*image)(x3, y3), cF);
	c0 = cubicInterpolate(c0, c5, c6, c7, dx);
	c8 = cubicInterpolate(c8, c1, c2, c9, dx);
	cA = cubicInterpolate(cA, c3, c4, cB, dx);
	cC = cubicInterpolate(cC, cD, cE, cF, dx);
	return cubicInterpolate(c0, c8, cA, cC, dy);
}


__END_YAFRAY

#endif // Y_IMAGETEX_H
