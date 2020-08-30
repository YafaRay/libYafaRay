/****************************************************************************
 *
 *      pngHandler.cc: Joint Photographic Experts Group (JPEG) format handler
 *      This is part of the libYafaRay package
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

extern "C"
{
#include <setjmp.h>
#include <jpeglib.h>
}

BEGIN_YAFRAY

#define INV_8  0.00392156862745098039f // 1 / 255

// error handlers for libJPEG,

METHODDEF(void) jpgErrorMessage__(j_common_ptr info)
{
	char buffer[JMSG_LENGTH_MAX];
	(*info->err->format_message)(info, buffer);
	Y_ERROR << "JPEG Library Error: " << buffer << YENDL;
}

struct JpgErrorManager
{
	struct ::jpeg_error_mgr pub_;
	jmp_buf setjmp_buffer_;
};

// JPEG error manager pointer
typedef struct JpgErrorManager *ErrorPtr_t;

void jpgExitOnError__(j_common_ptr info)
{
	ErrorPtr_t myerr = (ErrorPtr_t)info->err;
	(*info->err->output_message)(info);
	longjmp(myerr->setjmp_buffer_, 1);
}

// The actual class

class JpgHandler: public ImageHandler
{
	public:
		JpgHandler();
		~JpgHandler();
		bool loadFromFile(const std::string &name);
		bool saveToFile(const std::string &name, int img_index = 0);
		static ImageHandler *factory(ParamMap &params, RenderEnvironment &render);
};

JpgHandler::JpgHandler()
{
	has_alpha_ = false;
	multi_layer_ = false;

	handler_name_ = "JPEGHandler";
}

JpgHandler::~JpgHandler()
{
	clearImgBuffers();
}

bool JpgHandler::saveToFile(const std::string &name, int img_index)
{
	int h = getHeight(img_index);
	int w = getWidth(img_index);

	std::string name_without_tmp = name;
	name_without_tmp.erase(name_without_tmp.length() - 4);
	if(session__.renderInProgress()) Y_INFO << handler_name_ << ": Autosaving partial render (" << roundFloatPrecision__(session__.currentPassPercent(), 0.01) << "% of pass " << session__.currentPass() << " of " << session__.totalPasses() << ") RGB" << " file as \"" << name_without_tmp << "\"...  " << getDenoiseParams() << YENDL;
	else Y_INFO << handler_name_ << ": Saving RGB" << " file as \"" << name_without_tmp << "\"...  " << getDenoiseParams() << YENDL;

	struct ::jpeg_compress_struct info;
	struct JpgErrorManager jerr;
	int x, y, ix;
	YByte_t *scanline = nullptr;

	FILE *fp = File::open(name, "wb");

	if(!fp)
	{
		Y_ERROR << handler_name_ << ": Cannot open file for writing " << name << YENDL;
		return false;
	}

	info.err = jpeg_std_error(&jerr.pub_);
	info.err->output_message = jpgErrorMessage__;
	jerr.pub_.error_exit = jpgExitOnError__;

	jpeg_create_compress(&info);
	jpeg_stdio_dest(&info, fp);

	info.image_width = w;
	info.image_height = h;
	info.in_color_space = JCS_RGB;
	info.input_components = 3;

	jpeg_set_defaults(&info);

	info.dct_method = JDCT_FLOAT;
	jpeg_set_quality(&info, 100, TRUE);

	jpeg_start_compress(&info, TRUE);

	scanline = new YByte_t[w * 3 ];

	//The denoise functionality will only work if YafaRay is built with OpenCV support
#ifdef HAVE_OPENCV
	if(denoise_)
	{
		ImageBuffer denoised_buffer = img_buffer_.at(img_index)->getDenoisedLdrBuffer(denoise_hcol_, denoise_hlum_, denoise_mix_);
		for(y = 0; y < h; y++)
		{
			for(x = 0; x < w; x++)
			{
				ix = x * 3;
				Rgba col = denoised_buffer.getColor(x, y);
				col.clampRgba01();
				scanline[ix]   = (YByte_t)(col.getR() * 255);
				scanline[ix + 1] = (YByte_t)(col.getG() * 255);
				scanline[ix + 2] = (YByte_t)(col.getB() * 255);
			}

			jpeg_write_scanlines(&info, &scanline, 1);
		}
	}
	else
#endif //HAVE_OPENCV
	{
		for(y = 0; y < h; y++)
		{
			for(x = 0; x < w; x++)
			{
				ix = x * 3;
				Rgba col = img_buffer_.at(img_index)->getColor(x, y);
				col.clampRgba01();
				scanline[ix] = (YByte_t)(col.getR() * 255);
				scanline[ix + 1] = (YByte_t)(col.getG() * 255);
				scanline[ix + 2] = (YByte_t)(col.getB() * 255);
			}

			jpeg_write_scanlines(&info, &scanline, 1);
		}
	}

	delete [] scanline;

	jpeg_finish_compress(&info);
	jpeg_destroy_compress(&info);

	File::close(fp);

	if(has_alpha_)
	{
		std::string alphaname = name.substr(0, name.size() - 4) + "_alpha.jpg";
		if(session__.renderInProgress()) Y_INFO << handler_name_ << ": Autosaving partial render (" << roundFloatPrecision__(session__.currentPassPercent(), 0.01) << "% of pass " << session__.currentPass() << " of " << session__.totalPasses() << ") Alpha channel as \"" << alphaname << "\"...  " << getDenoiseParams() << YENDL;
		else Y_INFO << handler_name_ << ": Saving Alpha channel as \"" << alphaname << "\"...  " << getDenoiseParams() << YENDL;

		fp = File::open(alphaname, "wb");

		if(!fp)
		{
			Y_ERROR << handler_name_ << ": Cannot open file for writing " << alphaname << YENDL;
			return false;
		}

		info.err = jpeg_std_error(&jerr.pub_);
		info.err->output_message = jpgErrorMessage__;
		jerr.pub_.error_exit = jpgExitOnError__;

		jpeg_create_compress(&info);
		jpeg_stdio_dest(&info, fp);

		info.image_width = w;
		info.image_height = h;
		info.in_color_space = JCS_GRAYSCALE;
		info.input_components = 1;

		jpeg_set_defaults(&info);

		info.dct_method = JDCT_FLOAT;
		jpeg_set_quality(&info, 100, TRUE);

		jpeg_start_compress(&info, TRUE);

		scanline = new YByte_t[ w ];

		for(y = 0; y < h; y++)
		{
			for(x = 0; x < w; x++)
			{
				float col = std::max(0.f, std::min(1.f, img_buffer_.at(img_index)->getColor(x, y).getA()));

				scanline[x] = (YByte_t)(col * 255);
			}

			jpeg_write_scanlines(&info, &scanline, 1);
		}

		delete [] scanline;

		jpeg_finish_compress(&info);
		jpeg_destroy_compress(&info);

		File::close(fp);
	}

	Y_VERBOSE << handler_name_ << ": Done." << YENDL;

	return true;
}

bool JpgHandler::loadFromFile(const std::string &name)
{
	jpeg_decompress_struct info;
	JpgErrorManager jerr;

	FILE *fp = File::open(name, "rb");

	Y_INFO << handler_name_ << ": Loading image \"" << name << "\"..." << YENDL;

	if(!fp)
	{
		Y_ERROR << handler_name_ << ": Cannot open file " << name << YENDL;
		return false;
	}

	info.err = jpeg_std_error(&jerr.pub_);
	info.err->output_message = jpgErrorMessage__;
	jerr.pub_.error_exit = jpgExitOnError__;

	if(setjmp(jerr.setjmp_buffer_))
	{
		jpeg_destroy_decompress(&info);

		File::close(fp);

		return false;
	}

	jpeg_create_decompress(&info);
	jpeg_stdio_src(&info, fp);
	jpeg_read_header(&info, TRUE);

	jpeg_start_decompress(&info);

	bool is_gray = ((info.out_color_space == JCS_GRAYSCALE) & (info.output_components == 1));
	bool is_rgb  = ((info.out_color_space == JCS_RGB) & (info.output_components == 3));
	bool is_cmyk = ((info.out_color_space == JCS_CMYK) & (info.output_components == 4));// TODO: findout if blender's non-standard jpeg + alpha comply with this or not, the code for conversion is below

	if((!is_gray) && (!is_rgb) && (!is_cmyk))
	{
		Y_ERROR << handler_name_ << ": Unsupported color space: " << info.out_color_space << "| Color components: " << info.output_components << YENDL;

		jpeg_finish_decompress(&info);
		jpeg_destroy_decompress(&info);

		File::close(fp);

		return false;
	}

	has_alpha_ = false;
	width_ = info.output_width;
	height_ = info.output_height;

	clearImgBuffers();

	int n_channels = 3;
	if(grayscale_) n_channels = 1;
	else if(has_alpha_) n_channels = 4;

	img_buffer_.push_back(new ImageBuffer(width_, height_, n_channels, getTextureOptimization()));

	YByte_t *scanline = new YByte_t[width_ * info.output_components];

	int y = 0;
	int ix = 0;

	while(info.output_scanline < info.output_height)
	{
		jpeg_read_scanlines(&info, &scanline, 1);

		for(int x = 0; x < width_; x++)
		{
			Rgba color;

			if(is_gray)
			{
				float colscan = scanline[x] * INV_8;
				color.set(colscan, colscan, colscan, 1.f);
			}
			else if(is_rgb)
			{
				ix = x * 3;
				color.set(scanline[ix] * INV_8,
						  scanline[ix + 1] * INV_8,
						  scanline[ix + 2] * INV_8,
				          1.f);
			}
			else if(is_cmyk)
			{
				ix = x * 4;
				float k = scanline[ix + 3] * INV_8;
				float i_k = 1.f - k;

				color.set(1.f - std::max((scanline[ix] * INV_8 * i_k) + k, 1.f),
				          1.f - std::max((scanline[ix + 1] * INV_8 * i_k) + k, 1.f),
				          1.f - std::max((scanline[ix + 2] * INV_8 * i_k) + k, 1.f),
				          1.f);
			}
			else // this is probabbly (surely) never executed, i need to research further; this assumes blender non-standard jpeg + alpha
			{
				ix = x * 4;
				float a = scanline[ix + 3] * INV_8;
				float i_a = 1.f - a;
				color.set(std::max(0.f, std::min((scanline[ix] * INV_8) - i_a, 1.f)),
						  std::max(0.f, std::min((scanline[ix + 1] * INV_8) - i_a, 1.f)),
						  std::max(0.f, std::min((scanline[ix + 2] * INV_8) - i_a, 1.f)),
						  a);
			}

			img_buffer_.at(0)->setColor(x, y, color, color_space_, gamma_);
		}
		y++;
	}

	delete [] scanline;

	jpeg_finish_decompress(&info);
	jpeg_destroy_decompress(&info);

	File::close(fp);

	Y_VERBOSE << handler_name_ << ": Done." << YENDL;

	return true;
}


ImageHandler *JpgHandler::factory(ParamMap &params, RenderEnvironment &render)
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

	ImageHandler *ih = new JpgHandler();

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
		render.registerImageHandler("jpg", "jpg jpeg", "JPEG [Joint Photographic Experts Group]", JpgHandler::factory);
	}

}

END_YAFRAY
