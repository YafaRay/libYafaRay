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

#include "imagehandler/imagehandler_hdr.h"
#include "common/logging.h"
#include "common/session.h"
#include "common/environment.h"
#include "common/param.h"
#include "utility/util_math.h"
#include "common/file.h"

#include <fstream>
#include <iostream>
#include <vector>
#include <iomanip>

BEGIN_YAFARAY

HdrHandler::HdrHandler()
{
	has_alpha_ = false;
	multi_layer_ = false;

	handler_name_ = "hdrHandler";
}

HdrHandler::~HdrHandler()
{
	clearImgBuffers();
}

bool HdrHandler::loadFromFile(const std::string &name)
{
	FILE *fp = File::open(name, "rb");

	Y_INFO << handler_name_ << ": Loading image \"" << name << "\"..." << YENDL;

	if(!fp)
	{
		Y_ERROR << handler_name_ << ": Cannot open file " << name << YENDL;
		return false;
	}

	if(!readHeader(fp))
	{
		Y_ERROR << handler_name_ << ": An error has occurred while reading the header..." << YENDL;
		File::close(fp);
		return false;
	}

	// discard old image data
	clearImgBuffers();

	has_alpha_ = false;	//FIXME: why is alpha false in HDR??
	int n_channels = 4;	//FIXME: despite alpha being false, number of channels was 4 anyway. I'm keeping this just in case, but I think it should be 3??
	if(grayscale_) n_channels = 1;
	else if(has_alpha_) n_channels = 4;

	img_buffer_.push_back(new ImageBuffer(width_, height_, n_channels, getTextureOptimization()));

	int scan_width = (header_.y_first_) ? width_ : height_;

	// run length encoding is not allowed so read flat and exit
	if((scan_width < 8) || (scan_width > 0x7fff))
	{
		for(int y = header_.min_[0]; y != header_.max_[0]; y += header_.step_[0])
		{
			if(!readOrle(fp, y, scan_width))
			{
				Y_ERROR << handler_name_ << ": An error has occurred while reading uncompressed scanline..." << YENDL;
				File::close(fp);
				return false;
			}
		}
		File::close(fp);
		return true;
	}

	for(int y = header_.min_[0]; y != header_.max_[0]; y += header_.step_[0])
	{
		RgbePixel pix;

		if(fread((char *)&pix, 1, sizeof(RgbePixel), fp) != sizeof(RgbePixel))
		{
			Y_ERROR << handler_name_ << ": An error has occurred while reading scanline start..." << YENDL;
			File::close(fp);
			return false;
		}

		if(feof(fp))
		{
			Y_ERROR << handler_name_ << ": EOF reached while reading scanline start..." << YENDL;
			File::close(fp);
			return false;
		}

		if(pix.isArleDesc())  // Adaptaive RLE schema encoding
		{
			if(pix.getArleCount() > scan_width)
			{
				Y_ERROR << handler_name_ << ": Error reading, invalid ARLE scanline width..." << YENDL;
				File::close(fp);
				return false;
			}

			if(!readArle(fp, y, pix.getArleCount()))
			{
				Y_ERROR << handler_name_ << ": An error has occurred while reading ARLE scanline..." << YENDL;
				File::close(fp);
				return false;
			}
		}
		else // Original RLE schema encoding or raw without compression
		{
			// rewind the read pixel to start reading from the begining of the scanline
			fseek(fp, (long int) - sizeof(RgbePixel), SEEK_CUR);

			if(!readOrle(fp, y, scan_width))
			{
				Y_ERROR << handler_name_ << ": An error has occurred while reading RLE scanline..." << YENDL;
				File::close(fp);
				return false;
			}
		}
	}

	File::close(fp);

	Y_VERBOSE << handler_name_ << ": Done." << YENDL;

	return true;
}

