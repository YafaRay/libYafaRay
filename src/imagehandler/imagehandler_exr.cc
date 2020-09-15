/****************************************************************************
 *
 *      exrHandler.cc: EXR format handler
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

#ifdef HAVE_OPENEXR

#include "imagehandler/imagehandler_exr.h"
#include "common/logging.h"
#include "common/session.h"
#include "common/param.h"
#include "utility/util_math.h"
#include "common/file.h"
#include "common/renderpasses.h"
#include "common/scene.h"

#include <ImfOutputFile.h>
#include <ImfChannelList.h>
#include <ImfRgbaFile.h>
#include <ImfArray.h>
#include <ImfVersion.h>
#include <IexThrowErrnoExc.h>

using namespace Imf;
using namespace Imath;

BEGIN_YAFARAY

typedef GenericScanlineBuffer<Rgba> HalfRgbaScanlineImage_t;

//Class C_IStream from "Reading and Writing OpenEXR Image Files with the IlmImf Library" in the OpenEXR sources
class CiStream: public Imf::IStream
{
	public:
		CiStream(FILE *file, const char file_name[]):
				Imf::IStream(file_name), file_(file) {}
		virtual bool read(char c[], int n);
		virtual Int64 tellg();
		virtual void seekg(Int64 pos);
		virtual void clear();
	private:
		FILE *file_;
};

bool CiStream::read(char c[], int n)
{
	if(n != (int) fread(c, 1, n, file_))
	{
		// fread() failed, but the return value does not distinguish
		// between I/O errors and end of file, so we call ferror() to
		// determine what happened.
		if(ferror(file_)) Iex::throwErrnoExc();
		else throw Iex::InputExc("Unexpected end of file.");
	}
	return feof(file_);
}

Int64 CiStream::tellg()
{
	return ftell(file_);
}

void CiStream::seekg(Int64 pos)
{
	clearerr(file_);
	fseek(file_, pos, SEEK_SET);
}

void CiStream::clear()
{
	clearerr(file_);
}


class CoStream: public Imf::OStream
{
	public:
		CoStream(FILE *file, const char file_name[]):
				Imf::OStream(file_name), file_(file) {}
		virtual void write(const char c[], int n);
		virtual Int64 tellp();
		virtual void seekp(Int64 pos);
	private:
		FILE *file_;
};

void CoStream::write(const char c[], int n)
{
	if(n != (int) fwrite(c, 1, n, file_))
	{
		// fwrite() failed, but the return value does not distinguish
		// between I/O errors and end of file, so we call ferror() to
		// determine what happened.
		if(ferror(file_)) Iex::throwErrnoExc();
		else throw Iex::InputExc("Unexpected end of file.");
	}
}

Int64 CoStream::tellp()
{
	return ftell(file_);
}

void CoStream::seekp(Int64 pos)
{
	clearerr(file_);
	fseek(file_, pos, SEEK_SET);
}

ExrHandler::ExrHandler()
{
	handler_name_ = "EXRHandler";
}

ExrHandler::~ExrHandler()
{
	clearImgBuffers();
}

bool ExrHandler::saveToFile(const std::string &name, int img_index)
{
	int h = getHeight(img_index);
	int w = getWidth(img_index);

	std::string name_without_tmp = name;
	name_without_tmp.erase(name_without_tmp.length() - 4);
	if(session__.renderInProgress()) Y_INFO << handler_name_ << ": Autosaving partial render (" << roundFloatPrecision__(session__.currentPassPercent(), 0.01) << "% of pass " << session__.currentPass() << " of " << session__.totalPasses() << ") RGB" << (has_alpha_ ? "A" : "") << " file as \"" << name_without_tmp << "\"...  " << getDenoiseParams() << YENDL;
	else Y_INFO << handler_name_ << ": Saving RGB" << (has_alpha_ ? "A" : "") << " file as \"" << name_without_tmp << "\"...  " << getDenoiseParams() << YENDL;

	int chan_size = sizeof(half);
	const int num_colchan = 4;
	int totchan_size = num_colchan * chan_size;

	Header header(w, h);

	header.compression() = ZIP_COMPRESSION;

	header.channels().insert("R", Channel(HALF));
	header.channels().insert("G", Channel(HALF));
	header.channels().insert("B", Channel(HALF));
	header.channels().insert("A", Channel(HALF));

	FILE *fp = File::open(name.c_str(), "wb");
	CoStream ostr(fp, name.c_str());
	OutputFile file(ostr, header);

	Imf::Array2D<Imf::Rgba> pixels;
	pixels.resizeErase(h, w);

	for(int i = 0; i < w; ++i)
	{
		for(int j = 0; j < h; ++j)
		{
			Rgba col = img_buffer_.at(img_index)->getColor(i, j);
			pixels[j][i].r = col.r_;
			pixels[j][i].g = col.g_;
			pixels[j][i].b = col.b_;
			pixels[j][i].a = col.a_;
		}
	}

	char *data_ptr = (char *)&pixels[0][0];

	FrameBuffer fb;
	fb.insert("R", Slice(HALF, data_ptr, totchan_size, w * totchan_size));
	fb.insert("G", Slice(HALF, data_ptr +   chan_size, totchan_size, w * totchan_size));
	fb.insert("B", Slice(HALF, data_ptr + 2 * chan_size, totchan_size, w * totchan_size));
	fb.insert("A", Slice(HALF, data_ptr + 3 * chan_size, totchan_size, w * totchan_size));

	file.setFrameBuffer(fb);

	bool result = true;
	try
	{
		file.writePixels(h);
		Y_VERBOSE << handler_name_ << ": Done." << YENDL;
	}
	catch(const std::exception &exc)
	{
		Y_ERROR << handler_name_ << ": " << exc.what() << YENDL;
		result = false;
	}

	File::close(fp);
	return result;
}

bool ExrHandler::saveToFileMultiChannel(const std::string &name, const PassesSettings *passes_settings)
{
	int h_0 = img_buffer_.at(0)->getHeight();
	int w_0 = img_buffer_.at(0)->getWidth();

	bool all_image_buffers_same_size = true;
	for(size_t idx = 0; idx < img_buffer_.size(); ++idx)
	{
		if(img_buffer_.at(idx)->getHeight() != h_0) all_image_buffers_same_size = false;
		if(img_buffer_.at(idx)->getWidth() != w_0) all_image_buffers_same_size = false;
	}

	if(!all_image_buffers_same_size)
	{
		Y_ERROR << handler_name_ << ": Saving Multilayer EXR failed: not all the images in the imageBuffer have the same size. Make sure all images in buffer have the same size or use a non-multilayered EXR format." << YENDL;
		return false;
	}

	std::string exr_layer_name;

	std::string name_without_tmp = name;
	name_without_tmp.erase(name_without_tmp.length() - 4);

	if(session__.renderInProgress()) Y_INFO << handler_name_ << ": Autosaving partial render (" << roundFloatPrecision__(session__.currentPassPercent(), 0.01) << "% of pass " << session__.currentPass() << " of " << session__.totalPasses() << ") Multilayer EXR" << " file as \"" << name_without_tmp << "\"...  " << getDenoiseParams() << YENDL;
	else Y_INFO << handler_name_ << ": Saving Multilayer EXR" << " file as \"" << name_without_tmp << "\"...  " << getDenoiseParams() << YENDL;

	int chan_size = sizeof(half);
	const int num_colchan = 4;
	int totchan_size = num_colchan * chan_size;

	Header header(w_0, h_0);
	FrameBuffer fb;
	header.compression() = ZIP_COMPRESSION;

	std::vector<Imf::Array2D<Imf::Rgba> *> pixels;

	for(size_t idx = 0; idx < img_buffer_.size(); ++idx)
	{
		const std::string ext_pass_name = passes_settings->extPassesSettings()(idx).name();
		exr_layer_name = "RenderLayer." + ext_pass_name + ".";
		Y_VERBOSE << "    Writing EXR Layer: " << ext_pass_name << YENDL;

		const std::string channel_r_string = exr_layer_name + "R";
		const std::string channel_g_string = exr_layer_name + "G";
		const std::string channel_b_string = exr_layer_name + "B";
		const std::string channel_a_string = exr_layer_name + "A";

		const char *channel_r = channel_r_string.c_str();
		const char *channel_g = channel_g_string.c_str();
		const char *channel_b = channel_b_string.c_str();
		const char *channel_a = channel_a_string.c_str();

		header.channels().insert(channel_r, Channel(HALF));
		header.channels().insert(channel_g, Channel(HALF));
		header.channels().insert(channel_b, Channel(HALF));
		header.channels().insert(channel_a, Channel(HALF));

		pixels.push_back(new Imf::Array2D<Imf::Rgba>);
		pixels.at(idx)->resizeErase(h_0, w_0);

		for(int i = 0; i < w_0; ++i)
		{
			for(int j = 0; j < h_0; ++j)
			{
				Rgba col = img_buffer_.at(idx)->getColor(i, j);
				(*pixels.at(idx))[j][i].r = col.r_;
				(*pixels.at(idx))[j][i].g = col.g_;
				(*pixels.at(idx))[j][i].b = col.b_;
				(*pixels.at(idx))[j][i].a = col.a_;
			}
		}

		char *data_ptr = (char *) & (*pixels.at(idx))[0][0];

		fb.insert(channel_r, Slice(HALF, data_ptr, totchan_size, w_0 * totchan_size));
		fb.insert(channel_g, Slice(HALF, data_ptr + chan_size, totchan_size, w_0 * totchan_size));
		fb.insert(channel_b, Slice(HALF, data_ptr + 2 * chan_size, totchan_size, w_0 * totchan_size));
		fb.insert(channel_a, Slice(HALF, data_ptr + 3 * chan_size, totchan_size, w_0 * totchan_size));
	}

	FILE *fp = File::open(name.c_str(), "wb");
	CoStream ostr(fp, name.c_str());
	OutputFile file(ostr, header);
	file.setFrameBuffer(fb);

	bool result = true;
	try
	{
		file.writePixels(h_0);
		Y_VERBOSE << handler_name_ << ": Done." << YENDL;
		for(size_t idx = 0; idx < pixels.size(); ++idx)
		{
			delete pixels.at(idx);
			pixels.at(idx) = nullptr;
		}
		pixels.clear();
	}
	catch(const std::exception &exc)
	{
		Y_ERROR << handler_name_ << ": " << exc.what() << YENDL;
		for(size_t idx = 0; idx < pixels.size(); ++idx)
		{
			delete pixels.at(idx);
			pixels.at(idx) = nullptr;
		}
		pixels.clear();
		result = false;
	}

	File::close(fp);
	return result;
}

bool ExrHandler::loadFromFile(const std::string &name)
{
	FILE *fp = File::open(name.c_str(), "rb");
	Y_INFO << handler_name_ << ": Loading image \"" << name << "\"..." << YENDL;

	if(!fp)
	{
		Y_ERROR << handler_name_ << ": Cannot open file " << name << YENDL;
		return false;
	}
	else
	{
		char bytes[4];
		fread(&bytes, 1, 4, fp);
		if(!isImfMagic(bytes)) return false;
		fseek(fp, 0, SEEK_SET);
	}

	bool result = true;
	try
	{
		CiStream istr(fp, name.c_str());
		RgbaInputFile file(istr);
		Box2i dw = file.dataWindow();

		width_  = dw.max.x - dw.min.x + 1;
		height_ = dw.max.y - dw.min.y + 1;
		has_alpha_ = true;

		clearImgBuffers();

		int n_channels = 3;
		if(grayscale_) n_channels = 1;
		else if(has_alpha_) n_channels = 4;

		img_buffer_.push_back(new ImageBuffer(width_, height_, n_channels, getTextureOptimization()));

		Imf::Array2D<Imf::Rgba> pixels;
		pixels.resizeErase(width_, height_);
		file.setFrameBuffer(&pixels[0][0] - dw.min.y - dw.min.x * height_, height_, 1);
		file.readPixels(dw.min.y, dw.max.y);

		for(int i = 0; i < width_; ++i)
		{
			for(int j = 0; j < height_; ++j)
			{
				Rgba col;
				col.r_ = pixels[i][j].r;
				col.g_ = pixels[i][j].g;
				col.b_ = pixels[i][j].b;
				col.a_ = pixels[i][j].a;
				img_buffer_.at(0)->setColor(i, j, col, color_space_, gamma_);
			}
		}
	}
	catch(const std::exception &exc)
	{
		Y_ERROR << handler_name_ << ": " << exc.what() << YENDL;
		result = false;
	}

	File::close(fp);
	return result;
}

ImageHandler *ExrHandler::factory(ParamMap &params, Scene &scene)
{
	int pixtype = HALF;
	int compression = ZIP_COMPRESSION;
	int width = 0;
	int height = 0;
	bool with_alpha = false;
	bool for_output = true;
	bool multi_layer = false;
	bool img_grayscale = false;
	bool denoise_enabled = false;
	int denoise_h_lum = 3;
	int denoise_h_col = 3;
	float denoise_mix = 0.8f;

	params.getParam("pixel_type", pixtype);
	params.getParam("compression", compression);
	params.getParam("width", width);
	params.getParam("height", height);
	params.getParam("alpha_channel", with_alpha);
	params.getParam("for_output", for_output);
	params.getParam("img_multilayer", multi_layer);
	params.getParam("img_grayscale", img_grayscale);
	/*	//Denoise is not available for HDR/EXR images
	 * 	params.getParam("denoiseEnabled", denoiseEnabled);
	 *	params.getParam("denoiseHLum", denoiseHLum);
	 *	params.getParam("denoiseHCol", denoiseHCol);
	 *	params.getParam("denoiseMix", denoiseMix);
	 */
	ImageHandler *ih = new ExrHandler();

	ih->setTextureOptimization(TextureOptimization::None);

	if(for_output)
	{
		if(logger__.getUseParamsBadge()) height += logger__.getBadgeHeight();
		ih->initForOutput(width, height, scene.getPassesSettings(), denoise_enabled, denoise_h_lum, denoise_h_col, denoise_mix, with_alpha, multi_layer, img_grayscale);
	}

	return ih;
}

END_YAFARAY

#endif // HAVE_OPENEXR
