#ifndef Y_SCRHALTON_H
#define Y_SCRHALTON_H

#include <yafray_config.h>

extern YAFRAYCORE_EXPORT const int* faure[];

__BEGIN_YAFRAY

const int prims[51] = { 1, 2, 3, 5, 7, 11, 13, 17, 19, 23, 29,
						31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
						73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 
						127, 131, 137, 139, 149, 151, 157, 163, 167, 173,
						179, 181, 191, 193, 197, 199, 211, 223, 227, 229  };

/** Low Discrepancy Halton sampling */
// dim MUST NOT be larger than 50! Above that, random numbers may be
// the better choice anyway, not even scrambling is realiable at high dimensions.
inline double scrHalton(int dim, unsigned int n)
{
	const int *sigma = faure[dim];
	unsigned int base = prims[dim];
	double invBase = 1.0/double(base);
	double value = 0.0;
	double f, factor;
	
	f = factor = invBase;
	while (n>0) {
		value += double(sigma[n % base]) * factor;
		n /= base;
		factor *= f;
	}
	return value;
}






__END_YAFRAY

#endif // Y_SCRHALTON_H
