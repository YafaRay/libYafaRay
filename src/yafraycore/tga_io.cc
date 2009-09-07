
#include <yafraycore/tga_io.h>
#include <utilities/buffer.h>
#include <iostream>
#include <stdio.h>


__BEGIN_YAFRAY

using namespace std;

const unsigned char TGAHDR[12] = {0, 0, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const char *TGA_FOOTER = "TRUEVISION-XFILE.\0";

outTga_t::outTga_t(int resx, int resy, const char *fname, bool sv_alpha)
{
	unsigned int tsz = resx*resy;
	data = new unsigned char[tsz*3];
	sizex = resx;
	sizey = resy;
	outfile = fname;

	alpha_buf = NULL;
	save_alpha = sv_alpha;
	if (save_alpha)
	{
		alpha_buf = new unsigned char[tsz];
	}
}

/* bool outTga_t::putPixel(int x, int y, const color_t &c, 
		CFLOAT alpha,PFLOAT depth)
{
	unsigned int yx = sizex*y + x;
	(data+yx*3) << c;
	if (save_alpha)
		alpha_buf[yx] = (unsigned char)(255.0*((alpha<0)?0:((alpha>1)?1:alpha)));
	return true;
} */

bool outTga_t::putPixel(int x, int y, const float *c, int channels)
{
	unsigned int yx = sizex*y + x;
	unsigned char *pix = (data+yx*3);
	pix[0]= (c[0]<0.f) ? 0 : ((c[0]>=1.f) ? 255 : (unsigned char)(255.f*c[0]) );
	pix[1]= (c[1]<0.f) ? 0 : ((c[1]>=1.f) ? 255 : (unsigned char)(255.f*c[1]) );
	pix[2]= (c[2]<0.f) ? 0 : ((c[2]>=1.f) ? 255 : (unsigned char)(255.f*c[2]) );
	if (save_alpha && channels > 4)
		alpha_buf[yx] = (unsigned char)(255.0*((c[4]<0)?0:((c[4]>1)?1:c[4])));
	return true;
}

outTga_t::~outTga_t()
{
	if (data) {
		delete[] data;
		data = NULL;
	}
	if (alpha_buf) {
		delete[] alpha_buf;
		alpha_buf = NULL;
	}
}

bool outTga_t::savetga(const char* filename)
{
	// name is assigned by default
	cout << "Saving Targa file as \"" << filename << "\": ";

	FILE* fp;
	unsigned short w, h, x, y;
	unsigned char* yscan;
	unsigned char btsdesc[2];
	unsigned int dto;
	if (save_alpha) {
		btsdesc[0] = 0x20; // 32 bits
		btsdesc[1] = 0x28; // topleft / 8 bit alpha
	}
	else {
		btsdesc[0] = 0x18; // 24 bits
		btsdesc[1] = 0x20; // topleft / no alpha
	}
	w = (unsigned short)sizex;
	h = (unsigned short)sizey;
	fp = fopen(filename, "wb");
	if (fp == NULL)
		return false;
	else 
	{
		fwrite(&TGAHDR, 12, 1, fp);
		fputc(w, fp);
		fputc(w>>8, fp);
		fputc(h, fp);
		fputc(h>>8, fp);
		fwrite(&btsdesc, 2, 1, fp);
		for (y=0; y<h; y++) 
		{
			// swap R & B channels
			dto = y*w;
			yscan = &data[dto*3];
			for (x=0; x<w; x++, yscan+=3) 
			{
				fputc(*(yscan+2), fp);
				fputc(*(yscan+1), fp);
				fputc(*yscan,  fp);
				if (save_alpha) fputc(alpha_buf[dto+x], fp);
			}
		}
		// write targa 2.0 footer:
		for(int i=0; i<8; ++i) fputc(0, fp);
		for(int i=0; i<18; ++i) fputc(TGA_FOOTER[i], fp);
		fclose(fp);
		cout<<"OK"<<endl;
		return true;
	}
}

/*=============================================================
/  TGA loading code
=============================================================*/

enum TGA_TYPE_t{TGA_NO_DATA=0, TGA_UNC_CMAP, TGA_UNC_TRUE, TGA_UNC_GRAY,
		TGA_RLE_CMAP=9, TGA_RLE_TRUE, TGA_RLE_GRAY} TGA_TYPE;

/* void getColor(FILE *fp, unsigned char RGBA[4], unsigned int byte_per_pix, bool has_alpha, unsigned char *colmap)
{
	unsigned char c1=0, c2=0;
	unsigned short cmap_idx=0;
	if(byte_per_pix==1) {
		RGBA[0] = RGBA[1] = RGBA[2] = (unsigned char)fgetc(fp);
		// a bit strange this, but works..
		if(colmap) cmap_idx = (unsigned short)(RGBA[0]<<2);
	}
	else if (byte_per_pix==2)
	{
		// 16 bit color
		c1 = (unsigned char)fgetc(fp);
		c2 = (unsigned char)fgetc(fp);
		if (colmap) cmap_idx = (unsigned short)(c1 + (c2<<8));
		else
		{
			RGBA[2] = (unsigned char)(((c1 & 31)*255)/31);
			RGBA[1] = (unsigned char)(((((c1 & 0xe0)>>5) + ((c2 & 3)<<3))*255)/31);
			RGBA[0] = (unsigned char)(((c2>>2)*255)/31);
		}
	}
	else
	{
		RGBA[2] = (unsigned char)fgetc(fp);
		RGBA[1] = (unsigned char)fgetc(fp);
		RGBA[0] = (unsigned char)fgetc(fp);
	}
	if(colmap)
	{
		// get all colors from colormap
		RGBA[0] = colmap[cmap_idx++];
		RGBA[1] = colmap[cmap_idx++];
		RGBA[2] = colmap[cmap_idx++];
		RGBA[3] = colmap[cmap_idx];
	}
	else if (has_alpha)
	{
		// get alpha value, for 16 bit case, 'hidden' in c2
		if (byte_per_pix==2)
			RGBA[3] = (c2 & 128) ? 0 : 255;
		else if (byte_per_pix==1) RGBA[3]=RGBA[0];
		else RGBA[3]=(unsigned char)fgetc(fp);
	}
} */

void getColor(unsigned char *c, unsigned char RGBA[4], unsigned int byte_per_pix, bool has_alpha, unsigned char *colmap)
{
	unsigned short cmap_idx=0;
	if(byte_per_pix==1) {
		RGBA[0] = RGBA[1] = RGBA[2] = c[0];
		// a bit strange this, but works..
		if(colmap) cmap_idx = (unsigned short)(RGBA[0]<<2);
	}
	else if (byte_per_pix==2)
	{
		// 16 bit color
		if (colmap) cmap_idx = (unsigned short)(c[0] + (c[1]<<8));
		else
		{
			RGBA[2] = (unsigned char)(((c[0] & 31)*255)/31);
			RGBA[1] = (unsigned char)(((((c[0] & 0xe0)>>5) + ((c[1] & 3)<<3))*255)/31);
			RGBA[0] = (unsigned char)(((c[1]>>2)*255)/31);
		}
	}
	else
	{
		RGBA[2] = c[0];
		RGBA[1] = c[1];
		RGBA[0] = c[2];
	}
	if(colmap)
	{
		// get all colors from colormap
		RGBA[0] = colmap[cmap_idx++];
		RGBA[1] = colmap[cmap_idx++];
		RGBA[2] = colmap[cmap_idx++];
		RGBA[3] = colmap[cmap_idx];
	}
	else if (has_alpha)
	{
		// get alpha value, for 16 bit case, 'hidden' in c[1]
		if (byte_per_pix==2) RGBA[3] = (c[1] & 128) ? 0 : 255;
		else if (byte_per_pix==1) RGBA[3]=RGBA[0];
		else RGBA[3]=c[3];
	}
}

YAFRAYCORE_EXPORT gBuf_t<unsigned char, 4> * load_tga(const char *name)
{
	// The disadvantage of old targa images is that they have no "magic bytes"
	// this means that basically any file could be accepted as tga.
	unsigned char alpha_bits, cmap_bits, img_ori;
	unsigned char *COLMAP=0; //possible colormap
	unsigned char header[19];
	unsigned short width, height, cmap_len;
	unsigned short x, y;
	bool has_alpha, isgray, has_cmap, is_cmap, compressed;
	unsigned int byte_per_pix, idlen, pt, scan_pt/* , f_size, read_bytes */;
	
	FILE *fp = fopen(name, "rb");
	if(!fp)
	{
		std::cerr << "load_tga: Cannot open file\n";
		return NULL;
	}
	//fseek (fp , 0 , SEEK_END);
	//f_size = ftell (fp);
	//rewind (fp);
	fread(&header, 1, 18, fp);
	
	has_cmap = header[1] == 1;
	is_cmap = ((header[2]==TGA_UNC_CMAP) || (header[2]==TGA_RLE_CMAP));
	if( is_cmap && !has_cmap ) //error, colormapped image without color map...
	{
		fclose(fp);
		return 0;
	}
	
	cmap_bits = header[7];
	if (is_cmap && (cmap_bits!=15) && (cmap_bits!=16) && (cmap_bits!=24) && (cmap_bits!=32))
	{
		std::cerr << "Unsupported colormap bitformat\n";
		fclose(fp);
		return 0;
	}
	
	if ((header[2]!=TGA_UNC_TRUE) && (header[2]!=TGA_UNC_GRAY) &&
		(header[2]!=TGA_RLE_TRUE) && (header[2]!=TGA_RLE_GRAY) && !is_cmap)
	{
		// 'no-image-data' not supported, possibly other unknown type
		//err_str = "Targa type not supported";
		fclose(fp);
		return 0;
	}
	
	isgray = ((header[2]==TGA_UNC_GRAY) || (header[2]==TGA_RLE_GRAY));
	compressed = ((header[2]==TGA_RLE_TRUE) || (header[2]==TGA_RLE_GRAY) ||	(header[2]==TGA_RLE_CMAP));
	
	// width & height (or any non-char) read as little endian
	width = (unsigned short)(header[12] | (header[13]<<8));
	height = (unsigned short)(header[14] | (header[15]<<8));
	byte_per_pix = (unsigned char)(header[16]>>3);
	alpha_bits = (unsigned char)(header[17] & 15);
	has_alpha = (alpha_bits!=0);
	img_ori = (unsigned char)((header[17] & 48)>>4);
	
	if(isgray)
	{
		if ((byte_per_pix!=1) && (byte_per_pix!=2)) {
			//err_str = "Unsupported grayscale image format";
			fclose(fp);
			return 0;
		}
	}
	else if(is_cmap) {
		if(byte_per_pix>2) {
			// maybe this is actually correct, but ignore for now...
			//err_str = "24/32 bit colormap index???";
			fclose(fp);
			return 0;
		}
	}
	else {
		if ((byte_per_pix<2) || (byte_per_pix>4)) {
			//err_str = "Unsupported pixelformat, only accept 15/16-, 24- or 32-bit";
			fclose(fp);
			return 0;
		}
	}

	if (has_alpha) {
		if ((!((byte_per_pix==1) && (alpha_bits==8))) &&
				(!((byte_per_pix==2) && (alpha_bits==1))) &&
				(!((byte_per_pix==4) && (alpha_bits==8)))) {
			//err_str = "Unsupported alpha format";
			fclose(fp);
			return 0;
		}
	}

	// read past any id
	idlen = (unsigned int)header[0];
	if (idlen) fseek(fp, idlen, SEEK_CUR);
	
	// read in colormap if needed
	if (is_cmap)
	{
		cmap_len = (unsigned short)(header[5] | (header[6]<<8));
		COLMAP = new unsigned char[cmap_len<<2]; //always expanded to 32 bit
		unsigned char * cpt = COLMAP;
		if (cmap_bits<=16) {
			// 15/16 bit
			for(x=0;x<cmap_len;x++) {
				unsigned char c1 = (unsigned char)fgetc(fp);
				unsigned char c2 = (unsigned char)fgetc(fp);
				// RGBA in order
				*cpt++ = (unsigned char)(((c2>>2)*255)/31);
				*cpt++ = (unsigned char)(((((c1 & 0xe0)>>5) + ((c2 & 3)<<3))*255)/31);
				*cpt++ = (unsigned char)(((c1 & 31)*255)/31);
				if (cmap_bits==16)
					*cpt++ = (unsigned char)((c2 & 128)*255);
				else
					*cpt++ = 0;
			}
		}
		else {
			// 24/32 bit
			for (x=0;x<cmap_len;x++) {
				*(cpt+2) = (unsigned char)fgetc(fp);
				*(cpt+1) = (unsigned char)fgetc(fp);
				*(cpt) = (unsigned char)fgetc(fp);
				if (cmap_bits==32)
					*(cpt+3) = (unsigned char)fgetc(fp);
				else
					*(cpt+3) = 0;
				cpt += 4;
			}
		}
	}
	
	unsigned int totsize = width*height*4;
	cBuffer_t* imgdata = new cBuffer_t(width, height);
	unsigned char* datap = (*imgdata)(0, 0);
	unsigned char RGBA[4] = {0, 0, 0, 255}, packet[128*4]; //max size for raw packet
	
	pt = 0;	// image data pointer
	if (compressed) {
		// RUN LENGTH ENCODED
		while (pt<totsize) {
			if(feof(fp)){ std::cout << "incomplete file!\n"; break; }
			unsigned char repcnt = (unsigned char)fgetc(fp);
			if(feof(fp)){ std::cout << "incomplete file!\n"; break; }
			
			bool rle_packet = ((repcnt & 0x80)!=0);
			repcnt = (unsigned char)((repcnt & 0x7f)+1);
			if( (pt+4*repcnt) > totsize){ std::cout << "possibly erroneous decompression!" << std::endl; break; }
			if (rle_packet) {
				if( fread(packet, byte_per_pix, 1, fp) != 1 ){ std::cout << "incomplete file!\n"; break; }
				getColor(packet, RGBA, byte_per_pix, has_alpha, COLMAP);
				for (x=0;x<(unsigned short)repcnt;x++) {
					datap[pt++] = RGBA[0];
					datap[pt++] = RGBA[1];
					datap[pt++] = RGBA[2];
					datap[pt++] = RGBA[3];
				}
			}
			else { // raw packet
				if( fread(packet, byte_per_pix, repcnt, fp) != repcnt ){ std::cout << "incomplete file!\n"; break; }
				for (x=0;x<(unsigned short)repcnt;x++) {
					getColor(packet+x*byte_per_pix, RGBA, byte_per_pix, has_alpha, COLMAP);
					datap[pt++] = RGBA[0];
					datap[pt++] = RGBA[1];
					datap[pt++] = RGBA[2];
					datap[pt++] = RGBA[3];
				}
			}
		}
	}
	else {
		// UNCOMPRESSED IMAGE
		unsigned char *scanline = new unsigned char[width*byte_per_pix];
		for (y=0;y<height;y++) {
			if( fread(scanline, byte_per_pix, width, fp) != width ){ std::cout << "incomplete file!\n"; break; }
			for(x=0, scan_pt=0;x<width;x++, scan_pt+=byte_per_pix)
			{
				getColor(scanline+x*byte_per_pix, RGBA, byte_per_pix, has_alpha, COLMAP);
				datap[pt++] = RGBA[0];
				datap[pt++] = RGBA[1];
				datap[pt++] = RGBA[2];
				datap[pt++] = RGBA[3];
			}
		}
		delete[] scanline;
	}

	fclose(fp);
	fp = NULL;

	// flip data if needed
	const int imgbp = 4;	//cbuffer byte per pix.
	unsigned short ofy = (unsigned short)(width*imgbp);	// row byte offset
	if ((img_ori & 2)==0) {
		unsigned char *bot, *top, tmp;
		for (y=0;y<(height>>1);y++) {
			top = (*imgdata)(0, y);
			bot = (*imgdata)(0, (height-1)-y);
			for (x=0;x<width*imgbp;x++, top++, bot++) {
				tmp = *top;
				*top = *bot;
				*bot = tmp;
			}
		}
	}
	if (img_ori & 1) {
		unsigned char *lt, *rt, tmp;
		for (y=0;y<height;y++) {
			lt = (*imgdata)(y, 0);
			rt = lt + (ofy-imgbp);
			for (x=0;x<(width>>1);x++, rt-=imgbp*2) {
				tmp=*lt;  *lt++=*rt; *rt++=tmp;
				tmp=*lt;  *lt++=*rt; *rt++=tmp;
				tmp=*lt;  *lt++=*rt; *rt++=tmp;
				tmp=*lt;  *lt++=*rt; *rt++=tmp;
			}
		}
	}

	//err_str = "No errors";	
	return imgdata;
}

__END_YAFRAY
