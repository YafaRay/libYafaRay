#include <utilities/guifont.h>
#include <utilities/loadMemPNG.h>
#include <utilities/yafLogoTiny.h>

#include <core_api/imagefilm.h>
#include <yafraycore/monitor.h>
#include <utilities/math_utils.h>
#include <yafraycore/timer.h>
#include <yaf_revision.h>
#include <cstring>
#include <string>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <iomanip>

#if HAVE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#endif

__BEGIN_YAFRAY

#define FILTER_TABLE_SIZE 16
#define MAX_FILTER_SIZE 8

//! Simple alpha blending with pixel weighting
#define alphaBlend(b_bg_col, b_weight, b_fg_col, b_alpha) (( b_bg_col * (1.f - b_alpha) ) + ( b_fg_col * b_weight * b_alpha ))

#if HAVE_FREETYPE

void imageFilm_t::drawFontBitmap( FT_Bitmap* bitmap, int x, int y)
{
	int i, j, p, q;
	int x_max = x + bitmap->width;
	int y_max = y + bitmap->rows;
	color_t textColor(1.f);

	for ( i = x, p = 0; i < x_max; i++, p++ )
	{
		for ( j = y, q = 0; j < y_max; j++, q++ )
		{
			if ( i >= w || j >= h )
				continue;

			pixel_t pix;
			int tmpBuf = bitmap->buffer[q * bitmap->width + p];
			if (tmpBuf > 0) {
				pix = (*image)(i, j);
				float alpha = (float)tmpBuf/255.0;
				pix.col = alphaBlend((*image)(i, j).col, pix.weight, colorA_t(textColor, std::max((*image)(x, y).col.getA(), alpha)), alpha);
				(*image)(i, j) = pix;
			}
		}
	}
}


void imageFilm_t::drawRenderSettings()
{
	FT_Library library;
	FT_Face face;

	FT_GlyphSlot slot;
	FT_Vector pen; // untransformed origin
	FT_Error error;

#ifdef RELEASE
	std::string version = std::string(VERSION);
#else
	std::string version = std::string(YAF_SVN_REV);
#endif

	std::stringstream ss;
	
	ss << "YafaRay (" << version << ")";
	
	ss << std::setprecision(4);
	double times = gTimer.getTime("rendert");
	int timem, timeh;
	gTimer.splitTime(times, &times, &timem, &timeh);
	ss << " | Render time:";
	if (timeh > 0) ss << " " << timeh << "h";
	if (timem > 0) ss << " " << timem << "m";
	ss << " " << times << "s";
	ss << " | " << aaSettings;
	ss << "\nLighting: " << integratorSettings;
	
	if(!customString.empty())
	{
		ss << " | " << customString;
	}

	std::string text = ss.str();//"Testing text free of exporter dependencies";

	Y_INFO << "ImageOverly: render settings\n" << text << "\n";

	// use 10pt at default dpi
	float fontsize = 9.5f;

	error = FT_Init_FreeType( &library ); // initialize library
	if ( error ) { Y_ERROR << "ImageOverly: FreeType lib couldn't be initialized!\n"; return; }

	error = FT_New_Memory_Face( library, (const FT_Byte*)guifont, guifont_size, 0, &face ); // create face object
	if ( error ) { Y_ERROR << "ImageOverly: FreeType couldn't load the font!\n"; return; }

	error = FT_Set_Char_Size( face, (FT_F26Dot6)(fontsize * 64.0), 0, 0, 0 ); // set character size
	if ( error ) { Y_ERROR << "ImageOverly: FreeType couldn't set the character size!\n"; return; }

	slot = face->glyph;

	// offsets
	int textOffsetX = 4;
	int textOffsetY = 18;
	int textInterlineOffset = 13;
	int logoWidth = 0;

	// Draw logo image
	imgBuffer_t *logo = load_mem_png(yafLogoTiny, yafLogoTiny_size);
	if(logo)
	{
		int lx, ly;
		int sx = 0;
		int sy = h - logo->resy();
		int imWidth = logo->resx() + sx;
		int imHeight = logo->resy() + sy;
		logoWidth = logo->resx();
		textOffsetX += logoWidth;

		for ( lx = sx; lx < imWidth; lx++ )
		{
			for ( ly = sy; ly < imHeight; ly++ )
			{
				colorA_t col = (*logo)(lx-sx, ly-sy);
				pixel_t pix = (*image)(lx, ly);
				
				pix.col = alphaBlend((*image)(lx, ly).col, pix.weight, col, col.getA());
				
				(*image)(lx, ly) = pix;
			}
		}
		delete logo;
	}

	// Draw the dark bar at the bottom
	float bgAlpha = 0.3f;
	color_t bgColor(0.f);
	
	for ( int x = logoWidth; x < w; x++ ) {
		for ( int y = h - 30; y < h; y++ ) {
			(*image)(x, y).col = alphaBlend((*image)(x, y).col, (*image)(x, y).weight, colorA_t(bgColor, std::max((*image)(x, y).col.getA(), bgAlpha)), bgAlpha);
		}
	}
	
	
	// The pen position in 26.6 cartesian space coordinates
	pen.x = textOffsetX * 64;
	pen.y = textOffsetY * 64;
	
	// Draw the text
	for ( size_t n = 0; n < text.size(); n++ )
	{
		// Set Coordinates for the carrige return
		if (text[n] == '\n') {
			pen.x = textOffsetX * 64;
			pen.y -= textInterlineOffset * 64;
			continue;
		}

		// Set transformation
		FT_Set_Transform( face, 0, &pen );

		// Load glyph image into the slot (erase previous one)
		error = FT_Load_Char( face, text[n], FT_LOAD_DEFAULT );
		if ( error ) { Y_ERROR << "ImageOverly: FreeType Couldn't load the glyph image for: '" << text[n] << "'!\n"; continue; }
		
		// Render the glyph into the slot
		FT_Render_Glyph( slot, FT_RENDER_MODE_NORMAL );

		// Now, draw to our target surface (convert position)
		drawFontBitmap( &slot->bitmap, slot->bitmap_left, h - slot->bitmap_top);

		// increment pen position
		pen.x += slot->advance.x;
		pen.y += slot->advance.y;
	}
	
	// Cleanup
	FT_Done_Face    ( face );
	FT_Done_FreeType( library );
}
#endif

