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
 */

#include "image/image_manipulation.h"
#include "image/image_manipulation_opencv.h"
#include "image/image.h"
#include "image/image_layers.h"
#include "common/mask_edge_toon_params.h"
#include "common/logger.h"
#include <opencv2/photo/photo.hpp>

namespace yafaray {

Image *image_manipulation_opencv::getDenoisedLdrImage(Logger &logger, const Image *image, const DenoiseParams &denoise_params)
{
	if(!denoise_params.enabled_) return nullptr;

	auto image_denoised = Image::factory(logger, image->getWidth(), image->getHeight(), image->getType(), image->getOptimization());
	if(!image_denoised) return image_denoised;

	const int width = image_denoised->getWidth();
	const int height = image_denoised->getHeight();

	cv::Mat a(height, width, CV_8UC3);
	cv::Mat b(height, width, CV_8UC3);
	cv::Mat_<cv::Vec3b> a_vec = a;
	cv::Mat_<cv::Vec3b> b_vec = b;

	for(int y = 0; y < height; ++y)
	{
		for(int x = 0; x < width; ++x)
		{
			Rgb color = image->getColor(x, y);
			color.clampRgb01();

			a_vec(y, x)[0] = (uint8_t) (color.getR() * 255);
			a_vec(y, x)[1] = (uint8_t) (color.getG() * 255);
			a_vec(y, x)[2] = (uint8_t) (color.getB() * 255);
		}
	}

	cv::fastNlMeansDenoisingColored(a, b, denoise_params.hlum_, denoise_params.hcol_, 7, 21);

	for(int y = 0; y < height; ++y)
	{
		for(int x = 0; x < width; ++x)
		{
			Rgba col;
			col.r_ = (denoise_params.mix_ * (float) b_vec(y, x)[0] + (1.f - denoise_params.mix_) * (float) a_vec(y, x)[0]) / 255.0;
			col.g_ = (denoise_params.mix_ * (float) b_vec(y, x)[1] + (1.f - denoise_params.mix_) * (float) a_vec(y, x)[1]) / 255.0;
			col.b_ = (denoise_params.mix_ * (float) b_vec(y, x)[2] + (1.f - denoise_params.mix_) * (float) a_vec(y, x)[2]) / 255.0;
			col.a_ = image->getColor(x, y).a_;
			image_denoised->setColor(x, y, col);
		}
	}
	return image_denoised;
}

void edgeImageDetection(std::vector<cv::Mat> &image_mat, float edge_threshold, int edge_thickness, float smoothness)
{
	//The result of the edges detection will be stored in the first component image of the vector

	//Calculate edges for the different component images and combine them in the first component image
	for(auto it = image_mat.begin(); it != image_mat.end(); it++)
	{
		cv::Laplacian(*it, *it, -1, 3, 1, 0, cv::BORDER_DEFAULT);
		if(it != image_mat.begin()) image_mat.at(0) = cv::max(image_mat.at(0), *it);
	}

	//Get a pure black/white image of the edges
	cv::threshold(image_mat.at(0), image_mat.at(0), edge_threshold, 1.0, 0);

	//Make the edges wider if needed
	if(edge_thickness > 1)
	{
		cv::Mat kernel = cv::Mat::ones(edge_thickness, edge_thickness, CV_32F) / (float)(edge_thickness * edge_thickness);
		cv::filter2D(image_mat.at(0), image_mat.at(0), -1, kernel);
		cv::threshold(image_mat.at(0), image_mat.at(0), 0.1, 1.0, 0);
	}

	//Soften the edges if needed
	if(smoothness > 0.f) cv::GaussianBlur(image_mat.at(0), image_mat.at(0), cv::Size(3, 3), /*sigmaX=*/ smoothness);
}

void image_manipulation_opencv::generateDebugFacesEdges(ImageLayers &film_image_layers, int xstart, int width, int ystart, int height, bool drawborder, const EdgeToonParams &edge_params, const ImageBuffer2D<Gray> &weights)
{
	const Image *normal_image = film_image_layers(LayerDef::NormalGeom).image_.get();
	const Image *z_depth_image = film_image_layers(LayerDef::ZDepthNorm).image_.get();
	Image *debug_faces_edges_image = film_image_layers(LayerDef::DebugFacesEdges).image_.get();

	if(normal_image && z_depth_image)
	{
		std::vector<cv::Mat> image_mat;
		for(int i = 0; i < 4; ++i) image_mat.emplace_back(height, width, CV_32FC1);
		for(int j = ystart; j < height; ++j)
		{
			for(int i = xstart; i < width; ++i)
			{
				const float weight = weights(i, j).getFloat();
				const Rgb col_normal = (*normal_image).getColor(i, j).normalized(weight);
				const float z_depth = (*z_depth_image).getColor(i, j).normalized(weight).a_; //FIXME: can be further optimized

				image_mat.at(0).at<float>(j, i) = col_normal.getR();
				image_mat.at(1).at<float>(j, i) = col_normal.getG();
				image_mat.at(2).at<float>(j, i) = col_normal.getB();
				image_mat.at(3).at<float>(j, i) = z_depth;
			}
		}

		edgeImageDetection(image_mat, edge_params.threshold_, edge_params.thickness_, edge_params.smoothness_);

		for(int j = ystart; j < height; ++j)
		{
			for(int i = xstart; i < width; ++i)
			{
				Rgb col_edge = Rgb(image_mat.at(0).at<float>(j, i));
				if(drawborder && (i <= xstart + 1 || j <= ystart + 1 || i >= width - 1 - 1 || j >= height - 1 - 1)) col_edge = Rgba(0.5f, 0.f, 0.f, 1.f);
				debug_faces_edges_image->setColor(i, j, Rgba{col_edge});
			}
		}
	}
}

void image_manipulation_opencv::generateToonAndDebugObjectEdges(ImageLayers &film_image_layers, int xstart, int width, int ystart, int height, bool drawborder, const EdgeToonParams &edge_params, const ImageBuffer2D<Gray> &weights)
{
	const Image *normal_image = film_image_layers(LayerDef::NormalSmooth).image_.get();
	const Image *z_depth_image = film_image_layers(LayerDef::ZDepthNorm).image_.get();
	Image *debug_objects_edges_image = film_image_layers(LayerDef::DebugObjectsEdges).image_.get();
	Image *toon_image = film_image_layers(LayerDef::Toon).image_.get();

	const float toon_pre_smooth = edge_params.toon_pre_smooth_;
	const float toon_quantization = edge_params.toon_quantization_;
	const float toon_post_smooth = edge_params.toon_post_smooth_;
	const Rgb toon_edge_color = edge_params.toon_color_;
	const int object_edge_thickness = edge_params.face_thickness_;
	const float object_edge_threshold = edge_params.face_threshold_;
	const float object_edge_smoothness = edge_params.face_smoothness_;

	if(normal_image && z_depth_image)
	{
		cv::Mat_<cv::Vec3f> image_mat_combined_vec(height, width, CV_32FC3);
		std::vector<cv::Mat> image_mat;
		for(int i = 0; i < 4; ++i) image_mat.emplace_back(height, width, CV_32FC1);
		Rgb col_normal(0.f);
		float z_depth = 0.f;

		for(int j = ystart; j < height; ++j)
		{
			for(int i = xstart; i < width; ++i)
			{
				const float weight = weights(i, j).getFloat();
				col_normal = (*normal_image).getColor(i, j).normalized(weight);
				z_depth = (*z_depth_image).getColor(i, j).normalized(weight).a_; //FIXME: can be further optimized

				image_mat_combined_vec(j, i)[0] = film_image_layers(LayerDef::Combined).image_->getColor(i, j).normalized(weight).b_;
				image_mat_combined_vec(j, i)[1] = film_image_layers(LayerDef::Combined).image_->getColor(i, j).normalized(weight).g_;
				image_mat_combined_vec(j, i)[2] = film_image_layers(LayerDef::Combined).image_->getColor(i, j).normalized(weight).r_;

				image_mat.at(0).at<float>(j, i) = col_normal.getR();
				image_mat.at(1).at<float>(j, i) = col_normal.getG();
				image_mat.at(2).at<float>(j, i) = col_normal.getB();
				image_mat.at(3).at<float>(j, i) = z_depth;
			}
		}

		cv::GaussianBlur(image_mat_combined_vec, image_mat_combined_vec, cv::Size(3, 3), toon_pre_smooth);

		if(toon_quantization > 0.f)
		{
			for(int j = ystart; j < height; ++j)
			{
				for(int i = xstart; i < width; ++i)
				{
					{
						float h, s, v;
						Rgb col(image_mat_combined_vec(j, i)[0], image_mat_combined_vec(j, i)[1], image_mat_combined_vec(j, i)[2]);
						col.rgbToHsv(h, s, v);
						h = std::round(h / toon_quantization) * toon_quantization;
						s = std::round(s / toon_quantization) * toon_quantization;
						v = std::round(v / toon_quantization) * toon_quantization;
						col.hsvToRgb(h, s, v);
						image_mat_combined_vec(j, i)[0] = col.r_;
						image_mat_combined_vec(j, i)[1] = col.g_;
						image_mat_combined_vec(j, i)[2] = col.b_;
					}
				}
			}
			cv::GaussianBlur(image_mat_combined_vec, image_mat_combined_vec, cv::Size(3, 3), toon_post_smooth);
		}

		edgeImageDetection(image_mat, object_edge_threshold, object_edge_thickness, object_edge_smoothness);

		for(int j = ystart; j < height; ++j)
		{
			for(int i = xstart; i < width; ++i)
			{
				const float edge_value = image_mat.at(0).at<float>(j, i);
				Rgb col_edge = Rgb(edge_value);

				if(drawborder && (i <= xstart + 1 || j <= ystart + 1 || i >= width - 1 - 1 || j >= height - 1 - 1)) col_edge = Rgba(0.5f, 0.f, 0.f, 1.f);

				debug_objects_edges_image->setColor(i, j, Rgba{col_edge});

				Rgb col_toon(image_mat_combined_vec(j, i)[2], image_mat_combined_vec(j, i)[1], image_mat_combined_vec(j, i)[0]);
				col_toon.blend(toon_edge_color, edge_value);

				if(drawborder && (i <= xstart + 1 || j <= ystart + 1 || i >= width - 1 - 1 || j >= height - 1 - 1)) col_toon = Rgba(0.5f, 0.f, 0.f, 1.f);

				toon_image->setColor(i, j, Rgba{col_toon});
			}
		}
	}
}

int image_manipulation_opencv::generateMipMaps(Logger &logger, std::vector<std::shared_ptr<Image>> &images)
{
	if(images.empty()) return 0;
	int img_index = 0;
	//bool blur_seamless = true;
	int w = images.at(0)->getWidth();
	int h = images.at(0)->getHeight();

	if(logger.isVerbose()) logger.logVerbose("Format: generating mipmaps for texture of resolution [", w, " x ", h, "]");

	const cv::Mat a(h, w, CV_32FC4);
	cv::Mat_<cv::Vec4f> a_vec = a;

	for(int j = 0; j < h; ++j)
	{
		for(int i = 0; i < w; ++i)
		{
			Rgba color = images[img_index]->getColor(i, j);
			a_vec(j, i)[0] = color.getR();
			a_vec(j, i)[1] = color.getG();
			a_vec(j, i)[2] = color.getB();
			a_vec(j, i)[3] = color.getA();
		}
	}

	//Mipmap generation using the temporary full float buffer to reduce information loss
	while(w > 1 || h > 1)
	{
		const int w_2 = (w + 1) / 2;
		const int h_2 = (h + 1) / 2;
		++img_index;
		images.emplace_back(Image::factory(logger, w_2, h_2, images[img_index - 1]->getType(), images[img_index - 1]->getOptimization()));

		const cv::Mat b(h_2, w_2, CV_32FC4);
		const cv::Mat_<cv::Vec4f> b_vec = b;
		cv::resize(a, b, cv::Size(w_2, h_2), 0, 0, cv::INTER_AREA);

		for(int j = 0; j < h_2; ++j)
		{
			for(int i = 0; i < w_2; ++i)
			{
				Rgba tmp_col(0.f);
				tmp_col.r_ = b_vec(j, i)[0];
				tmp_col.g_ = b_vec(j, i)[1];
				tmp_col.b_ = b_vec(j, i)[2];
				tmp_col.a_ = b_vec(j, i)[3];
				images[img_index]->setColor(i, j, tmp_col);
			}
		}
		w = w_2;
		h = h_2;
		if(logger.isDebug())logger.logDebug("Format: generated mipmap ", img_index, " [", w_2, " x ", h_2, "]");
	}

	if(logger.isVerbose()) logger.logVerbose("Format: mipmap generation done: ", img_index, " mipmaps generated.");
	return img_index;
}


} //namespace yafaray