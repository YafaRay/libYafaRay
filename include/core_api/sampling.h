
#ifndef Y_SAMPLING_H
#define Y_SAMPLING_H

#include<yafray_config.h>

#include "imagesplitter.h"
#include <vector>

__BEGIN_YAFRAY


class sampler_t
{
	public:
	sampler_t(int width, int height);
	/*! add dimension to the sampler
		\return reached dimension, needed to request the samples */
	int addDimension(int max_samples);
	/*! set the image area to generate samples for; remember that each thread
		has its own sampler instance, which gets assigned image areas this way
		until the whole image has been rendered.
	*/
	bool setArea(const renderArea_t &a);
	//! get the next value in the sequence for dimension
	float getNext(unsigned int dimension);
//	bool getSample(sample_t &s);
	protected:
	std::vector<sequence_t> dim; //!< strore the sequences for each dimension
}



__END_YAFRAY

#endif // Y_SAMPLING_H