void imageFilm_t::setAAParams(const std::string &aa_params)
{
	aaSettings = aa_params;
}

void imageFilm_t::setIntegParams(const std::string &integ_params)
{
	integratorSettings = integ_params;
}

void imageFilm_t::setCustomString(const std::string &custom)
{
	customString = custom;
}

typedef float filterFunc(float dx, float dy);

float Box(float dx, float dy){ return 1.f; }

//! Mitchel-Netravali constants
//! with B = 1/3 and C = 1/3 as suggested by the authors
//! mnX1 = constants for 1 <= |x| < 2
//! mna1 = (-B - 6 * C)/6
//! mnb1 = (6 * B + 30 * C)/6
//! mnc1 = (-12 * B - 48 * C)/6
//! mnd1 = (8 * B + 24 * C)/6
//!
//! mnX2 = constants for 1 > |x|
//! mna2 = (12 - 9 * B - 6 * C)/6
//! mnb2 = (-18 + 12 * B + 6 * C)/6
//! mnc2 = (6 - 2 * B)/6
/*
#define mna1 -0.38888889
#define mnb1  2.0
#define mnc1 -3.33333333
#define mnd1  1.77777778

#define mna2  1.16666666
#define mnb2 -2.0
#define mnc2  0.88888889
*/
#define gaussExp 0.00247875

float Mitchell(float dx, float dy)
{
	float x = 2.f * fSqrt(dx*dx + dy*dy);
	
	if(x >= 2.f) return (0.f);

	if(x >= 1.f) // from mitchell-netravali paper 1 <= |x| < 2
	{
		return (float)( x * ( x * ( x * -0.38888889f + 2.0f) - 3.33333333f) + 1.77777778f );
	}

	return (float)( x * x * ( 1.16666666f * x - 2.0f ) + 0.88888889f );
}

float Gauss(float dx, float dy)
{
	float r2 = dx*dx + dy*dy;
	return std::max(0.f, float(fExp(-6 * r2) - gaussExp));
}

