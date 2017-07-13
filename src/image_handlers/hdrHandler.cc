/****************************************************************************
 *
 *      hdrHandler.cc: Radiance hdr format handler
 *      This is part of the yafray package
 *      Copyright (C) 2010 George Laskowsky Ziguilinsky (Glaskows)
 *						   Rodrigo Placencia Vazquez (DarkTide)
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

#include <core_api/environment.h>
#include <core_api/imagehandler.h>
#include <core_api/params.h>
#include <core_api/scene.h>
#include <utilities/math_utils.h>
#include <utilities/fileUtils.h>

#include <fstream>
#include <iostream>
#include <vector>
#include <iomanip>

#include "hdrUtils.h"

__BEGIN_YAFRAY

class hdrHandler_t: public imageHandler_t
{
public:
	hdrHandler_t();
	~hdrHandler_t();
	bool loadFromFile(const std::string &name);
	bool saveToFile(const std::string &name, int imgIndex = 0);
	static imageHandler_t *factory(paraMap_t &params, renderEnvironment_t &render);
	bool isHDR() { return true; }

private:
	bool writeHeader(std::ofstream &file, int imgIndex);
	bool writeScanline(std::ofstream &file, rgbePixel_t *scanline, int imgIndex);
	bool readHeader(FILE *fp); //!< Reads file header and detects if the file is valid
	bool readORLE(FILE *fp, int y, int scanWidth); //!< Reads the scanline with the original Radiance RLE schema or without compression
	bool readARLE(FILE *fp, int y, int scanWidth); //!< Reads a scanline with Adaptative RLE schema

	rgbeHeader_t header;
};

hdrHandler_t::hdrHandler_t()
{
	m_hasAlpha = false;
	m_MultiLayer = false;

	handlerName = "hdrHandler";
}

hdrHandler_t::~hdrHandler_t()
{
	clearImgBuffers();
}

bool hdrHandler_t::loadFromFile(const std::string &name)
{
	FILE *fp = fileUnicodeOpen(name, "rb");
	
	Y_INFO << handlerName << ": Loading image \"" << name << "\"..." << yendl;

	if(!fp)
	{
		Y_ERROR << handlerName << ": Cannot open file " << name << yendl;
		return false;
	}

	if (!readHeader(fp))
	{
		Y_ERROR << handlerName << ": An error has occurred while reading the header..." << yendl;
		fileUnicodeClose(fp);
		return false;
	}

	// discard old image data
	clearImgBuffers();

	m_hasAlpha = false;	//FIXME: why is alpha false in HDR??
	int nChannels = 4;	//FIXME: despite alpha being false, number of channels was 4 anyway. I'm keeping this just in case, but I think it should be 3??
	if(m_grayscale) nChannels = 1;
	else if(m_hasAlpha) nChannels = 4;

	imgBuffer.push_back(new imageBuffer_t(m_width, m_height, nChannels, getTextureOptimization()));

	int scanWidth = (header.yFirst) ? m_width : m_height;
	
	// run length encoding is not allowed so read flat and exit
	if ((scanWidth < 8) || (scanWidth > 0x7fff))
	{
		for (int y = header.min[0]; y != header.max[0]; y += header.step[0])
		{
			if (!readORLE(fp, y, scanWidth))
			{
				Y_ERROR << handlerName << ": An error has occurred while reading uncompressed scanline..." << yendl;
				fileUnicodeClose(fp);
				return false;
			}
		}
		fileUnicodeClose(fp);
		return true;
	}
	
	for (int y = header.min[0]; y != header.max[0]; y += header.step[0])
	{
		rgbePixel_t pix;

		if (fread((char *)&pix, 1, sizeof(rgbePixel_t), fp) != sizeof(rgbePixel_t))
		{
			Y_ERROR << handlerName << ": An error has occurred while reading scanline start..." << yendl;
			fileUnicodeClose(fp);
			return false;
		}

		if (feof(fp))
		{
			Y_ERROR << handlerName << ": EOF reached while reading scanline start..." << yendl;
			fileUnicodeClose(fp);
			return false;
		}
		
		if (pix.isARLEDesc()) // Adaptaive RLE schema encoding
		{
			if (pix.getARLECount() > scanWidth)
			{
				Y_ERROR << handlerName << ": Error reading, invalid ARLE scanline width..." << yendl;
				fileUnicodeClose(fp);
				return false;
			}

			if (!readARLE(fp, y, pix.getARLECount()))
			{
				Y_ERROR << handlerName << ": An error has occurred while reading ARLE scanline..." << yendl;
				fileUnicodeClose(fp);
				return false;
			}
		}
		else // Original RLE schema encoding or raw without compression
		{
			// rewind the read pixel to start reading from the begining of the scanline
			fseek(fp, (long int) -sizeof(rgbePixel_t), SEEK_CUR);

			if(!readORLE(fp, y, scanWidth))
			{
				Y_ERROR << handlerName << ": An error has occurred while reading RLE scanline..." << yendl;
				fileUnicodeClose(fp);
				return false;
			}
		}
	}

	fileUnicodeClose(fp);

	Y_VERBOSE << handlerName << ": Done." << yendl;

	return true;
}

bool hdrHandler_t::readHeader(FILE *fp)
{
	std::string line;
	int line_size = 1000;
	char* linebuf = (char*) malloc(line_size);

	fgets(linebuf, line_size, fp); 
	line = std::string(linebuf);

	if (line.find("#?") == std::string::npos)
	{
		Y_ERROR << handlerName << ": File is not a valid Radiance RBGE image..." << yendl;
		free(linebuf);
		return false;
	}

	size_t foundPos = 0;

	header.exposure = 1.f;

	// search for optional flags
	for(;;)
	{
		fgets(linebuf, line_size, fp); 
		line = std::string(linebuf);

		if (line == "" || line == "\n") break; // We found the end of the header tag section and we move on

		//Find variables
		// We only check for the most used tags and ignore the rest

		if((foundPos = line.find("FORMAT=")) != std::string::npos)
		{
			if(line.substr(foundPos+7).find("32-bit_rle_rgbe") == std::string::npos)
			{
				Y_ERROR << handlerName << ": Sorry this is an XYZE file, only RGBE images are supported..." << yendl;
				free(linebuf);
				return false;
			}
		}
		else if((foundPos = line.find("EXPOSURE=")) != std::string::npos)
		{
			float exp = 0.f;
			converter(line.substr(foundPos+9), exp);
			header.exposure *= exp; // Exposure is cumulative if several EXPOSURE tags exist on the file
		}
	}

	// check image size and orientation
	fgets(linebuf, line_size, fp); 
	line = std::string(linebuf);
	
	std::vector<std::string> sizeOrient = tokenize(line);
	
	header.yFirst = (sizeOrient[0].find("Y") != std::string::npos);

	int w = 3, h = 1;
	int x = 2, y = 0;
	int f = 0, s = 1;

	if(!header.yFirst)
	{
		w = 1; h = 3;
		x = 0; y = 2;
		f = 1; s = 0;
	}

	converter(sizeOrient[w], m_width);
	converter(sizeOrient[h], m_height);

	// Set the reading order to fit yafaray's image coordinates
	bool fromLeft = (sizeOrient[x].find("+") != std::string::npos);
	bool fromTop = (sizeOrient[y].find("-") != std::string::npos);

	header.min[f] = 0;
	header.max[f] = m_height;
	header.step[f] = 1;

	header.min[s] = 0;
	header.max[s] = m_width;
	header.step[s] = 1;

	if(!fromLeft)
	{
		header.min[s] = m_width - 1;
		header.max[s] = -1;
		header.step[s] = -1;
	}

	if(!fromTop)
	{
		header.min[f] = m_height - 1;
		header.max[f] = -1;
		header.step[f] = -1;
	}

	free(linebuf);
	return true;
}

bool hdrHandler_t::readORLE(FILE *fp, int y, int scanWidth)
{
	rgbePixel_t *scanline = new rgbePixel_t[scanWidth]; // Scanline buffer
	int rshift = 0;
	int count;
	int x = header.min[1];

	rgbePixel_t pixel;

	while (x < scanWidth)
	{
		if (fread((char *)&pixel, 1, sizeof(rgbePixel_t), fp) != sizeof(rgbePixel_t))
		{
			Y_ERROR << handlerName << ": An error has occurred while reading RLE scanline header..." << yendl;
			return false;
		}

		// RLE encoded
		if (pixel.isORLEDesc())
		{
			count = pixel.getORLECount(rshift);

			if (count > scanWidth - x)
			{
				Y_ERROR << handlerName << ": Scanline width greater than image width..." << yendl;
				return false;
			}

			pixel = scanline[x-1];

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
	for(int x = header.min[1]; x != header.max[1]; x += header.max[1])
	{
		if(header.yFirst) imgBuffer.at(0)->setColor(x, y, scanline[j].getRGBA(), m_colorSpace, m_gamma);
		else imgBuffer.at(0)->setColor(y, x, scanline[j].getRGBA(), m_colorSpace, m_gamma);
		j++;
	}

	delete [] scanline;
	scanline = nullptr;

	return true;
}

bool hdrHandler_t::readARLE(FILE *fp, int y, int scanWidth)
{
	rgbePixel_t *scanline = new rgbePixel_t[scanWidth]; // Scanline buffer
	yByte count = 0; // run description
	yByte col = 0; // color component
	
	if (scanline == nullptr)
	{
		Y_ERROR << handlerName << ": Unable to allocate buffer memory..." << yendl;
		return false;
	}

	int j;

	// Read the 4 pieces of the scanline in order RGBE
	for(int chan = 0; chan < 4; chan++)
	{
		j = 0;
		while(j < scanWidth)
		{
			if(fread((char *)&count, 1, 1, fp) != 1)
			{
				Y_ERROR << handlerName << ": An error has occurred while reading ARLE scanline..." << yendl;
				return false;
			}

			if (count > 128)
			{
				count &= 0x7F; // get the value without the run flag (value mask: 01111111)
				
				if (count + j > scanWidth)
				{
					Y_ERROR << handlerName << ": Run width greater than image width..." << yendl;
					return false;
				}

				if(fread((char *)&col, 1, 1, fp) != 1)
				{
					Y_ERROR << handlerName << ": An error has occurred while reading ARLE scanline..." << yendl;
					return false;
				}

				while(count--) scanline[j++][chan] = col;
			}
			else // else is a non-run raw values
			{
				if (count + j > scanWidth)
				{
					Y_ERROR << handlerName << ": Non-run width greater than image width or equal to zero..." << yendl;
					return false;
				}

				while(count--)
				{
					if(fread((char *)&col, 1, 1, fp) != 1)
					{
						Y_ERROR << handlerName << ": An error has occurred while reading ARLE scanline..." << yendl;
						return false;
					}
					scanline[j++][chan] = col;
				}
			}
		}
 	}
	
	j = 0;

	// put the pixels on the main buffer
	for(int x = header.min[1]; x != header.max[1]; x += header.step[1])
	{
		if(header.yFirst) imgBuffer.at(0)->setColor(x, y, scanline[j].getRGBA(), m_colorSpace, m_gamma);
		else imgBuffer.at(0)->setColor(y, x, scanline[j].getRGBA(), m_colorSpace, m_gamma);
		j++;
	}

	delete [] scanline;
	scanline = nullptr;

	return true;
}

bool hdrHandler_t::saveToFile(const std::string &name, int imgIndex)
{
	int h = getHeight(imgIndex);
	int w = getWidth(imgIndex);

	std::ofstream file(name.c_str(), std::ios::out | std::ios::binary);

	if (!file.is_open())
	{
		return false;
	}
	else
	{
		std::string nameWithoutTmp = name;
		nameWithoutTmp.erase(nameWithoutTmp.length()-4);
		if(session.renderInProgress()) Y_INFO << handlerName << ": Autosaving partial render (" << RoundFloatPrecision(session.currentPassPercent(), 0.01) << "% of pass " << session.currentPass() << " of " << session.totalPasses() << ") RGBE file as \"" << nameWithoutTmp << "\"...  " << getDenoiseParams()  << yendl;
		else Y_INFO << handlerName << ": Saving RGBE file as \"" << nameWithoutTmp << "\"...  " << getDenoiseParams() << yendl;
		if (m_hasAlpha) Y_VERBOSE << handlerName << ": Ignoring alpha channel." << yendl;

		writeHeader(file, imgIndex);

		rgbePixel_t signature; //scanline start signature for adaptative RLE
		signature.setScanlineStart(w); //setup the signature

		rgbePixel_t *scanline = new rgbePixel_t[w];

		// write using adaptive-rle encoding
		for (int y = 0; y < h; y++)
		{
			// write scanline start signature
			file.write((char *)&signature, sizeof(rgbePixel_t));

			// fill the scanline buffer
			for (int x = 0; x < w; x++)
			{
				scanline[x] = getPixel(x, y, imgIndex);
			}

			// write the scanline RLE compressed by channel in 4 separated blocks not as contigous pixels pixel blocks
			if (!writeScanline(file, scanline, imgIndex))
			{
				Y_ERROR << handlerName << ": An error has occurred during scanline saving..." << yendl;
				return false;
			}
		}
		delete [] scanline;
		file.close();
	}

	Y_VERBOSE << handlerName << ": Done." << yendl;

	return true;
}

bool hdrHandler_t::writeHeader(std::ofstream &file, int imgIndex)
{
	int h = getHeight(imgIndex);
	int w = getWidth(imgIndex);

	if (h <= 0 || w <=0) return false;

	file<<"#?"<<header.programType<<"\n";
	file<<"# Image created with YafaRay\n";
	file<<"EXPOSURE="<<header.exposure<<"\n";
	file<<"FORMAT=32-bit_rle_rgbe\n\n";
	file<<"-Y "<<h<<" +X "<<w<<"\n";

	return true;
}

bool hdrHandler_t::writeScanline(std::ofstream &file, rgbePixel_t *scanline, int imgIndex)
{
	int w = getWidth(imgIndex);

	int cur, beg_run, run_count, old_run_count, nonrun_count;
	yByte runDesc;

	// write the scanline RLE compressed by channel in 4 separated blocks not as contigous pixels pixel blocks
	for (int chan = 0; chan < 4; chan++)
	{
		cur = 0;

		while(cur < w)
		{
			beg_run = cur;
			run_count = old_run_count = 0;

			while ((run_count < 4) && (beg_run < w))
			{
				beg_run += run_count;
				old_run_count = run_count;
				run_count = 1;
				while ((scanline[beg_run][chan] == scanline[beg_run + run_count][chan]) && (beg_run + run_count < w) && (run_count < 127))
				{
					run_count++;
				}
			}

			// write short run
			if ((old_run_count > 1) && (old_run_count == beg_run - cur))
			{
				runDesc = 128 + old_run_count;
				file.write((const char*)&runDesc, 1);
				file.write((const char*)&scanline[cur][chan], 1);
				cur = beg_run;
			}

			// write non run bytes, until we get to big run
			while(cur < beg_run)
			{
				nonrun_count = beg_run - cur;

				// Non run count can't be greater than 128
				if (nonrun_count > 128) nonrun_count = 128;

				runDesc = nonrun_count;
				file.write((const char*)&runDesc, 1);

				for(int i = 0; i < nonrun_count; i++)
				{
					file.write((const char*)&scanline[cur + i][chan], 1);
				}

				cur += nonrun_count;
			}

			// write out next run if one was found
			if (run_count >= 4)
			{
				runDesc = 128 + run_count;
				file.write((const char*)&runDesc, 1);
				file.write((const char*)&scanline[beg_run][chan], 1);
				cur += run_count;
			}

			// this shouldn't happend <<< if it happens then there is a logic error on the function :P, DarkTide
			if (cur > w) return false;
		}
	}

	return true;
}

imageHandler_t *hdrHandler_t::factory(paraMap_t &params,renderEnvironment_t &render)
{
	int width = 0;
	int height = 0;
	bool withAlpha = false;
	bool forOutput = true;
	bool img_grayscale = false;
	bool denoiseEnabled = false;
	int denoiseHLum = 3;
	int denoiseHCol = 3;
	float denoiseMix = 0.8f;

	params.getParam("width", width);
	params.getParam("height", height);
	params.getParam("alpha_channel", withAlpha);
	params.getParam("for_output", forOutput);
	params.getParam("img_grayscale", img_grayscale);
/*	//Denoise is not available for HDR/EXR images
 * 	params.getParam("denoiseEnabled", denoiseEnabled);
 *	params.getParam("denoiseHLum", denoiseHLum);
 *	params.getParam("denoiseHCol", denoiseHCol);
 *	params.getParam("denoiseMix", denoiseMix);
 */
	imageHandler_t *ih = new hdrHandler_t();

	ih->setTextureOptimization(TEX_OPTIMIZATION_NONE);

	if(forOutput)
	{
		if(yafLog.getUseParamsBadge()) height += yafLog.getBadgeHeight();
		ih->initForOutput(width, height, render.getRenderPasses(), denoiseEnabled, denoiseHLum, denoiseHCol, denoiseMix, withAlpha, false, img_grayscale);
	}

	return ih;
}

extern "C"
{

	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerImageHandler("hdr", "hdr pic", "HDR [Radiance RGBE]", hdrHandler_t::factory);
	}

}
__END_YAFRAY
