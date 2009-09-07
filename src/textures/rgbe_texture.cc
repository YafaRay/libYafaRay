
#include <cstring>
#include <textures/imagetex.h>
#include <core_api/environment.h>

__BEGIN_YAFRAY

#define MINELEN 8
#define MAXELEN 0x7fff
#define MINRUN	4	// minimum run length
#define RED 0
#define GRN 1
#define BLU 2
#define EXP 3
#define COLXS 128

RGBEtexture_t::RGBEtexture_t(gBuf_t<rgbe_t, 1> *im, textureImage_t::INTERPOLATE_TYPE intp, double exposure):
	textureImage_t(intp), image(im), EXPadjust(1.f)
{
	if(exposure != 0.0f) setExposureAdjust(exposure);
}

RGBEtexture_t::~RGBEtexture_t()
{
	if(image) delete image;
	image = 0;
}

bool checkHDR(FILE *file, int &xmax, int &ymax)
{
	char cs[256], st1[80], st2[80];
	bool resok = false, HDRok = false;
	while (!feof(file) && !HDRok)
	{
		fgets(cs, 255, file);
		if (strstr(cs, "32-bit_rle_rgbe")){ HDRok = true; break; }
	}
	if(!HDRok) return false;
	while (!feof(file) && !resok)
	{
		fgets(cs, 255, file);
		if (strcmp(cs, "\n") == 0) {
			// empty line found, next is resolution info, format: -Y N +X N
			// directly followed by data
			fgets(cs, 255, file);
			sscanf(cs, "%s %d %s %d", (char*)&st1, &ymax, (char*)&st2, &xmax);
			resok = true;
		}
	}
	return (HDRok && resok);
}

// old format
bool oldreadcolrs(FILE *file, rgbe_t *scan, int xmax)
{
	int i, rshift = 0, len = xmax;
	while (len > 0) {
		scan[0].rgbe[RED] = (unsigned char)getc(file);
		scan[0].rgbe[GRN] = (unsigned char)getc(file);
		scan[0].rgbe[BLU] = (unsigned char)getc(file);
		scan[0].rgbe[EXP] = (unsigned char)getc(file);
		if (feof(file) || ferror(file)) return false;
		if (scan[0].rgbe[RED] == 1 && scan[0].rgbe[GRN] == 1 && scan[0].rgbe[BLU] == 1) {
			for (i=scan[0].rgbe[EXP]<<rshift;i>0;i--) {
				scan[0] = scan[-1];
				scan++;
				len--;
			}
			rshift += 8;
		}
		else {
			scan++;
			len--;
			rshift = 0;
		}
	}
	return true;
}

// read and possibly RLE decode a rgbe scanline
bool freadcolrs(FILE *file, rgbe_t *scan, int xmax)
{
	int i,j,code,val;
	if ((xmax < MINELEN) | (xmax > MAXELEN)) return oldreadcolrs(file, scan, xmax);
	if ((i = getc(file)) == EOF) return false;
	if (i != 2) {
		ungetc(i, file);
		return oldreadcolrs(file, scan, xmax);
	}
	scan[0].rgbe[GRN] = (unsigned char)getc(file);
	scan[0].rgbe[BLU] = (unsigned char)getc(file);
	if ((i = getc(file)) == EOF) return false;
	if (((scan[0].rgbe[BLU] << 8) | i) != xmax) return false;
	for (i=0;i<4;i++)
		for (j=0;j<xmax;) {
			if ((code = getc(file)) == EOF) return false;
			if (code > 128) {
				code &= 127;
				val = getc(file);
				while (code--)
					scan[j++].rgbe[i] = (unsigned char)val;
			}
			else
				while (code--)
					scan[j++].rgbe[i] = (unsigned char)getc(file);
		}
	return feof(file) ? false : true;
}

gBuf_t<rgbe_t, 1>* loadHDR(const char* filename)
{
	FILE *file = fopen(filename,"rb");
	int xmax, ymax;
	if (file==NULL) return false;
	if (!checkHDR(file, xmax, ymax)) {
		fclose(file);
		return false;
	}
	bool ok=true;
	gBuf_t<rgbe_t, 1> *image = new gBuf_t<rgbe_t, 1>(xmax, ymax);
	rgbe_t *scanline = new rgbe_t[xmax];
	for (int y=ymax-1;y>=0;y--) {
		if( freadcolrs(file, scanline, xmax) )
		{
			for(int x=0; x<xmax; ++x) *(*image)(x, y) = scanline[x];
		}
		else
		{
			delete image; delete[] scanline; ok = false; break;
		}
	}
	fclose(file);
	delete[] scanline;
	return ok ? image : 0;
}

void RGBEtexture_t::resolution(int &x, int &y, int &z) const
{
	if(image){ x=image->resx(); y=image->resy(); z=0; }
	else { x=0; y=0; z=0; }
}

static inline void getRgbePixel(rgbe_t *data, colorA_t &col){ col = color_t(*data); }

colorA_t RGBEtexture_t::getColor(const point3d_t &p) const
{
	point3d_t p1 = p;
	bool outside = doMapping(p1);
	if(outside) return colorA_t(0.f, 0.f, 0.f, 0.f);
	// p->x/y == u, v
	if(!image) return color_t(0.0);
	colorA_t expad(EXPadjust,EXPadjust,EXPadjust,1.f);
	return expad * interpolateImage(image, intp_type, p1, getRgbePixel);
	
}

colorA_t RGBEtexture_t::getColor(int x, int y, int z) const
{
	int resx, resy;
	if(image) resx=image->resx(), resy=image->resy();
	else return colorA_t(0.f);
	x = (x<0) ? 0 : ((x>=resx) ? resx-1 : x);
	y = (y<0) ? 0 : ((y>=resy) ? resy-1 : y);
	colorA_t c1;
	getRgbePixel( (*image)(x, y), c1);
	return c1;
}

CFLOAT RGBEtexture_t::getFloat(const point3d_t &p) const
{
	return getColor(p).energy();
}

__END_YAFRAY
