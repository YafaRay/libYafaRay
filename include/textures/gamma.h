
#ifndef Y_GAMMA_H
#define Y_GAMMA_H

#include <core_api/color.h>

__BEGIN_YAFRAY

class gammaLUT_t
{
	public:
		gammaLUT_t(float gamma): g(gamma)
		{
			setGamma(g);
		}
		void setGamma(float gamma)
		{
			g = gamma;
			LUT[0]=0;
			for(int i=1;i<256;i++)
			{
				LUT[i] = fPow( 0.00392156862745098039 * (float)i, gamma ); // constant = 1.f/255.f
			}
		}
		float getGamma() { return g; }
		const CFLOAT& operator[] (int i) const { return LUT[i]; }
		void operator()(unsigned char *data, colorA_t &col) const
		{
			col.set( LUT[ data[0] ],
					 LUT[ data[1] ],
					 LUT[ data[2] ],
					 LUT[ data[3] ] );
		}
	protected:
		float g;
		CFLOAT LUT[256];
};


__END_YAFRAY

#endif // Y_GAMMA_H
