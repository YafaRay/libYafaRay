/****************************************************************************
 *
 *      hdrHandler.cc: Radiance hdr format handler
 *      This is part of the libYafaRay package
 *      Copyright (C) 2010 George Laskowsky Ziguilinsky (Glaskows)
 *      			   Rodrigo Placencia Vazquez (DarkTide)
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

#include "format/format_hdr.h"
#include "common/logger.h"
#include "common/param.h"
#include "common/file.h"
#include "scene/scene.h"
#include "image/image_layers.h"
#include "color/color_layers.h"

#include <fstream>

BEGIN_YAFARAY

Image * HdrFormat::loadFromFile(const std::string &name, const Image::Optimization &optimization, const ColorSpace &color_space, float gamma)
{
	std::FILE *fp = File::open(name, "rb");
	logger_.logInfo(getFormatName(), ": Loading image \"", name, "\"...");
	if(!fp)
	{
		logger_.logError(getFormatName(), ": Cannot open file ", name);
		return nullptr;
	}
	int width = 0;
	int height = 0;
	if(!readHeader(fp, width, height))
	{
		logger_.logError(getFormatName(), ": An error has occurred while reading the header...");
		File::close(fp);
		return nullptr;
	}
	const Image::Type type = Image::getTypeFromSettings(true, grayscale_);
	auto image = Image::factory(logger_, width, height, type, optimization);
	const int scan_width = (header_.y_first_) ? width : height;
	// run length encoding is not allowed so read flat and exit
	if((scan_width < 8) || (scan_width > 0x7fff))
	{
		for(int y = header_.min_[0]; y != header_.max_[0]; y += header_.step_[0])
		{
			if(!readOrle(fp, y, scan_width, image, color_space, gamma))
			{
				logger_.logError(getFormatName(), ": An error has occurred while reading uncompressed scanline...");
				File::close(fp);
				return nullptr;
			}
		}
		File::close(fp);
		return image;
	}

	for(int y = header_.min_[0]; y != header_.max_[0]; y += header_.step_[0])
	{
		RgbePixel pix;
		if(std::fread((char *)&pix, 1, sizeof(RgbePixel), fp) != sizeof(RgbePixel))
		{
			logger_.logError(getFormatName(), ": An error has occurred while reading scanline start...");
			File::close(fp);
			return nullptr;
		}
		if(std::feof(fp))
		{
			logger_.logError(getFormatName(), ": EOF reached while reading scanline start...");
			File::close(fp);
			return nullptr;
		}
		if(pix.isArleDesc())  // Adaptaive RLE schema encoding
		{
			if(pix.getArleCount() > scan_width)
			{
				logger_.logError(getFormatName(), ": Error reading, invalid ARLE scanline width...");
				File::close(fp);
				return nullptr;
			}
			if(!readArle(fp, y, pix.getArleCount(), image, color_space, gamma))
			{
				logger_.logError(getFormatName(), ": An error has occurred while reading ARLE scanline...");
				File::close(fp);
				return nullptr;
			}
		}
		else // Original RLE schema encoding or raw without compression
		{
			// rewind the read pixel to start reading from the begining of the scanline
			std::fseek(fp, static_cast<long int>(-sizeof(RgbePixel)), SEEK_CUR);
			if(!readOrle(fp, y, scan_width, image, color_space, gamma))
			{
				logger_.logError(getFormatName(), ": An error has occurred while reading RLE scanline...");
				File::close(fp);
				return nullptr;
			}
		}
	}
	File::close(fp);
	if(logger_.isVerbose()) logger_.logVerbose(getFormatName(), ": Done.");
	return image;
}

bool HdrFormat::readHeader(FILE *fp, int &width, int &height)
{
	const int line_size = 1000;
	char *linebuf = static_cast<char *>(std::malloc(line_size));
	std::fgets(linebuf, line_size, fp);
	std::string line = std::string(linebuf);
	if(line.find("#?") == std::string::npos)
	{
		logger_.logError(getFormatName(), ": File is not a valid Radiance RBGE image...");
		std::free(linebuf);
		return false;
	}
	size_t found_pos = 0;
	header_.exposure_ = 1.f;
	// search for optional flags
	for(;;)
	{
		std::fgets(linebuf, line_size, fp);
		line = std::string(linebuf);
		if(line == "" || line == "\n") break;  // We found the end of the header tag section and we move on
		//Find variables
		// We only check for the most used tags and ignore the rest
		if((found_pos = line.find("FORMAT=")) != std::string::npos)
		{
			if(line.substr(found_pos + 7).find("32-bit_rle_rgbe") == std::string::npos)
			{
				logger_.logError(getFormatName(), ": Sorry this is an XYZE file, only RGBE images are supported...");
				std::free(linebuf);
				return false;
			}
		}
		else if((found_pos = line.find("EXPOSURE=")) != std::string::npos)
		{
			float exp = 0.f;
			string::converter(line.substr(found_pos + 9), exp);
			header_.exposure_ *= exp; // Exposure is cumulative if several EXPOSURE tags exist on the file
		}
	}
	// check image size and orientation
	std::fgets(linebuf, line_size, fp);
	line = std::string(linebuf);
	const std::vector<std::string> size_orient = string::tokenize(line);
	header_.y_first_ = (size_orient[0].find('Y') != std::string::npos);
	int w = 3, h = 1;
	int x = 2, y = 0;
	int f = 0, s = 1;
	if(!header_.y_first_)
	{
		w = 1; h = 3;
		x = 0; y = 2;
		f = 1; s = 0;
	}
	string::converter(size_orient[w], width);
	string::converter(size_orient[h], height);

	// Set the reading order to fit yafaray's image coordinates
	const bool from_left = (size_orient[x].find('+') != std::string::npos);
	const bool from_top = (size_orient[y].find('-') != std::string::npos);
	header_.min_[f] = 0;
	header_.max_[f] = height;
	header_.step_[f] = 1;
	header_.min_[s] = 0;
	header_.max_[s] = width;
	header_.step_[s] = 1;
	if(!from_left)
	{
		header_.min_[s] = width - 1;
		header_.max_[s] = -1;
		header_.step_[s] = -1;
	}
	if(!from_top)
	{
		header_.min_[f] = height - 1;
		header_.max_[f] = -1;
		header_.step_[f] = -1;
	}
	std::free(linebuf);
	return true;
}

bool HdrFormat::readOrle(std::FILE *fp, int y, int scan_width, Image *image, const ColorSpace &color_space, float gamma)
{
	auto scanline = std::unique_ptr<RgbePixel[]>(new RgbePixel[scan_width]); // Scanline buffer
	int rshift = 0;
	RgbePixel pixel;
	for(int x = header_.min_[1]; x < scan_width; )
	{
		if(std::fread((char *)&pixel, 1, sizeof(RgbePixel), fp) != sizeof(RgbePixel))
		{
			logger_.logError(getFormatName(), ": An error has occurred while reading RLE scanline header...");
			return false;
		}
		// RLE encoded
		if(pixel.isOrleDesc())
		{
			int count = pixel.getOrleCount(rshift);
			if(count > scan_width - x)
			{
				logger_.logError(getFormatName(), ": Scanline width greater than image width...");
				return false;
			}
			pixel = scanline[x - 1];
			while(count--) scanline[x++] = pixel;
			rshift += 8;
		}
		else
		{
			scanline[x++] = pixel;
			rshift = 0;
		}
	}

	int j = 0;
	// put the pixels on the main buffer
	for(int x = header_.min_[1]; x != header_.max_[1]; x += header_.max_[1])
	{
		Rgba color = scanline[j].getRgba();
		color.linearRgbFromColorSpace(color_space, gamma);
		if(header_.y_first_) image->setColor(x, y, color);
		else image->setColor(y, x, color);
		++j;
	}
	return true;
}

bool HdrFormat::readArle(std::FILE *fp, int y, int scan_width, Image *image, const ColorSpace &color_space, float gamma)
{
	auto scanline = std::unique_ptr<RgbePixel[]>(new RgbePixel[scan_width]); // Scanline buffer
	if(!scanline) //FIXME: this no longer does anything since exceptions were introduced, failing "new" does not return nullptr but an exception (which is unhandled here)
	{
		logger_.logError(getFormatName(), ": Unable to allocate buffer memory...");
		return false;
	}
	// Read the 4 pieces of the scanline in order RGBE
	for(int chan = 0; chan < 4; chan++)
	{
		int j = 0;
		while(j < scan_width)
		{
			uint8_t count = 0; // run description
			if(std::fread((char *)&count, 1, 1, fp) != 1)
			{
				logger_.logError(getFormatName(), ": An error has occurred while reading ARLE scanline...");
				return false;
			}
			if(count > 128)
			{
				count &= 0x7F; // get the value without the run flag (value mask: 01111111)
				if(count + j > scan_width)
				{
					logger_.logError(getFormatName(), ": Run width greater than image width...");
					return false;
				}
				uint8_t col = 0; // color component
				if(std::fread((char *)&col, 1, 1, fp) != 1)
				{
					logger_.logError(getFormatName(), ": An error has occurred while reading ARLE scanline...");
					return false;
				}
				while(count--) scanline[j++][chan] = col;
			}
			else // else is a non-run raw values
			{
				if(count + j > scan_width)
				{
					logger_.logError(getFormatName(), ": Non-run width greater than image width or equal to zero...");
					return false;
				}
				while(count--)
				{
					uint8_t col = 0; // color component
					if(std::fread((char *)&col, 1, 1, fp) != 1)
					{
						logger_.logError(getFormatName(), ": An error has occurred while reading ARLE scanline...");
						return false;
					}
					scanline[j++][chan] = col;
				}
			}
		}
	}
	int j = 0;
	// put the pixels on the main buffer
	for(int x = header_.min_[1]; x != header_.max_[1]; x += header_.step_[1])
	{
		Rgba color = scanline[j].getRgba();
		color.linearRgbFromColorSpace(color_space, gamma);
		if(header_.y_first_) image->setColor(x, y, color);
		else image->setColor(y, x, color);
		++j;
	}
	return true;
}

bool HdrFormat::saveToFile(const std::string &name, const ImageLayer &image_layer, ColorSpace color_space, float gamma, bool alpha_premultiply)
{
	const int h = image_layer.getHeight();
	const int w = image_layer.getWidth();
	std::ofstream file(name.c_str(), std::ios::out | std::ios::binary);
	if(!file.is_open()) return false;
	else
	{
		writeHeader(file, image_layer.image_.get());
		RgbePixel signature; //scanline start signature for adaptative RLE
		signature.setScanlineStart(w); //setup the signature
		auto scanline = std::unique_ptr<RgbePixel[]>(new RgbePixel[w]);
		// write using adaptive-rle encoding
		for(int y = 0; y < h; y++)
		{
			// write scanline start signature
			file.write((char *)&signature, sizeof(RgbePixel));
			// fill the scanline buffer
			for(int x = 0; x < w; x++) scanline[x] = Layer::postProcess(image_layer.image_->getColor(x, y), image_layer.layer_.getType(), color_space, gamma, alpha_premultiply);
			// write the scanline RLE compressed by channel in 4 separated blocks not as contigous pixels pixel blocks
			if(!writeScanline(file, scanline.get(), image_layer.image_.get()))
			{
				logger_.logError(getFormatName(), ": An error has occurred during scanline saving...");
				return false;
			}
		}
		file.close();
	}
	if(logger_.isVerbose()) logger_.logVerbose(getFormatName(), ": Done.");
	return true;
}

bool HdrFormat::writeHeader(std::ofstream &file, const Image *image) const
{
	const int h = image->getHeight();
	const int w = image->getWidth();
	if(h <= 0 || w <= 0) return false;
	file << "#?" << header_.program_type_ << "\n";
	file << "# Image created with YafaRay\n";
	file << "EXPOSURE=" << header_.exposure_ << "\n";
	file << "FORMAT=32-bit_rle_rgbe\n\n";
	file << "-Y " << h << " +X " << w << "\n";
	return true;
}

bool HdrFormat::writeScanline(std::ofstream &file, RgbePixel *scanline, const Image *image)
{
	const int w = image->getWidth();
	// write the scanline RLE compressed by channel in 4 separated blocks not as contigous pixels pixel blocks
	for(int chan = 0; chan < 4; chan++)
	{
		int cur = 0;
		while(cur < w)
		{
			int beg_run = cur;
			int run_count = 0;
			int old_run_count = 0;
			while((run_count < 4) && (beg_run < w))
			{
				beg_run += run_count;
				old_run_count = run_count;
				run_count = 1;
				while((scanline[beg_run][chan] == scanline[beg_run + run_count][chan]) && (beg_run + run_count < w) && (run_count < 127))
				{
					run_count++;
				}
			}
			// write short run
			if((old_run_count > 1) && (old_run_count == beg_run - cur))
			{
				const uint8_t run_desc = 128 + old_run_count;
				file.write((const char *)&run_desc, 1);
				file.write((const char *)&scanline[cur][chan], 1);
				cur = beg_run;
			}
			// write non run bytes, until we get to big run
			while(cur < beg_run)
			{
				int nonrun_count = beg_run - cur;
				// Non run count can't be greater than 128
				if(nonrun_count > 128) nonrun_count = 128;
				const uint8_t run_desc = nonrun_count;
				file.write((const char *)&run_desc, 1);
				for(int i = 0; i < nonrun_count; i++)
				{
					file.write((const char *)&scanline[cur + i][chan], 1);
				}
				cur += nonrun_count;
			}
			// write out next run if one was found
			if(run_count >= 4)
			{
				const uint8_t run_desc = 128 + run_count;
				file.write((const char *)&run_desc, 1);
				file.write((const char *)&scanline[beg_run][chan], 1);
				cur += run_count;
			}
			// this shouldn't happend <<< if it happens then there is a logic error on the function :P, DarkTide
			if(cur > w) return false;
		}
	}
	return true;
}

END_YAFARAY