bool HdrHandler::readHeader(FILE *fp)
{
	std::string line;
	int line_size = 1000;
	char *linebuf = (char *) malloc(line_size);

	fgets(linebuf, line_size, fp);
	line = std::string(linebuf);

	if(line.find("#?") == std::string::npos)
	{
		Y_ERROR << handler_name_ << ": File is not a valid Radiance RBGE image..." << YENDL;
		free(linebuf);
		return false;
	}

	size_t found_pos = 0;

	header_.exposure_ = 1.f;

	// search for optional flags
	for(;;)
	{
		fgets(linebuf, line_size, fp);
		line = std::string(linebuf);

		if(line == "" || line == "\n") break;  // We found the end of the header tag section and we move on

		//Find variables
		// We only check for the most used tags and ignore the rest

		if((found_pos = line.find("FORMAT=")) != std::string::npos)
		{
			if(line.substr(found_pos + 7).find("32-bit_rle_rgbe") == std::string::npos)
			{
				Y_ERROR << handler_name_ << ": Sorry this is an XYZE file, only RGBE images are supported..." << YENDL;
				free(linebuf);
				return false;
			}
		}
		else if((found_pos = line.find("EXPOSURE=")) != std::string::npos)
		{
			float exp = 0.f;
			converter__(line.substr(found_pos + 9), exp);
			header_.exposure_ *= exp; // Exposure is cumulative if several EXPOSURE tags exist on the file
		}
	}

	// check image size and orientation
	fgets(linebuf, line_size, fp);
	line = std::string(linebuf);

	std::vector<std::string> size_orient = tokenize__(line);

	header_.y_first_ = (size_orient[0].find("Y") != std::string::npos);

	int w = 3, h = 1;
	int x = 2, y = 0;
	int f = 0, s = 1;

	if(!header_.y_first_)
	{
		w = 1; h = 3;
		x = 0; y = 2;
		f = 1; s = 0;
	}

	converter__(size_orient[w], width_);
	converter__(size_orient[h], height_);

	// Set the reading order to fit yafaray's image coordinates
	bool from_left = (size_orient[x].find("+") != std::string::npos);
	bool from_top = (size_orient[y].find("-") != std::string::npos);

	header_.min_[f] = 0;
	header_.max_[f] = height_;
	header_.step_[f] = 1;

	header_.min_[s] = 0;
	header_.max_[s] = width_;
	header_.step_[s] = 1;

	if(!from_left)
	{
		header_.min_[s] = width_ - 1;
		header_.max_[s] = -1;
		header_.step_[s] = -1;
	}

	if(!from_top)
	{
		header_.min_[f] = height_ - 1;
		header_.max_[f] = -1;
		header_.step_[f] = -1;
	}

	free(linebuf);
	return true;
}

