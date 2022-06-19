#pragma once
/****************************************************************************
 *      This is part of the libYafaRay package
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

#ifndef YAFARAY_IMAGE_MANIPULATION_OPENCV_H
#define YAFARAY_IMAGE_MANIPULATION_OPENCV_H

namespace yafaray {

namespace image_manipulation_opencv
{
	Image *getDenoisedLdrImage(Logger &logger, const Image *image, const DenoiseParams &denoise_params);
	void generateDebugFacesEdges(int xstart, int width, int ystart, int height, bool drawborder, const EdgeToonParams &edge_params);
	void generateDebugFacesEdges(ImageLayers &film_image_layers, int xstart, int width, int ystart, int height, bool drawborder, const EdgeToonParams &edge_params, const ImageBuffer2D<Gray> &weights);
	void generateToonAndDebugObjectEdges(ImageLayers &film_image_layers, int xstart, int width, int ystart, int height, bool drawborder, const EdgeToonParams &edge_params, const ImageBuffer2D<Gray> &weights);
	int generateMipMaps(Logger &logger, std::vector<std::shared_ptr<Image>> &images);
}

} //namespace yafaray

#endif //YAFARAY_IMAGE_MANIPULATION_OPENCV_H