imageFilm_t::imageFilm_t (int width, int height, int xstart, int ystart, colorOutput_t &out, float filterSize, filterType filt,
						  renderEnvironment_t *e, bool showSamMask, int tSize, imageSpliter_t::tilesOrderType tOrder, bool pmA, bool drawParams):
	flags(0), w(width), h(height), cx0(xstart), cy0(ystart), gamma(1.0), filterw(filterSize*0.5), output(&out),
	clamp(false), split(true), interactive(true), abort(false), correctGamma(false), estimateDensity(false), numSamples(0),
	splitter(0), pbar(0), env(e), showMask(showSamMask), tileSize(tSize), tilesOrder(tOrder), premultAlpha(pmA), drawParams(drawParams)
{
	cx1 = xstart + width;
	cy1 = ystart + height;
	filterTable = new float[FILTER_TABLE_SIZE * FILTER_TABLE_SIZE];
	
	// allocate image, the pixels are NOT YET set black! See init()...
	image = new tiledArray2D_t<pixel_t, 3>(width, height, false);
	
	// fill filter table:
	float *fTp = filterTable;
	float scale = 1.f/(float)FILTER_TABLE_SIZE;
	
	filterFunc *ffunc=0;
	switch(filt)
	{
		case MITCHELL: ffunc = Mitchell; filterw *= 2.6f; break;
		case GAUSS: ffunc = Gauss; filterw *= 2.f; break;
		case BOX:
		default:	ffunc = Box;
	}
	filterw = std::max(0.501, filterw); // filter needs to cover at least the area of one pixel!
	if(filterw > 0.5*MAX_FILTER_SIZE) filterw = 0.5*MAX_FILTER_SIZE;
	for(int y=0; y<FILTER_TABLE_SIZE; ++y)
	{
		for(int x=0; x<FILTER_TABLE_SIZE; ++x)
		{
			*fTp = ffunc((x+.5f)*scale, (y+.5f)*scale);
			++fTp;
		}
	}
	
	tableScale = 0.9999 * FILTER_TABLE_SIZE/filterw;
	area_cnt = 0;
	_n_unlocked = _n_locked = 0;
//	std::cout << "==ctor: cx0 "<<cx0<<", cx1 "<<cx1<<" cy0, "<<cy0<<", cy1 "<<cy1<<"\n";
	pbar = new ConsoleProgressBar_t(80);
}

void imageFilm_t::setProgressBar(progressBar_t *pb)
{
	if(pbar) delete pbar;
	pbar = pb;
}

void imageFilm_t::setGamma(float gammaVal, bool enable)
{
	correctGamma = enable;
	if(gammaVal > 0) gamma = 1.f/gammaVal; //gamma correction means applying gamma curve with 1/gamma
}

void imageFilm_t::setDensityEstimation(bool enable)
{
	if(enable) densityImage.resize(w, h, false);
	estimateDensity = enable;
}


void imageFilm_t::init(int numPasses)
{
	unsigned int size = image->size();
	pixel_t *pixels = image->getData();
	std::memset(pixels, 0, size * sizeof(pixel_t));
	//clear density image
	if(estimateDensity)
	{
		std::memset(densityImage.getData(), 0, densityImage.size()*sizeof(color_t));
	}
	//clear custom channels:
	for(unsigned int i=0; i<channels.size(); ++i)
	{
		tiledArray2D_t<float, 3> &chan = channels[i];
		float *cdata = chan.getData();
		std::memset(cdata, 0, chan.size() * sizeof(float));
	}
	
	if(split)
	{
		next_area = 0;
		splitter = new imageSpliter_t(w, h, cx0, cy0, tileSize, tilesOrder);
		area_cnt = splitter->size();
	}
	else area_cnt = 1;

	if(pbar) pbar->init(area_cnt);

	abort = false;
	completed_cnt = 0;
	nPass = 1;
	nPasses = numPasses;
}

// currently the splitter only gives tiles in scanline order...
bool imageFilm_t::nextArea(renderArea_t &a)
{
	if(abort) return false;
	int ifilterw = int(ceil(filterw));
	if(split)
	{
		int n;
		splitterMutex.lock();
		n = next_area++;
		splitterMutex.unlock();
		if(	splitter->getArea(n, a) )
		{
			a.sx0 = a.X + ifilterw;
			a.sx1 = a.X + a.W - ifilterw;
			a.sy0 = a.Y + ifilterw;
			a.sy1 = a.Y + a.H - ifilterw;
			
			if(interactive)
			{
				outMutex.lock();
				int end_x = a.X+a.W, end_y = a.Y+a.H;
				output->highliteArea(a.X, a.Y, end_x, end_y);
				outMutex.unlock();
			}

			return true;
		}
	}
	else
	{
		if(area_cnt) return false;
		a.X = cx0;
		a.Y = cy0;
		a.W = w;
		a.H = h;
		a.sx0 = a.X + ifilterw;
		a.sx1 = a.X + a.W - ifilterw;
		a.sy0 = a.Y + ifilterw;
		a.sy1 = a.Y + a.H - ifilterw;
		++area_cnt;
		return true;
	}
	return false;
}

