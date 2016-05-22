
#ifndef Y_IMAGESPLITTER_H
#define Y_IMAGESPLITTER_H

#include <yafray_config.h>

#include <vector>
#include <cmath>

#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/vector.hpp>

__BEGIN_YAFRAY

struct renderArea_t
{
	renderArea_t(int x,int y,int w,int h):X(x),Y(y),W(w),H(h),
		realX(x),realY(y),realW(w),realH(h),resample(w*h)
	{};
	renderArea_t() {};

	void set(int x,int y,int w,int h)
	{
		realX=X=x;
		realY=Y=y;
		realW=W=w;
		realH=H=h;
//		image.resize(w*h);
//		depth.resize(w*h);
		resample.resize(w*h);
	}
	void setReal(int x,int y,int w,int h)
	{
		realX=x;
		realY=y;
		realW=w;
		realH=h;
	}
	bool checkResample(CFLOAT threshold);
//	bool out(colorOutput_t &o);

//	colorA_t & imagePixel(int x,int y) {return image[(y-Y)*W+(x-X)];};
//	PFLOAT & depthPixel(int x,int y)   {return depth[(y-Y)*W+(x-X)];};
	bool  resamplePixel(int x,int y)  {return resample[(y-Y)*W+(x-X)];};

	int X,Y,W,H,realX,realY,realW,realH;
	int sx0, sx1, sy0, sy1; //!< safe area, i.e. region unaffected by samples outside (needs to be set by ImageFilm_t)
//	std::vector<colorA_t> image;
//	std::vector<PFLOAT> depth;
	std::vector<bool> resample;

	friend class boost::serialization::access;
	template<class Archive> void serialize(Archive & ar, const unsigned int version)
	{
		ar & BOOST_SERIALIZATION_NVP(X);
		ar & BOOST_SERIALIZATION_NVP(Y);
		ar & BOOST_SERIALIZATION_NVP(W);
		ar & BOOST_SERIALIZATION_NVP(H);
		ar & BOOST_SERIALIZATION_NVP(realX);
		ar & BOOST_SERIALIZATION_NVP(realY);
		ar & BOOST_SERIALIZATION_NVP(realW);
		ar & BOOST_SERIALIZATION_NVP(realH);
		ar & BOOST_SERIALIZATION_NVP(sx0);
		ar & BOOST_SERIALIZATION_NVP(sx1);
		ar & BOOST_SERIALIZATION_NVP(sy0);
		ar & BOOST_SERIALIZATION_NVP(sy1);
		ar & BOOST_SERIALIZATION_NVP(resample);
	}
};

/*!	Splits the image to be rendered into pieces, e.g. "buckets" for
	different threads.
	CAUTION! Some methods need to be thread save!
*/
class imageSpliter_t
{
	public:
		enum tilesOrderType { LINEAR, RANDOM, CENTRE_RANDOM };
		imageSpliter_t() {};
		imageSpliter_t(int w, int h, int x0,int y0, int bsize, tilesOrderType torder, int nthreads);
		/* return the n-th area to be rendered.
			\return false if n is out of range, true otherwise
		*/
		bool getArea(int n, renderArea_t &area);

		bool empty()const {return regions.empty();};
		int size()const {return regions.size();};

	protected:
	friend class imageSpliterCentreSorter_t;
		struct region_t
		{
			int x,y,w,h;
//			int rx,ry,rw,rh;
			template<class Archive> void serialize(Archive & ar, const unsigned int version)
			{
				ar & BOOST_SERIALIZATION_NVP(x);
				ar & BOOST_SERIALIZATION_NVP(y);
				ar & BOOST_SERIALIZATION_NVP(w);
				ar & BOOST_SERIALIZATION_NVP(h);
			}
		};
		int width,height,blocksize;
		std::vector<region_t> regions;
		tilesOrderType tilesorder;

		friend class boost::serialization::access;
		template<class Archive> void serialize(Archive & ar, const unsigned int version)
		{
			ar & BOOST_SERIALIZATION_NVP(width);
			ar & BOOST_SERIALIZATION_NVP(height);
			ar & BOOST_SERIALIZATION_NVP(blocksize);
			ar & BOOST_SERIALIZATION_NVP(regions);
			ar & BOOST_SERIALIZATION_NVP(tilesorder);
		}
};

class imageSpliterCentreSorter_t {
      int imageW, imageH, imageX0, imageY0;
public:
      imageSpliterCentreSorter_t(int image_w, int image_h, int image_x0, int image_y0) : imageW(image_w), imageH(image_h), imageX0(image_x0), imageY0(image_y0) {}
      bool operator()(imageSpliter_t::region_t const & a, imageSpliter_t::region_t const & b) const
      {
            return ((a.x-imageX0 - imageW/2) * (a.x-imageX0 - imageW/2) + (a.y-imageY0 - imageH/2) * (a.y-imageY0 - imageH/2)) < ((b.x-imageX0 - imageW/2) * (b.x-imageX0 - imageW/2) + (b.y-imageY0 - imageH/2) * (b.y-imageY0 - imageH/2));
      }
};


__END_YAFRAY

#endif // Y_IMAGESPLITTER_H