bool HdrHandler::readOrle(FILE *fp, int y, int scan_width)
{
	RgbePixel *scanline = new RgbePixel[scan_width]; // Scanline buffer
	int rshift = 0;
	int count;
	int x = header_.min_[1];

	RgbePixel pixel;

	while(x < scan_width)
	{
		if(fread((char *)&pixel, 1, sizeof(RgbePixel), fp) != sizeof(RgbePixel))
		{
			Y_ERROR << handler_name_ << ": An error has occurred while reading RLE scanline header..." << YENDL;
			return false;
		}

		// RLE encoded
		if(pixel.isOrleDesc())
		{
			count = pixel.getOrleCount(rshift);

			if(count > scan_width - x)
			{
				Y_ERROR << handler_name_ << ": Scanline width greater than image width..." << YENDL;
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
		if(header_.y_first_) img_buffer_.at(0)->setColor(x, y, scanline[j].getRgba(), color_space_, gamma_);
		else img_buffer_.at(0)->setColor(y, x, scanline[j].getRgba(), color_space_, gamma_);
		j++;
	}

	delete [] scanline;
	scanline = nullptr;

	return true;
}

bool HdrHandler::readArle(FILE *fp, int y, int scan_width)
{
	RgbePixel *scanline = new RgbePixel[scan_width]; // Scanline buffer
	YByte_t count = 0; // run description
	YByte_t col = 0; // color component

	if(scanline == nullptr)
	{
		Y_ERROR << handler_name_ << ": Unable to allocate buffer memory..." << YENDL;
		return false;
	}

	int j;

	// Read the 4 pieces of the scanline in order RGBE
	for(int chan = 0; chan < 4; chan++)
	{
		j = 0;
		while(j < scan_width)
		{
			if(fread((char *)&count, 1, 1, fp) != 1)
			{
				Y_ERROR << handler_name_ << ": An error has occurred while reading ARLE scanline..." << YENDL;
				return false;
			}

			if(count > 128)
			{
				count &= 0x7F; // get the value without the run flag (value mask: 01111111)

				if(count + j > scan_width)
				{
					Y_ERROR << handler_name_ << ": Run width greater than image width..." << YENDL;
					return false;
				}

				if(fread((char *)&col, 1, 1, fp) != 1)
				{
					Y_ERROR << handler_name_ << ": An error has occurred while reading ARLE scanline..." << YENDL;
					return false;
				}

				while(count--) scanline[j++][chan] = col;
			}
			else // else is a non-run raw values
			{
				if(count + j > scan_width)
				{
					Y_ERROR << handler_name_ << ": Non-run width greater than image width or equal to zero..." << YENDL;
					return false;
				}

				while(count--)
				{
					if(fread((char *)&col, 1, 1, fp) != 1)
					{
						Y_ERROR << handler_name_ << ": An error has occurred while reading ARLE scanline..." << YENDL;
						return false;
					}
					scanline[j++][chan] = col;
				}
			}
		}
	}

	j = 0;

	// put the pixels on the main buffer
	for(int x = header_.min_[1]; x != header_.max_[1]; x += header_.step_[1])
	{
		if(header_.y_first_) img_buffer_.at(0)->setColor(x, y, scanline[j].getRgba(), color_space_, gamma_);
		else img_buffer_.at(0)->setColor(y, x, scanline[j].getRgba(), color_space_, gamma_);
		j++;
	}

	delete [] scanline;
	scanline = nullptr;

	return true;
}

bool HdrHandler::saveToFile(const std::string &name, int img_index)
{
	int h = getHeight(img_index);
	int w = getWidth(img_index);

	std::ofstream file(name.c_str(), std::ios::out | std::ios::binary);

	if(!file.is_open())
	{
		return false;
	}
	else
	{
		std::string name_without_tmp = name;
		name_without_tmp.erase(name_without_tmp.length() - 4);
		if(session__.renderInProgress()) Y_INFO << handler_name_ << ": Autosaving partial render (" << roundFloatPrecision__(session__.currentPassPercent(), 0.01) << "% of pass " << session__.currentPass() << " of " << session__.totalPasses() << ") RGBE file as \"" << name_without_tmp << "\"...  " << getDenoiseParams() << YENDL;
		else Y_INFO << handler_name_ << ": Saving RGBE file as \"" << name_without_tmp << "\"...  " << getDenoiseParams() << YENDL;
		if(has_alpha_) Y_VERBOSE << handler_name_ << ": Ignoring alpha channel." << YENDL;

		writeHeader(file, img_index);

		RgbePixel signature; //scanline start signature for adaptative RLE
		signature.setScanlineStart(w); //setup the signature

		RgbePixel *scanline = new RgbePixel[w];

		// write using adaptive-rle encoding
		for(int y = 0; y < h; y++)
		{
			// write scanline start signature
			file.write((char *)&signature, sizeof(RgbePixel));

			// fill the scanline buffer
			for(int x = 0; x < w; x++)
			{
				scanline[x] = getPixel(x, y, img_index);
			}

			// write the scanline RLE compressed by channel in 4 separated blocks not as contigous pixels pixel blocks
			if(!writeScanline(file, scanline, img_index))
			{
				Y_ERROR << handler_name_ << ": An error has occurred during scanline saving..." << YENDL;
				return false;
			}
		}
		delete [] scanline;
		file.close();
	}

	Y_VERBOSE << handler_name_ << ": Done." << YENDL;

	return true;
}

bool HdrHandler::writeHeader(std::ofstream &file, int img_index)
{
	int h = getHeight(img_index);
	int w = getWidth(img_index);

	if(h <= 0 || w <= 0) return false;

	file << "#?" << header_.program_type_ << "\n";
	file << "# Image created with YafaRay\n";
	file << "EXPOSURE=" << header_.exposure_ << "\n";
	file << "FORMAT=32-bit_rle_rgbe\n\n";
	file << "-Y " << h << " +X " << w << "\n";

	return true;
}

bool HdrHandler::writeScanline(std::ofstream &file, RgbePixel *scanline, int img_index)
{
	int w = getWidth(img_index);

	int cur, beg_run, run_count, old_run_count, nonrun_count;
	YByte_t run_desc;

	// write the scanline RLE compressed by channel in 4 separated blocks not as contigous pixels pixel blocks
	for(int chan = 0; chan < 4; chan++)
	{
		cur = 0;

		while(cur < w)
		{
			beg_run = cur;
			run_count = old_run_count = 0;

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
				run_desc = 128 + old_run_count;
				file.write((const char *)&run_desc, 1);
				file.write((const char *)&scanline[cur][chan], 1);
				cur = beg_run;
			}

			// write non run bytes, until we get to big run
			while(cur < beg_run)
			{
				nonrun_count = beg_run - cur;

				// Non run count can't be greater than 128
				if(nonrun_count > 128) nonrun_count = 128;

				run_desc = nonrun_count;
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
				run_desc = 128 + run_count;
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

ImageHandler *HdrHandler::factory(ParamMap &params, RenderEnvironment &render)
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
	params.getParam("img_grayscale", img_grayscale);
	/*	//Denoise is not available for HDR/EXR images
	 * 	params.getParam("denoiseEnabled", denoiseEnabled);
	 *	params.getParam("denoiseHLum", denoiseHLum);
	 *	params.getParam("denoiseHCol", denoiseHCol);
	 *	params.getParam("denoiseMix", denoiseMix);
	 */
	ImageHandler *ih = new HdrHandler();

	ih->setTextureOptimization(TextureOptimization::None);

	if(for_output)
	{
		if(logger__.getUseParamsBadge()) height += logger__.getBadgeHeight();
		ih->initForOutput(width, height, render.getRenderPasses(), denoise_enabled, denoise_h_lum, denoise_h_col, denoise_mix, with_alpha, false, img_grayscale);
	}

	return ih;
}

END_YAFARAY