// !todo: make output optional, and maybe output surrounding pixels influenced by filter too
void imageFilm_t::finishArea(renderArea_t &a)
{
	outMutex.lock();
	int end_x = a.X+a.W-cx0, end_y = a.Y+a.H-cy0;
	for(int j=a.Y-cy0; j<end_y; ++j)
	{
		for(int i=a.X-cx0; i<end_x; ++i)
		{
			pixel_t &pixel = (*image)(i, j);
			colorA_t col;
			if(pixel.weight>0.f)
			{
				col = pixel.col*(1.f/pixel.weight);
				col.clampRGB0();
			}
			else col = 0.0;
			// !!! assume color output size matches width and height of image film, probably (likely!) stupid!
			if(correctGamma) col.gammaAdjust(gamma);
			float fb[5];
			fb[0] = col.R, fb[1] = col.G, fb[2] = col.B, fb[3] = col.A, fb[4] = 0.f;
			if( !output->putPixel(i, j, fb, 4 ) ) abort=true;
			//if( !output->putPixel(i, j, col, col.getA()) ) abort=true;
		}
	}
	if(interactive) output->flushArea(a.X, a.Y, end_x+cx0, end_y+cy0);
	if(pbar)
	{
		if(++completed_cnt == area_cnt) pbar->done();
		else pbar->update(1);
	}
	outMutex.unlock();
}

/* CAUTION! Implemantation of this function needs to be thread safe for samples that
	contribute to pixels outside the area a AND pixels that might get
	contributions from outside area a! (yes, really!) */
void imageFilm_t::addSample(const colorA_t &c, int x, int y, float dx, float dy, const renderArea_t *a)
{
	colorA_t col=c;
	if(clamp) col.clampRGB01();
//	static int bleh=0;
	int dx0, dx1, dy0, dy1, x0, x1, y0, y1;
	// get filter extent:
	dx0 = Round2Int( (double)dx - filterw);
	dx1 = Round2Int( (double)dx + filterw - 1.0); //uhm why not kill the -1 and make '<=' a '<' in loop ?
	dy0 = Round2Int( (double)dy - filterw);
	dy1 = Round2Int( (double)dy + filterw - 1.0);
	// make sure we don't leave image area:
//	if(bleh<3)
//		std::cout <<"x:"<<x<<" y:"<<y<< "dx0 "<<dx0<<", dx1 "<<dx1<<" dy0, "<<dy0<<", dy1 "<<dy1<<"\n";
	dx0 = std::max(cx0-x, dx0);
	dx1 = std::min(cx1-x-1, dx1);
	dy0 = std::max(cy0-y, dy0);
	dy1 = std::min(cy1-y-1, dy1);
//	if(bleh<3)
//		std::cout << "camp: dx0 "<<dx0<<", dx1 "<<dx1<<" dy0, "<<dy0<<", dy1 "<<dy1<<"\n";
	// get indizes in filter table
	double x_offs = dx - 0.5;

	int xIndex[MAX_FILTER_SIZE+1], yIndex[MAX_FILTER_SIZE+1];

	for (int i=dx0, n=0; i <= dx1; ++i, ++n) {
		double d = std::fabs( (double(i) - x_offs) * tableScale);
		xIndex[n] = Floor2Int(d);

		if(xIndex[n] > FILTER_TABLE_SIZE-1)
		{
			std::cout << "filter table x error!\n";
			std::cout << "x: "<<x<<" dx: "<<dx<<" dx0: "<<dx0 <<" dx1: "<<dx1<<"\n";
			std::cout << "tableScale: "<<tableScale<<" d: "<<d<<" xIndex: "<<xIndex[n]<<"\n";
			throw std::logic_error("addSample error");
		}
	}

	double y_offs = dy - 0.5;

	for (int i=dy0, n=0; i <= dy1; ++i, ++n) {
		double d = std::fabs( (double(i) - y_offs) * tableScale);
		yIndex[n] = Floor2Int(d);

		if(yIndex[n] > FILTER_TABLE_SIZE-1)
		{
			std::cout << "filter table y error!\n";
			std::cout << "y: "<<y<<" dy: "<<dy<<" dy0: "<<dy0 <<" dy1: "<<dy1<<"\n";
			std::cout << "tableScale: "<<tableScale<<" d: "<<d<<" yIndex: "<<yIndex[n]<<"\n";
			throw std::logic_error("addSample error");
		}
	}
//	if(bleh<3)
//		std::cout << "double-check: dx0 "<<dx0<<", dx1 "<<dx1<<" dy0, "<<dy0<<", dy1 "<<dy1<<"\n";
	x0 = x+dx0; x1 = x+dx1;
	y0 = y+dy0; y1 = y+dy1;
//	if(bleh<3)
//		std::cout << "x0 "<<x0<<", x1 "<<x1<<", y0 "<<y0<<", y1 "<<y1<<"\n";
	// check if we need to be thread-safe, i.e. add outside safe area (4 ugly conditionals...can't help it):
	bool locked=false;
	if(!a || x0 < a->sx0 || x1 > a->sx1 || y0 < a->sy0 || y1 > a->sy1)
	{
		imageMutex.lock();
		locked=true;
		++_n_locked;
	}
	else ++_n_unlocked;
	for (int j = y0; j <= y1; ++j)
		for (int i = x0; i <= x1; ++i) {
			// get filter value at pixel (x,y)
			int offset = yIndex[j-y0]*FILTER_TABLE_SIZE + xIndex[i-x0];
			float filterWt = filterTable[offset];
			// update pixel values with filtered sample contribution
			pixel_t &pixel = (*image)(i - cx0, j - cy0);
			
			if(premultAlpha) pixel.col += (col * filterWt) * col.A;
			else pixel.col += (col * filterWt);
			
			pixel.weight += filterWt;
			/*if(i==0 && j==129) std::cout<<"col: "<<col<<" pcol: "<<
			pixel.col<<" pw: "<<pixel.weight<<" x:"<<x<<"y:"<<y<<"\n";*/
		}
	if(locked) imageMutex.unlock();
//	++bleh;
}

