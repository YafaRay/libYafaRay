//--------------------------------------------------------------------------------
// OpenEXR image load/save
//--------------------------------------------------------------------------------

#include <ImfOutputFile.h>
#include <ImfChannelList.h>
#include <ImfRgbaFile.h>
#include <ImfArray.h>
#include <ImfVersion.h>

#include <yafraycore/EXR_io.h>

using namespace std;
using namespace Imf;
using namespace Imath;

__BEGIN_YAFRAY

bool saveEXR(const char* fname, fcBuffer_t* fbuf, gBuf_t<float, 1>* zbuf, int width, int height, const std::string &outflags)
{
	PixelType pt = HALF;
	int chan_size = sizeof(half);
	if (int(outflags.find("float"))!=-1) {
		pt = FLOAT;
		chan_size = sizeof(float);
	}

	const int num_colchan = 4;
	int totchan_size = num_colchan*chan_size;

	Header header(width, height);

	// set compression type, zip default
	if (int(outflags.find("compression_none"))!=-1)
		header.compression() = NO_COMPRESSION;
	else if (int(outflags.find("compression_piz"))!=-1)
		header.compression() = PIZ_COMPRESSION;
	else if (int(outflags.find("compression_rle"))!=-1)
		header.compression() = RLE_COMPRESSION;
	else if (int(outflags.find("compression_pxr24"))!=-1)
		header.compression() = PXR24_COMPRESSION;
	else
		header.compression() = ZIP_COMPRESSION;

	header.channels().insert("R", Channel(pt));
	header.channels().insert("G", Channel(pt));
	header.channels().insert("B", Channel(pt));
	header.channels().insert("A", Channel(pt));

	float* fbufp = (*fbuf)(0, 0);
	char* tbufp = (char*)fbufp;
	Array<half> tbuf;
	if (pt==HALF) {
		// float->half
		int i = num_colchan*width*height;
		tbuf.resizeErase(i);
		while (--i) tbuf[i] = fbufp[i];
		tbufp = (char*)&tbuf[0];
	}

	FrameBuffer fb;
	fb.insert("R", Slice(pt, tbufp,               totchan_size, width*totchan_size));
	fb.insert("G", Slice(pt, tbufp +   chan_size, totchan_size, width*totchan_size));
	fb.insert("B", Slice(pt, tbufp + 2*chan_size, totchan_size, width*totchan_size));
	fb.insert("A", Slice(pt, tbufp + 3*chan_size, totchan_size, width*totchan_size));

	// zbuffer
	if (zbuf) {
		header.channels().insert("Z", Channel(FLOAT));
		fb.insert("Z", Slice(FLOAT, (char *)(*zbuf)(0, 0), sizeof(float), width*sizeof(float)));
	}

	OutputFile file(fname, header);
	file.setFrameBuffer(fb);
	try {
		file.writePixels(height);
		return true;
	}
	catch (const std::exception &exc) {
		cerr << "[saveEXR]: " << exc.what() << endl;
		return false;
	}
}

bool isEXR(const char* fname)
{
	FILE* fp = fopen(fname, "rb");
	if (fp) {
		char bytes[4];
		fread(&bytes, 1, 4, fp);
		fclose(fp);
		fp = NULL;
		return isImfMagic(bytes);
	}
	return false;
}

YAFRAYCORE_EXPORT fcBuffer_t* loadEXR(const char* fname)
{
	if (!isEXR(fname)) return NULL;
	try
	{
		RgbaInputFile file(fname);
		Box2i dw = file.dataWindow();

		int width  = dw.max.x - dw.min.x + 1;
		int height = dw.max.y - dw.min.y + 1;
		Array<Rgba> rgba(width*height);

		file.setFrameBuffer(&rgba[0] - dw.min.x - dw.min.y*width, 1, width);
		file.readPixels(dw.min.y, dw.max.y);

		fcBuffer_t* fbuf = new fcBuffer_t(width, height);
		float* fbufp = (*fbuf)(0, 0);
		for (int i=0; i<(width*height); ++i) {
			*fbufp++ = rgba[i].r;
			*fbufp++ = rgba[i].g;
			*fbufp++ = rgba[i].b;
			*fbufp++ = rgba[i].a;
		}
		return fbuf;
	}
	catch (const std::exception &exc) {
		cerr << "[loadEXR]: " << exc.what() << endl;
		return NULL;
	}
}

bool outEXR_t::SaveEXR()
{
	return saveEXR(filename, fbuf, zbuf, sizex, sizey, out_flags);
}

__END_YAFRAY
