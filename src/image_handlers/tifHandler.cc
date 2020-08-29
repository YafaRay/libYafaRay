/****************************************************************************
 *
 *      tifHandler.cc: Tag Image File Format (TIFF) image handler
 *      This is part of the yafray package
 *      Copyright (C) 2010 Rodrigo Placencia Vazquez
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

#include <core_api/imagehandler.h>
#include <core_api/logging.h>
#include <core_api/session.h>
#include <core_api/environment.h>
#include <core_api/params.h>
#include <utilities/math_utils.h>
#include <core_api/file.h>

#if defined(_WIN32)
#include <utilities/stringUtils.h>
#endif //defined(_WIN32)

namespace libtiff
{
#include <tiffio.h>
}

BEGIN_YAFRAY

#define INV_8  0.00392156862745098039 // 1 / 255
#define INV_16 0.00001525902189669642 // 1 / 65535

class TifHandler: public ImageHandler
{
	public:
		TifHandler();
		~TifHandler();
		bool loadFromFile(const std::string &name);
		bool saveToFile(const std::string &name, int img_index = 0);
		static ImageHandler *factory(ParamMap &params, RenderEnvironment &render);
};

TifHandler::TifHandler()
{
	has_alpha_ = false;
	multi_layer_ = false;

	handler_name_ = "TIFFHandler";
}

TifHandler::~TifHandler()
{
	clearImgBuffers();
}

bool TifHandler::saveToFile(const std::string &name, int img_index)
{
	int h = getHeight(img_index);
	int w = getWidth(img_index);

	std::string name_without_tmp = name;
	name_without_tmp.erase(name_without_tmp.length() - 4);
	if(session__.renderInProgress()) Y_INFO << handler_name_ << ": Autosaving partial render (" << roundFloatPrecision__(session__.currentPassPercent(), 0.01) << "% of pass " << session__.currentPass() << " of " << session__.totalPasses() << ") RGB" << (has_alpha_ ? "A" : "") << " file as \"" << name_without_tmp << "\"...  " << getDenoiseParams() << YENDL;
	else Y_INFO << handler_name_ << ": Saving RGB" << (has_alpha_ ? "A" : "") << " file as \"" << name_without_tmp << "\"...  " << getDenoiseParams() << YENDL;

#if defined(_WIN32)
	std::wstring wname = utf8ToWutf16Le__(name);
	libtiff::TIFF *out = libtiff::TIFFOpenW(wname.c_str(), "w");	//Windows needs the path in UTF16LE (unicode, UTF16, little endian) so we have to convert the UTF8 path to UTF16
#else
	libtiff::TIFF *out = libtiff::TIFFOpen(name.c_str(), "w");
#endif

	int channels;
	size_t bytes_per_scanline;

	if(has_alpha_) channels = 4;
	else channels = 3;

	libtiff::TIFFSetField(out, TIFFTAG_IMAGEWIDTH, w);
	libtiff::TIFFSetField(out, TIFFTAG_IMAGELENGTH, h);
	libtiff::TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, channels);
	libtiff::TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 8);
	libtiff::TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	libtiff::TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	libtiff::TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

	bytes_per_scanline = channels * w;

	YByte_t *scanline = (YByte_t *)libtiff::_TIFFmalloc(bytes_per_scanline);

	libtiff::TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, libtiff::TIFFDefaultStripSize(out, bytes_per_scanline));

	//The denoise functionality will only work if YafaRay is built with OpenCV support
#ifdef HAVE_OPENCV
	if(denoise_)
	{
		ImageBuffer denoised_buffer = img_buffer_.at(img_index)->getDenoisedLdrBuffer(denoise_hcol_, denoise_hlum_, denoise_mix_);
		for(int y = 0; y < h; y++)
		{
			for(int x = 0; x < w; x++)
			{
				int ix = x * channels;
				Rgba col = denoised_buffer.getColor(x, y);
				col.clampRgba01();
				scanline[ix] = (YByte_t)(col.getR() * 255.f);
				scanline[ix + 1] = (YByte_t)(col.getG() * 255.f);
				scanline[ix + 2] = (YByte_t)(col.getB() * 255.f);
				if(has_alpha_) scanline[ix + 3] = (YByte_t)(col.getA() * 255.f);
			}

			if(TIFFWriteScanline(out, scanline, y, 0) < 0)
			{
				Y_ERROR << handler_name_ << ": An error occurred while writing TIFF file" << YENDL;
				libtiff::TIFFClose(out);
				libtiff::_TIFFfree(scanline);

				return false;
			}
		}
	}
	else
#endif //HAVE_OPENCV
	{
		for(int y = 0; y < h; y++)
		{
			for(int x = 0; x < w; x++)
			{
				int ix = x * channels;
				Rgba col = img_buffer_.at(img_index)->getColor(x, y);
				col.clampRgba01();
				scanline[ix] = (YByte_t)(col.getR() * 255.f);
				scanline[ix + 1] = (YByte_t)(col.getG() * 255.f);
				scanline[ix + 2] = (YByte_t)(col.getB() * 255.f);
				if(has_alpha_) scanline[ix + 3] = (YByte_t)(col.getA() * 255.f);
			}

			if(TIFFWriteScanline(out, scanline, y, 0) < 0)
			{
				Y_ERROR << handler_name_ << ": An error occurred while writing TIFF file" << YENDL;
				libtiff::TIFFClose(out);
				libtiff::_TIFFfree(scanline);

				return false;
			}
		}
	}

	libtiff::TIFFClose(out);
	libtiff::_TIFFfree(scanline);

	Y_VERBOSE << handler_name_ << ": Done." << YENDL;

	return true;
}

bool TifHandler::loadFromFile(const std::string &name)
{
	libtiff::uint32 w, h;

#if defined(_WIN32)
	std::wstring wname = utf8ToWutf16Le__(name);
	libtiff::TIFF *tif = libtiff::TIFFOpenW(wname.c_str(), "r");	//Windows needs the path in UTF16LE (unicode, UTF16, little endian) so we have to convert the UTF8 path to UTF16
#else
	libtiff::TIFF *tif = libtiff::TIFFOpen(name.c_str(), "r");
#endif
	Y_INFO << handler_name_ << ": Loading image \"" << name << "\"..." << YENDL;



	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);

	libtiff::uint32 *tiff_data = (libtiff::uint32 *)libtiff::_TIFFmalloc(w * h * sizeof(libtiff::uint32));

	if(!libtiff::TIFFReadRGBAImage(tif, w, h, tiff_data, 0))
	{
		Y_ERROR << handler_name_ << ": Error reading TIFF file" << YENDL;
		return false;
	}

	has_alpha_ = true;
	width_ = (int)w;
	height_ = (int)h;

	clearImgBuffers();

	int n_channels = 3;
	if(grayscale_) n_channels = 1;
	else if(has_alpha_) n_channels = 4;
	img_buffer_.push_back(new ImageBuffer(width_, height_, n_channels, getTextureOptimization()));

	int i = 0;

	for(int y = height_ - 1; y >= 0; y--)
	{
		for(int x = 0; x < width_; x++)
		{
			Rgba color;
			color.set((float)TIFFGetR(tiff_data[i]) * INV_8,
					  (float)TIFFGetG(tiff_data[i]) * INV_8,
					  (float)TIFFGetB(tiff_data[i]) * INV_8,
					  (float)TIFFGetA(tiff_data[i]) * INV_8);
			i++;

			img_buffer_.at(0)->setColor(x, y, color, color_space_, gamma_);
		}
	}

	libtiff::_TIFFfree(tiff_data);

	libtiff::TIFFClose(tif);

	Y_VERBOSE << handler_name_ << ": Done." << YENDL;

	return true;
}

ImageHandler *TifHandler::factory(ParamMap &params, RenderEnvironment &render)
{
	int width = 0;
	int height = 0;
	bool with_alpha = false;
	bool for_output = true;
	bool img_grayscale = false;
	bool denoise_enabled = false;
	int denoise_h_lum = 3;
	int denoise_h_col = 3;
	float denoise_mix = 0.8f;

	params.getParam("width", width);
	params.getParam("height", height);
	params.getParam("alpha_channel", with_alpha);
	params.getParam("for_output", for_output);
	params.getParam("denoiseEnabled", denoise_enabled);
	params.getParam("denoiseHLum", denoise_h_lum);
	params.getParam("denoiseHCol", denoise_h_col);
	params.getParam("denoiseMix", denoise_mix);
	params.getParam("img_grayscale", img_grayscale);

	ImageHandler *ih = new TifHandler();

	if(for_output)
	{
		if(logger__.getUseParamsBadge()) height += logger__.getBadgeHeight();
		ih->initForOutput(width, height, render.getRenderPasses(), denoise_enabled, denoise_h_lum, denoise_h_col, denoise_mix, with_alpha, false, img_grayscale);
	}

	return ih;
}

extern "C"
{

	YAFRAYPLUGIN_EXPORT void registerPlugin__(RenderEnvironment &render)
	{
		render.registerImageHandler("tif", "tif tiff", "TIFF [Tag Image File Format]", TifHandler::factory);
	}

}

END_YAFRAY