void imageFilm_t::addDensitySample(const color_t &c, int x, int y, float dx, float dy, const renderArea_t *a)
{
	if(!estimateDensity) return;
	
	int dx0, dx1, dy0, dy1, x0, x1, y0, y1;
	// get filter extent:
	dx0 = Round2Int( (double)dx - filterw);
	dx1 = Round2Int( (double)dx + filterw - 1.0); //uhm why not kill the -1 and make '<=' a '<' in loop ?
	dy0 = Round2Int( (double)dy - filterw);
	dy1 = Round2Int( (double)dy + filterw - 1.0);
	// make sure we don't leave image area:
	dx0 = std::max(cx0-x, dx0);
	dx1 = std::min(cx1-x-1, dx1);
	dy0 = std::max(cy0-y, dy0);
	dy1 = std::min(cy1-y-1, dy1);
	
	double x_offs = dx - 0.5;
	int xIndex[MAX_FILTER_SIZE+1], yIndex[MAX_FILTER_SIZE+1];
	for (int i=dx0, n=0; i <= dx1; ++i, ++n) {
		double d = std::fabs( (double(i) - x_offs) * tableScale);
		xIndex[n] = Floor2Int(d);
		if(xIndex[n] > FILTER_TABLE_SIZE-1)
		{
			throw std::logic_error("addSample error");
		}
	}
	double y_offs = dy - 0.5;
	for (int i=dy0, n=0; i <= dy1; ++i, ++n) {
		float d = fabsf( (double(i) - y_offs) * tableScale);
		yIndex[n] = Floor2Int(d);
		if(yIndex[n] > FILTER_TABLE_SIZE-1)
		{
			throw std::logic_error("addSample error");
		}
	}
	x0 = x+dx0; x1 = x+dx1;
	y0 = y+dy0; y1 = y+dy1;
	
	
	densityImageMutex.lock();
	for (int j = y0; j <= y1; ++j)
		for (int i = x0; i <= x1; ++i) {
			// get filter value at pixel (x,y)
			int offset = yIndex[j-y0]*FILTER_TABLE_SIZE + xIndex[i-x0];
			float filterWt = filterTable[offset];
			// update pixel values with filtered sample contribution
			color_t &pixel = densityImage(i - cx0, j - cy0);
			pixel += c * filterWt;
		}
	++numSamples;
	densityImageMutex.unlock();
}

// warning! not really thread-safe currently!
// although this is write-only and overwriting the same pixel makes little sense...
void imageFilm_t::setChanPixel(float val, int chan, int x, int y)
{
	channels[chan](x-cx0, y-cy0) = val;
}

