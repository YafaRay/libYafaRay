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
#include <ImfVersion.h>
#include <IexThrowErrnoExc.h>

using namespace Imf;
using namespace Imath;

BEGIN_YAFARAY

//Class C_IStream from "Reading and Writing OpenEXR Image Files with the IlmImf Library" in the OpenEXR sources
class CiStream: public Imf::IStream
{
	public:
		CiStream(FILE *file, const char file_name[]):
				Imf::IStream(file_name), file_(file) {}
		virtual ~CiStream() override;
		virtual bool read(char c[], int n) override;
		virtual Int64 tellg() override;
		virtual void seekg(Int64 pos) override;
		virtual void clear() override;
		void close();
	private:
		FILE *file_ = nullptr;
};

bool CiStream::read(char c[], int n)
{
	if(file_)
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
	else return 0;
}

Int64 CiStream::tellg()
{
	if(file_)
	{
		return ftell(file_);
	}
	else return 0;
}

void CiStream::seekg(Int64 pos)
{
	if(file_)
	{
		clearerr(file_);
		fseek(file_, pos, SEEK_SET);
	}
}

void CiStream::clear()
{
	if(file_) clearerr(file_);
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
		CoStream(FILE *file, const char file_name[]):
				Imf::OStream(file_name), file_(file) {}
		virtual ~CoStream() override;
		virtual void write(const char c[], int n) override;
		virtual Int64 tellp() override;
		virtual void seekp(Int64 pos) override;
		void close();
	private:
		FILE *file_ = nullptr;
};

void CoStream::write(const char c[], int n)
{
	if(file_)
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
}

Int64 CoStream::tellp()
{
	if(file_) return ftell(file_);
	else return 0;
}

void CoStream::seekp(Int64 pos)
{
	if(file_)
	{
		clearerr(file_);
		fseek(file_, pos, SEEK_SET);
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
	int h = image->getHeight();
	int w = image->getWidth();

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
		Y_VERBOSE << getFormatName() << ": Done." << YENDL;
	}
	catch(const std::exception &exc)
	{
		Y_ERROR << getFormatName() << ": " << exc.what() << YENDL;
		result = false;
	}

	return result;
}

bool ExrFormat::saveToFileMultiChannel(const std::string &name, const ImageLayers *image_layers)
{
	int h_0 = (*image_layers)(Layer::Combined).image_->getHeight();
	int w_0 = (*image_layers)(Layer::Combined).image_->getWidth();

	bool all_image_buffers_same_size = true;
	for(const auto &image_layer : *image_layers)
	{
		if(image_layer.second.image_->getHeight() != h_0) all_image_buffers_same_size = false;
		if(image_layer.second.image_->getWidth() != w_0) all_image_buffers_same_size = false;
	}

	if(!all_image_buffers_same_size)
	{
		Y_ERROR << getFormatName() << ": Saving Multilayer EXR failed: not all the images in the imageBuffer have the same size. Make sure all images in buffer have the same size or use a non-multilayered EXR format." << YENDL;
		return false;
	}

	std::string exr_layer_name;
	int chan_size = sizeof(half);
	const int num_colchan = 4;
	int totchan_size = num_colchan * chan_size;

	Header header(w_0, h_0);
	FrameBuffer fb;
	header.compression() = ZIP_COMPRESSION;

	std::vector<Imf::Array2D<Imf::Rgba> *> pixels;

	for(const auto &image_layer : *image_layers)
	{
		const std::string layer_name = image_layer.second.layer_.getTypeName();
		exr_layer_name = "RenderLayer." + layer_name + ".";
		Y_VERBOSE << "    Writing EXR Layer: " << layer_name << YENDL;

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
		pixels.back()->resizeErase(h_0, w_0);

		for(int i = 0; i < w_0; ++i)
		{
			for(int j = 0; j < h_0; ++j)
			{
				Rgba col = image_layer.second.image_->getColor(i, j);
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
		Y_VERBOSE << getFormatName() << ": Done." << YENDL;
		for(size_t idx = 0; idx < pixels.size(); ++idx)
		{
			delete pixels.at(idx);
			pixels.at(idx) = nullptr;
		}
		pixels.clear();
	}
	catch(const std::exception &exc)
	{
		Y_ERROR << getFormatName() << ": " << exc.what() << YENDL;
		for(size_t idx = 0; idx < pixels.size(); ++idx)
		{
			delete pixels.at(idx);
			pixels.at(idx) = nullptr;
		}
		pixels.clear();
		result = false;
	}

	return result;
}

Image *ExrFormat::loadFromFile(const std::string &name, const Image::Optimization &optimization)
{
	FILE *fp = File::open(name.c_str(), "rb");
	Y_INFO << getFormatName() << ": Loading image \"" << name << "\"..." << YENDL;

	Image *image = nullptr;

	if(!fp)
	{
		Y_ERROR << getFormatName() << ": Cannot open file " << name << YENDL;
		return nullptr;
	}
	else
	{
		char bytes[4];
		fread(&bytes, 1, 4, fp);
		if(!isImfMagic(bytes)) return nullptr;
		fseek(fp, 0, SEEK_SET);
	}

	try
	{
		CiStream istr(fp, name.c_str());
		RgbaInputFile file(istr);
		Box2i dw = file.dataWindow();

		const int width  = dw.max.x - dw.min.x + 1;
		const int height = dw.max.y - dw.min.y + 1;

		const Image::Type type = Image::getTypeFromSettings(true, grayscale_);
		image = new Image(width, height, type, optimization);

		Imf::Array2D<Imf::Rgba> pixels;
		pixels.resizeErase(width, height);
		file.setFrameBuffer(&pixels[0][0] - dw.min.y - dw.min.x * height, height, 1);
		file.readPixels(dw.min.y, dw.max.y);

		for(int i = 0; i < width; ++i)
		{
			for(int j = 0; j < height; ++j)
			{
				Rgba col;
				col.r_ = pixels[i][j].r;
				col.g_ = pixels[i][j].g;
				col.b_ = pixels[i][j].b;
				col.a_ = pixels[i][j].a;
				image->setColor(i, j, col);//FIXME, color_space_, gamma_);
			}
		}
	}
	catch(const std::exception &exc)
	{
		Y_ERROR << getFormatName() << ": " << exc.what() << YENDL;
		if(image)
		{
			delete image;
			image = nullptr;
		}
	}
	return image;
}

Format *ExrFormat::factory(ParamMap &params)
{
	return new ExrFormat();
}

END_YAFARAY

#endif // HAVE_OPENEXR
