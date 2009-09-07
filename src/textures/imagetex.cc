/****************************************************************************
 * 			imagetex.cc: a texture class for various pixel image types
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
 */

#include <cstring>
#include <cctype>
#include <textures/imagetex.h>
#include <textures/gamma.h>
#if HAVE_EXR
#include <yafraycore/EXR_io.h>
#endif

__BEGIN_YAFRAY

gBuf_t<unsigned char, 4>* load_jpeg(const char *name);
#if HAVE_PNG
gBuf_t<unsigned char, 4> * load_png(const char *name);
#endif
gBuf_t<unsigned char, 4> * load_tga(const char *name);
gBuf_t<rgbe_t, 1>* loadHDR(const char* filename);

//-----------------------------------------------------------------------------------------
// Integer/Float Image Texture
//-----------------------------------------------------------------------------------------

textureImageIF_t::textureImageIF_t(cBuffer_t *im, fcBuffer_t *f_im, INTERPOLATE_TYPE intp):
				textureImage_t(intp), image(im), float_image(f_im), gammaLUT(0)
{ }

textureImageIF_t::~textureImageIF_t()
{
	if (image) {
		delete image;
		image = NULL;
	}
	if (float_image) {
		delete float_image;
		float_image = NULL;
	}
	if(gammaLUT){
		delete gammaLUT;
		gammaLUT = 0;
	}
}

void textureImageIF_t::resolution(int &x, int &y, int &z) const
{
	if(image){ x=image->resx(); y=image->resy(); z=0; }
	else if(float_image){ x=float_image->resx(); y=float_image->resy(); z=0; }
	else { x=0; y=0; z=0; }
}

static inline void getUcharPixel(unsigned char *data, colorA_t &col){ data >> col; }
static inline void getFloatPixel(float *data, colorA_t &col){ data >> col; }

colorA_t textureImageIF_t::getColor(const point3d_t &p) const
{
	// p->x/y == u, v
	point3d_t p1 = point3d_t(p.x, -p.y, p.z);
	bool outside = doMapping(p1);
	if(outside) return colorA_t(0.f, 0.f, 0.f, 0.f);
	colorA_t res(0.f);
	if(image)
	{
		if(gammaLUT)
			res = interpolateImage(image, intp_type, p1, *gammaLUT);
		else
			res = interpolateImage(image, intp_type, p1, getUcharPixel);
	}
	else if (float_image)
		res = interpolateImage(float_image, intp_type, p1, getFloatPixel);
	if(!use_alpha) res.A = 1.f;
	return res;
}

colorA_t textureImageIF_t::getColor(int x, int y, int z) const
{
	int resx, resy;
	if(image) resx=image->resx(), resy=image->resy();
	else if(float_image) resx=float_image->resx(), resy=float_image->resy();
	else return colorA_t(0.f);
	y = resy - y; //on occasion change image storage from bottom to top...
	if (x<0) x = 0;
	if (y<0) y = 0;
	if (x>=resx) x = resx-1;
	if (y>=resy) y = resy-1;
	colorA_t c1(0.f);
	if(image)
	{
		if(gammaLUT)
			(*gammaLUT)( (*image)(x, y), c1); 
		else
			getUcharPixel( (*image)(x, y), c1);
	}
	else if (float_image)
		getFloatPixel( (*float_image)(x, y), c1);
	return c1;
}

CFLOAT textureImageIF_t::getFloat(const point3d_t &p) const
{
	return getColor(p).energy();
}

void textureImageIF_t::setGammaLUT(gammaLUT_t *lut)
{
	gammaLUT = lut;
}

/* texture_t *textureImageIF_t::factory(paraMap_t &params,
		renderEnvironment_t &render)
{
	std::string _name, _intp="bilinear";	// default bilinear interpolation
	const std::string *name=&_name, *intp=&_intp;
	double gamma = 1.0;
	textureImageIF_t *tex = 0;
	params.getParam("interpolate", intp);
	params.getParam("filename", name);
	params.getParam("gamma", gamma);	
	if (*name=="")
		std::cerr << "Required argument filename not found for image texture\n";
	else
		tex = new textureImageIF_t(name->c_str(), *intp);
	if(!tex) return 0;
	std::cout << "gamma is: " << gamma << "\n";
	if(tex->image && (std::abs(1.0 - gamma) > 0.01) )
	{
		std::cout << "creating gamma LUT\n";
		tex->gammaLUT = new gammaLUT_t(gamma);
	}
	return tex;
} */

//-----------------------------------------------------------------------------------------
// Image Texture
//-----------------------------------------------------------------------------------------