void imageFilm_t::nextPass(bool adaptive_AA)
{
	int n_resample=0;
	
	splitterMutex.lock();
	next_area = 0;
	splitterMutex.unlock();
	nPass++;
	std::stringstream passString;
	
	if(flags) flags->clear();
	else flags = new tiledBitArray2D_t<3>(w, h, true);
	
	if(adaptive_AA && AA_thesh > 0.f)
	{
		float fb[4];
		fb[0] = 1.f;
		fb[1] = 0.5f;
		fb[2] = 0.5f;
		fb[3] = 1.f;

		for(int y=0; y<h-1; ++y)
		{
			for(int x=0; x<w-1; ++x)
			{
				bool needAA=false;
				float c=(*image)(x, y).normalized().abscol2bri();
				if(std::fabs(c-(*image)(x+1, y).normalized().col2bri()) >= AA_thesh)
				{
					needAA=true; flags->setBit(x+1, y);
				}
				if(std::fabs(c-(*image)(x, y+1).normalized().col2bri()) >= AA_thesh)
				{
					needAA=true; flags->setBit(x, y+1);
				}
				if(std::fabs(c-(*image)(x+1, y+1).normalized().col2bri()) >= AA_thesh)
				{
					needAA=true; flags->setBit(x+1, y+1);
				}
				if(x > 0 && std::fabs(c-(*image)(x-1, y+1).normalized().col2bri()) >= AA_thesh)
				{
					needAA=true; flags->setBit(x-1, y+1);
				}
				if(needAA)
				{
					flags->setBit(x, y);
					
					if(interactive && showMask)
					{
						color_t pixcol = (color_t)(*image)(x, y).normalized();
						float w = (pixcol.energy() + pixcol.maximum());
						fb[0] = fb[1] = fb[2] = w;
						output->putPixel(x, y, fb, 4);
					}
					
					++n_resample;
				}
			}
		}
	}
	else
	{
		n_resample = h*w;
	}
	
	if(interactive) output->flush();

	passString << "Rendering pass " << nPass << " of " << nPasses << ", resampling " << n_resample << " pixels.";

	Y_INFO << "imageFilm: " << passString.str() << "\n";
	
	if(pbar)
	{
		pbar->init(area_cnt);
		pbar->setTag(passString.str().c_str());
	}
	completed_cnt = 0;
}

void imageFilm_t::flush(int flags, colorOutput_t *out)
{
	Y_INFO << "imageFilm: Flushing buffer...\n";
	colorOutput_t *colout = out ? out : output;
#if HAVE_FREETYPE
	if (drawParams) {
		drawRenderSettings();
	}
#else
	if (drawParams) Y_WARNING << "imageFilm: compiled without freetype support overlay feature not available\n";
#endif
	int n = channels.size();
	float *fb = (float *)alloca( (n+5) * sizeof(float) );
	float multi = float(w*h)/(float)numSamples;
	for(int j=0; j<h; ++j)
	{
		for(int i=0; i<w; ++i)
		{
			pixel_t &pixel = (*image)(i, j);
			colorA_t col;
			if((flags & IF_IMAGE) && pixel.weight>0.f)
			{
				col = pixel.col/pixel.weight;
				col.clampRGB0();
			}
			else col = colorA_t(0.f);
			if(estimateDensity && (flags & IF_DENSITYIMAGE))
			{
				col += densityImage(i, j) * multi;
				col.clampRGB0();
			}
			// !!! assume color output size matches width and height of image film, probably (likely!) stupid!
			if(correctGamma) col.gammaAdjust(gamma);
			
			fb[0] = col.R, fb[1] = col.G, fb[2] = col.B, fb[3] = col.A, fb[4] = 0.f;
			for(int k=0; k<n; ++k) fb[k+4] = channels[k](i, j);
			colout->putPixel(i, j, fb, 4+n );
			//output->putPixel(i, j, col, col.getA());
		}
	}
	
	colout->flush();
	Y_INFO << "imageFilm: Done.\n";
}

bool imageFilm_t::doMoreSamples(int x, int y) const
{
	return (AA_thesh>0.f) ? flags->getBit(x-cx0, y-cy0) : true;
}

int imageFilm_t::addChannel(const std::string &name)
{
	channels.push_back( tiledArray2D_t<float, 3>() );
	tiledArray2D_t<float, 3> &chan = channels.back();
	chan.resize(w, h, false);
	
	return channels.size();
}

imageFilm_t::~imageFilm_t ()
{
	delete image;
	delete[] filterTable;
	if(splitter) delete splitter;
	if(pbar) delete pbar; //remove when pbar no longer created by imageFilm_t!!
	Y_INFO << "imageFilter stats:\n\tUnlocked adds: " << _n_unlocked << "\n\tLocked adds: " <<_n_locked<<"\n";
}

__END_YAFRAY
