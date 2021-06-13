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

#include "format/format_exr.h"
#include "common/logger.h"
#include "common/param.h"
#include "common/file.h"
#include "image/image_buffers.h"
#include "image/image_layers.h"
#include "scene/scene.h"

#include <ImfOutputFile.h>
#include <ImfChannelList.h>
#include <ImfRgbaFile.h>
#include <ImfArray.h>
#include <IexThrowErrnoExc.h>

using namespace Imf;
using namespace Imath;

BEGIN_YAFARAY

//Class C_IStream from "Reading and Writing OpenEXR Image Files with the IlmImf Library" in the OpenEXR sources
class CiStream: public Imf::IStream
{
	public:
		CiStream(std::FILE *file, const char file_name[]) : Imf::IStream(file_name), file_(file) { }
		virtual ~CiStream() override;
		virtual bool read(char c[], int n) override;
		virtual Int64 tellg() override;
		virtual void seekg(Int64 pos) override;
		virtual void clear() override;
		void close();
	private:
		std::FILE *file_ = nullptr;
};

bool CiStream::read(char c[], int n)
{
	if(file_)
	{
		if(n != static_cast<int>(std::fread(c, 1, n, file_)))
		{
			// fread() failed, but the return value does not distinguish
			// between I/O errors and end of file, so we call ferror() to
			// determine what happened.
			if(std::ferror(file_)) Iex::throwErrnoExc();
			else throw Iex::InputExc("Unexpected end of file.");
		}
		return (std::feof(file_) > 0);
	}
	else return false;
}

Int64 CiStream::tellg()
{
	if(file_) return std::ftell(file_);
	else return 0;
}

void CiStream::seekg(Int64 pos)
{
	if(file_)
	{
		std::clearerr(file_);
		std::fseek(file_, pos, SEEK_SET);
	}
}

void CiStream::clear()
{
	if(file_) std::clearerr(file_);
}

void CiStream::close()
{
	if(file_)
	{
		File::close(file_);
		file_ = nullptr;
	}
}

CiStream::~CiStream()
{
	close();
}


class CoStream: public Imf::OStream
{
	public:
		CoStream(std::FILE *file, const char file_name[]) : Imf::OStream(file_name), file_(file) { }
		virtual ~CoStream() override;
		virtual void write(const char c[], int n) override;
		virtual Int64 tellp() override;
		virtual void seekp(Int64 pos) override;
		void close();
	private:
		std::FILE *file_ = nullptr;
};

void CoStream::write(const char c[], int n)
{
	if(file_)
	{
		if(n != static_cast<int>(std::fwrite(c, 1, n, file_)))
		{
			// fwrite() failed, but the return value does not distinguish
			// between I/O errors and end of file, so we call ferror() to
			// determine what happened.
			if(std::ferror(file_)) Iex::throwErrnoExc();
			else throw Iex::InputExc("Unexpected end of file.");
		}
	}
}

Int64 CoStream::tellp()
{
	if(file_) return std::ftell(file_);
	else return 0;
}

void CoStream::seekp(Int64 pos)
{
	if(file_)
	{
		std::clearerr(file_);
		std::fseek(file_, pos, SEEK_SET);
	}
}

void CoStream::close()
{
	if(file_)
	{
		File::close(file_);
		file_ = nullptr;
	}
}

CoStream::~CoStream()
{
	close();
}