bool textureImage_t::doMapping(point3d_t &texpt) const
{
	bool outside = false;
	texpt = 0.5f*texpt + 0.5f;
	// repeat, only valid for REPEAT clipmode
	if (tex_clipmode==TCL_REPEAT) {
		if (xrepeat>1) {
			texpt.x *= (PFLOAT)xrepeat;
			if (texpt.x>1.0) texpt.x -= int(texpt.x); else if (texpt.x<0.0) texpt.x += 1-int(texpt.x);
		}
		if (yrepeat>1) {
			texpt.y *= (PFLOAT)yrepeat;
			if (texpt.y>1.0) texpt.y -= int(texpt.y); else if (texpt.y<0.0) texpt.y += 1-int(texpt.y);
		}
	}
	
	// crop
	if (cropx) texpt.x = cropminx + texpt.x*(cropmaxx - cropminx);
	if (cropy) texpt.y = cropminy + texpt.y*(cropmaxy - cropminy);
	
	// rot90
	if(rot90) std::swap(texpt.x, texpt.y);
	
	// clipping
	switch (tex_clipmode)
	{
		case TCL_CLIPCUBE: {
			if ((texpt.x<0) || (texpt.x>1) || (texpt.y<0) || (texpt.y>1) || (texpt.z<-1) || (texpt.z>1))
				outside = true;
			break;
		}
		case TCL_CHECKER: {
			int xs=(int)floor(texpt.x), ys=(int)floor(texpt.y);
			texpt.x -= xs;
			texpt.y -= ys;
			if ( !checker_odd && !((xs+ys) & 1) )
			{
				outside = true;
				break;
			}
			if ( !checker_even && ((xs+ys) & 1) )
			{
				outside = true;
				break;
			}
			// scale around center, (0.5, 0.5)
			if (checker_dist<1.0) {
				texpt.x = (texpt.x-0.5) / (1.0-checker_dist) + 0.5;
				texpt.y = (texpt.y-0.5) / (1.0-checker_dist) + 0.5;
			}
			// continue to TCL_CLIP
		}
		case TCL_CLIP: {
			if ((texpt.x<0) || (texpt.x>1) || (texpt.y<0) || (texpt.y>1))
				outside = true;
			break;
		}
		case TCL_EXTEND: {
			if (texpt.x>0.99999f) texpt.x=0.99999f; else if (texpt.x<0) texpt.x=0;
			if (texpt.y>0.99999f) texpt.y=0.99999f; else if (texpt.y<0) texpt.y=0;
			// no break, fall thru to TEX_REPEAT
		}
		default:
		case TCL_REPEAT: outside = false;
	}
	return outside;
}

void textureImage_t::setCrop(PFLOAT minx, PFLOAT miny, PFLOAT maxx, PFLOAT maxy)
{
	cropminx=minx, cropmaxx=maxx, cropminy=miny, cropmaxy=maxy;
	cropx = ((cropminx!=0.0) || (cropmaxx!=1.0));
	cropy = ((cropminy!=0.0) || (cropmaxy!=1.0));
}

int string2cliptype(const std::string *clipname)
{
	// default "repeat"
	int	tex_clipmode = TCL_REPEAT;
	if(!clipname) return tex_clipmode;
	if (*clipname=="extend")			tex_clipmode = TCL_EXTEND;
	else if (*clipname=="clip")		tex_clipmode = TCL_CLIP;
	else if (*clipname=="clipcube")	tex_clipmode = TCL_CLIPCUBE;
	else if (*clipname=="checker")	tex_clipmode = TCL_CHECKER;
	return tex_clipmode;
}

texture_t *textureImage_t::factory(paraMap_t &params, renderEnvironment_t &render)
{
	std::string _name, _intp="bilinear";	// default bilinear interpolation
	const std::string *name=&_name, *intp=&_intp;
	double gamma = 1.0;
	double expadj=0.0;
	bool normalmap=false;
	textureImage_t *tex = 0;
	params.getParam("interpolate", intp);
	params.getParam("gamma", gamma);
	params.getParam("exposure_adjust", expadj);
	params.getParam("normalmap", normalmap);
	if(!params.getParam("filename", name))
	{
		std::cerr << "Required argument filename not found for image texture\n";
		return 0;
	}
	// interpolation type, bilinear default
	INTERPOLATE_TYPE intp_type = BILINEAR;
	if (*intp=="none")		  intp_type = NONE;
	else if (*intp=="bicubic") intp_type = BICUBIC;
	
	// Load image, try to determine from extensions first
	const char *filename = name->c_str();
	const char *extp = strrchr(filename, '.');
	bool jpg_tried = false;
	bool tga_tried = false;
	bool png_tried = false;
	bool hdr_tried = false;
#if HAVE_EXR
	bool exr_tried = false;
#endif
	
	cBuffer_t *image = 0;
	fcBuffer_t *float_image = 0;
	gBuf_t<rgbe_t, 1> *rgbe_image = 0;

	std::cout << "Loading image file " << filename << std::endl;

	// try loading image using extension as indication of imagetype
	if (extp) {
		std::string ext( extp );
		for(unsigned int i=0; i<ext.size(); ++i) ext[i] = std::tolower(ext[i]);
		// hdr
		if ( (ext == ".hdr") || (ext == ".pic") ) // the "official" extension for radiance image files
		{
			rgbe_image = loadHDR(filename);
			hdr_tried = true;
		}
		// jpeg
		if ( (ext == ".jpg") || (ext == ".jpeg") )
#ifdef HAVE_JPEG
		{
			image = load_jpeg(filename);
			jpg_tried = true;
		}
#else
		std::cout << "Warning, yafray was compiled without jpeg support, cannot load image.\n";
#endif
		// PNG
		if(ext == ".png")
#if HAVE_PNG
		{
			image = load_png(filename);
			png_tried = true;
		}
#else
		std::cout << "Warning, yafray was compiled without PNG support, cannot load image.\n";
#endif
		// OpenEXR
		if(ext == ".exr")
#if HAVE_EXR
		{
			float_image = loadEXR(filename);
			exr_tried = true;
		}
#else
		std::cout << "Warning, yafray was compiled without OpenEXR support, cannot load image.\n";
#endif

			// targa, apparently, according to ps description, on mac tga extension can be .tpic
		if( (ext == ".tga") || (ext == ".tpic") )
		{
			image = load_tga(filename);
			tga_tried = true;
		}
	}
	// if none was able to load (or no extension), try every type until one or none succeeds
	// targa last (targa has no ID)
	if ((float_image==NULL) && (image==NULL)  && (rgbe_image==NULL)) {
		std::cout << "unknown file extension, testing format...";
		for(;;) {

			if (!hdr_tried) {
				rgbe_image = loadHDR(filename);
				if (rgbe_image)
				{
					std::cout << "identified as Radiance format!\n";
					break;
				}
			}

#ifdef HAVE_JPEG
			if (!jpg_tried) {
				image = load_jpeg(filename);
				if (image)
				{
					std::cout << "identified as Jpeg format!\n";
					break;
				}
			}
#endif

#if HAVE_EXR
			if (!exr_tried) {
				float_image = loadEXR(filename);
				if (float_image)
				{
					std::cout << "identified as OpenEXR format!\n";
					break;
				}
			}
#endif

//			if (!tga_tried) {
//				image = loadTGA(filename, true);
//				if (image)
//				{
//					std::cout << "identified as Targa format!\n";
//					break;
//				}
//			}

			// nothing worked, give up
			std::cout << "\nunknown format!\n";
			break;

		}

	}
	
	if(image)
	{
		textureImageIF_t *itex = new textureImageIF_t(image, float_image, intp_type);
		if((std::fabs(1.0 - gamma) > 0.01) )
		{
			std::cout << "creating gamma LUT\n";
			itex->setGammaLUT(new gammaLUT_t(gamma));
		}
		tex = itex;
	}
	else if(float_image) tex = new textureImageIF_t(image, float_image, intp_type);
	else if(rgbe_image) tex = new RGBEtexture_t(rgbe_image, intp_type, expadj);
	else{ std::cout << "Could not load image\n"; return 0; }
	
	std::cout << "OK\n";
	
	// setup image
	bool rot90 = false;
	bool even_tiles=false, odd_tiles=true;
	bool use_alpha=true, calc_alpha=false;
	int xrep=1, yrep=1;
	double minx=0.0, miny=0.0, maxx=1.0, maxy=1.0;
	double cdist=0.0;
	const std::string *clipmode=0;
	params.getParam("xrepeat", xrep);
	params.getParam("yrepeat", yrep);
	params.getParam("cropmin_x", minx);
	params.getParam("cropmin_y", miny);
	params.getParam("cropmax_x", maxx);
	params.getParam("cropmax_y", maxy);
	params.getParam("rot90", rot90);
	params.getParam("clipping", clipmode);
	params.getParam("even_tiles", even_tiles);
	params.getParam("odd_tiles", odd_tiles);
	params.getParam("checker_dist", cdist);
	params.getParam("use_alpha", use_alpha);
	params.getParam("calc_alpha", calc_alpha);
	tex->xrepeat = xrep;
	tex->yrepeat = yrep;
	tex->rot90 = rot90;
	tex->setCrop(minx, miny, maxx, maxy);
	tex->use_alpha = use_alpha;
	tex->calc_alpha = calc_alpha;
	tex->normalmap = normalmap;
	tex->tex_clipmode = string2cliptype(clipmode);
	tex->checker_even = even_tiles;
	tex->checker_odd = odd_tiles;
	tex->checker_dist = cdist;
	return tex;
}

__END_YAFRAY