bool ExrFormat::saveToFile(const std::string &name, const Image *image)
{
	const int h = image->getHeight();
	const int w = image->getWidth();
	const int chan_size = sizeof(half);
	const int num_colchan = 4;
	const int totchan_size = num_colchan * chan_size;
	Header header(w, h);
	header.compression() = ZIP_COMPRESSION;
	header.channels().insert("R", Channel(HALF));
	header.channels().insert("G", Channel(HALF));
	header.channels().insert("B", Channel(HALF));
	header.channels().insert("A", Channel(HALF));

	std::FILE *fp = File::open(name.c_str(), "wb");
	CoStream ostr(fp, name.c_str());
	OutputFile file(ostr, header);

	Imf::Array2D<Imf::Rgba> pixels;
	pixels.resizeErase(h, w);

	for(int i = 0; i < w; ++i)
	{
		for(int j = 0; j < h; ++j)
		{
			Rgba col = image->getColor(i, j);
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
		if(logger_.isVerbose()) logger_.logVerbose(getFormatName(), ": Done.");
	}
	catch(const std::exception &exc)
	{
		logger_.logError(getFormatName(), ": ", exc.what());
		result = false;
	}

	return result;
}

bool ExrFormat::saveToFileMultiChannel(const std::string &name, const ImageLayers *image_layers)
{
	const int h_0 = (*image_layers)(Layer::Combined).image_->getHeight();
	const int w_0 = (*image_layers)(Layer::Combined).image_->getWidth();

	bool all_image_buffers_same_size = true;
	for(const auto &image_layer : *image_layers)
	{
		if(image_layer.second.image_->getHeight() != h_0) all_image_buffers_same_size = false;
		if(image_layer.second.image_->getWidth() != w_0) all_image_buffers_same_size = false;
	}

	if(!all_image_buffers_same_size)
	{
		logger_.logError(getFormatName(), ": Saving Multilayer EXR failed: not all the images in the imageBuffer have the same size. Make sure all images in buffer have the same size or use a non-multilayered EXR format.");
		return false;
	}

	std::string exr_layer_name;
	const int chan_size = sizeof(half);
	const int num_colchan = 4;
	const int totchan_size = num_colchan * chan_size;

	Header header(w_0, h_0);
	FrameBuffer fb;
	header.compression() = ZIP_COMPRESSION;

	std::vector<std::unique_ptr<Imf::Array2D<Imf::Rgba>>> pixels;

	for(const auto &image_layer : *image_layers)
	{
		std::string layer_name = image_layer.second.layer_.getTypeName();
		const std::string exported_image_name = image_layer.second.layer_.getExportedImageName();
		if(!exported_image_name.empty()) layer_name += "-" + exported_image_name;
		exr_layer_name = "RenderLayer." + layer_name + ".";
		if(logger_.isVerbose()) logger_.logVerbose("    Writing EXR Layer: ", layer_name);

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

		pixels.emplace_back(new Imf::Array2D<Imf::Rgba>);
		pixels.back()->resizeErase(h_0, w_0);

		for(int i = 0; i < w_0; ++i)
		{
			for(int j = 0; j < h_0; ++j)
			{
				const Rgba col = image_layer.second.image_->getColor(i, j);
				(*pixels.back())[j][i].r = col.r_;
				(*pixels.back())[j][i].g = col.g_;
				(*pixels.back())[j][i].b = col.b_;
				(*pixels.back())[j][i].a = col.a_;
			}
		}

		char *data_ptr = (char *) & (*pixels.back())[0][0];

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
		if(logger_.isVerbose()) logger_.logVerbose(getFormatName(), ": Done.");
		pixels.clear();
	}
	catch(const std::exception &exc)
	{
		logger_.logError(getFormatName(), ": ", exc.what());
		pixels.clear();
		result = false;
	}
	return result;
}

std::unique_ptr<Image> ExrFormat::loadFromFile(const std::string &name, const Image::Optimization &optimization, const ColorSpace &color_space, float gamma)
{
	std::FILE *fp = File::open(name.c_str(), "rb");
	logger_.logInfo(getFormatName(), ": Loading image \"", name, "\"...");

	if(!fp)
	{
		logger_.logError(getFormatName(), ": Cannot open file ", name);
		return nullptr;
	}
	else
	{
		char bytes[4];
		std::fread(&bytes, 1, 4, fp);
		if(!isImfMagic(bytes)) return nullptr;
		std::fseek(fp, 0, SEEK_SET);
	}

	std::unique_ptr<Image> image;
	try
	{
		CiStream istr(fp, name.c_str());
		RgbaInputFile file(istr);
		const Box2i dw = file.dataWindow();
		const int width  = dw.max.x - dw.min.x + 1;
		const int height = dw.max.y - dw.min.y + 1;
		const Image::Type type = Image::getTypeFromSettings(true, grayscale_);
		image = Image::factory(logger_, width, height, type, optimization);
		Imf::Array2D<Imf::Rgba> pixels;
		pixels.resizeErase(width, height);
		file.setFrameBuffer(&pixels[0][0] - dw.min.y - dw.min.x * height, height, 1);
		file.readPixels(dw.min.y, dw.max.y);

		for(int i = 0; i < width; ++i)
		{
			for(int j = 0; j < height; ++j)
			{
				Rgba color;
				color.r_ = pixels[i][j].r;
				color.g_ = pixels[i][j].g;
				color.b_ = pixels[i][j].b;
				color.a_ = pixels[i][j].a;
				color.linearRgbFromColorSpace(color_space, gamma);
				image->setColor(i, j, color);
			}
		}
	}
	catch(const std::exception &exc)
	{
		logger_.logError(getFormatName(), ": ", exc.what());
		image = nullptr;
	}
	return image;
}

std::unique_ptr<Format> ExrFormat::factory(Logger &logger, ParamMap &params)
{
	return std::unique_ptr<Format>(new ExrFormat(logger));
}

END_YAFARAY
