/****************************************************************************
 *
 *      imagefilm.cc: image data handling class
 *      This is part of the libYafaRay package
 *      See AUTHORS for more information
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

#include "common/imagefilm.h"
#include "yafaray_config.h"
#include "common/logging.h"
#include "common/session.h"
#include "common/environment.h"
#include "output/output.h"
#include "imagehandler/imagehandler.h"
#include "common/scene.h"
#include "common/file.h"
#include "common/param.h"
#include "common/monitor.h"
#include "common/timer.h"
#include "utility/util_math.h"
#include "resource/yafLogoTiny.h"
#include <iomanip>

#if HAVE_FREETYPE
#include "resource/guifont.h"
#include <ft2build.h>
#include "utility/util_string.h"
#include FT_FREETYPE_H
#endif

#ifdef HAVE_OPENCV
#include <opencv2/photo/photo.hpp>
#endif

BEGIN_YAFARAY

#define FILTER_TABLE_SIZE 16
#define MAX_FILTER_SIZE 8

//! Simple alpha blending
#define ALPHA_BLEND(b_bg_col, b_fg_col, b_alpha) (( b_bg_col * (1.f - b_alpha) ) + ( b_fg_col * b_alpha ))

typedef float FilterFunc_t(float dx, float dy);

float box__(float dx, float dy) { return 1.f; }

/*!
Mitchell-Netravali constants
with B = 1/3 and C = 1/3 as suggested by the authors
mnX1 = constants for 1 <= |x| < 2
mna1 = (-B - 6 * C)/6
mnb1 = (6 * B + 30 * C)/6
mnc1 = (-12 * B - 48 * C)/6
mnd1 = (8 * B + 24 * C)/6

mnX2 = constants for 1 > |x|
mna2 = (12 - 9 * B - 6 * C)/6
mnb2 = (-18 + 12 * B + 6 * C)/6
mnc2 = (6 - 2 * B)/6

#define mna1 -0.38888889
#define mnb1  2.0
#define mnc1 -3.33333333
#define mnd1  1.77777778

#define mna2  1.16666666
#define mnb2 -2.0
#define mnc2  0.88888889
*/
#define GAUSS_EXP 0.00247875

float mitchell__(float dx, float dy)
{
	float x = 2.f * fSqrt__(dx * dx + dy * dy);

	if(x >= 2.f) return (0.f);

	if(x >= 1.f) // from mitchell-netravali paper 1 <= |x| < 2
	{
		return (float)(x * (x * (x * -0.38888889f + 2.0f) - 3.33333333f) + 1.77777778f);
	}

	return (float)(x * x * (1.16666666f * x - 2.0f) + 0.88888889f);
}

float gauss__(float dx, float dy)
{
	float r_2 = dx * dx + dy * dy;
	return std::max(0.f, float(fExp__(-6 * r_2) - GAUSS_EXP));
}

//Lanczos sinc window size 2
float lanczos2__(float dx, float dy)
{
	float x = fSqrt__(dx * dx + dy * dy);

	if(x == 0.f) return 1.f;

	if(-2 < x && x < 2)
	{
		float a = M_PI * x;
		float b = M_PI_2 * x;
		return ((fSin__(a) * fSin__(b)) / (a * b));
	}

	return 0.f;
}

ImageFilm::ImageFilm (int width, int height, int xstart, int ystart, ColorOutput &out, float filter_size, FilterType filt,
					  RenderEnvironment *e, bool show_sam_mask, int t_size, ImageSplitter::TilesOrderType tiles_order_type, bool pm_a):
		w_(width), h_(height), cx_0_(xstart), cy_0_(ystart), filterw_(filter_size * 0.5), output_(&out),
		env_(e), show_mask_(show_sam_mask), tile_size_(t_size), tiles_order_(tiles_order_type), premult_alpha_(pm_a)
{
	cx_1_ = xstart + width;
	cy_1_ = ystart + height;
	filter_table_ = new float[FILTER_TABLE_SIZE * FILTER_TABLE_SIZE];

	const RenderPasses *render_passes = env_->getRenderPasses();

	//Creation of the image buffers for the render passes
	for(int idx = 0; idx < render_passes->extPassesSize(); ++idx)
	{
		image_passes_.push_back(new Rgba2DImageWeighed_t(width, height));
	}

	//Creation of the image buffers for the auxiliary render passes
	for(int idx = 0; idx < render_passes->auxPassesSize(); ++idx)
	{
		aux_image_passes_.push_back(new Rgba2DImageWeighed_t(width, height));
	}

	density_image_ = nullptr;
	estimate_density_ = false;
	dp_image_ = nullptr;

	// fill filter table:
	float *f_tp = filter_table_;
	float scale = 1.f / (float)FILTER_TABLE_SIZE;

	FilterFunc_t *ffunc = 0;
	switch(filt)
	{
		case ImageFilm::FilterType::Mitchell: ffunc = mitchell__; filterw_ *= 2.6f; break;
		case ImageFilm::FilterType::Lanczos: ffunc = lanczos2__; break;
		case ImageFilm::FilterType::Gauss: ffunc = gauss__; filterw_ *= 2.f; break;
		case ImageFilm::FilterType::Box:
		default:	ffunc = box__;
	}

	filterw_ = std::min(std::max(0.501f, filterw_), 0.5f * MAX_FILTER_SIZE); // filter needs to cover at least the area of one pixel and no more than MAX_FILTER_SIZE/2

	for(int y = 0; y < FILTER_TABLE_SIZE; ++y)
	{
		for(int x = 0; x < FILTER_TABLE_SIZE; ++x)
		{
			*f_tp = ffunc((x + .5f) * scale, (y + .5f) * scale);
			++f_tp;
		}
	}

	table_scale_ = 0.9999 * FILTER_TABLE_SIZE / filterw_;
	area_cnt_ = 0;

	pbar_ = new ConsoleProgressBar(80);
	session__.setStatusCurrentPassPercent(pbar_->getPercent());

	aa_detect_color_noise_ = false;
	aa_dark_threshold_factor_ = 0.f;
	aa_variance_edge_size_ = 10;
	aa_variance_pixels_ = 0;
	aa_clamp_samples_ = 0.f;
}

ImageFilm::~ImageFilm ()
{

	//Deletion of the image buffers for the additional render passes
	for(size_t idx = 0; idx < image_passes_.size(); ++idx)
	{
		delete(image_passes_[idx]);
	}
	image_passes_.clear();

	for(size_t idx = 0; idx < aux_image_passes_.size(); ++idx)
	{
		delete(aux_image_passes_[idx]);
	}
	aux_image_passes_.clear();

	//Deletion of the auxiliary image buffers
	for(size_t idx = 0; idx < aux_image_passes_.size(); ++idx)
	{
		delete(aux_image_passes_[idx]);
	}
	aux_image_passes_.clear();

	if(density_image_) delete density_image_;
	delete[] filter_table_;
	if(splitter_) delete splitter_;
	if(dp_image_) delete dp_image_;
	if(pbar_) delete pbar_; //remove when pbar no longer created by imageFilm_t!!
}

void ImageFilm::init(int num_passes)
{
	// Clear color buffers
	for(size_t idx = 0; idx < image_passes_.size(); ++idx)
	{
		image_passes_[idx]->clear();
	}

	// Clear density image
	if(estimate_density_)
	{
		if(!density_image_) density_image_ = new Rgb2DImage_t(w_, h_);
		else density_image_->clear();
	}

	// Setup the bucket splitter
	if(split_)
	{
		next_area_ = 0;
		Scene *scene = env_->getScene();
		int n_threads = 1;
		if(scene) n_threads = scene->getNumThreads();
		splitter_ = new ImageSplitter(w_, h_, cx_0_, cy_0_, tile_size_, tiles_order_, n_threads);
		area_cnt_ = splitter_->size();
	}
	else area_cnt_ = 1;

	if(pbar_) pbar_->init(w_ * h_);
	session__.setStatusCurrentPassPercent(pbar_->getPercent());

	abort_ = false;
	completed_cnt_ = 0;
	n_pass_ = 1;
	n_passes_ = num_passes;

	images_auto_save_pass_counter_ = 0;
	film_auto_save_pass_counter_ = 0;
	resetImagesAutoSaveTimer();
	resetFilmAutoSaveTimer();
	g_timer__.addEvent("imagesAutoSaveTimer");
	g_timer__.addEvent("filmAutoSaveTimer");
	g_timer__.start("imagesAutoSaveTimer");
	g_timer__.start("filmAutoSaveTimer");

	if(!output_->isPreview())	// Avoid doing the Film Load & Save operations and updating the film check values when we are just rendering a preview!
	{
		if(film_file_save_load_ == FilmFileSaveLoad::LoadAndSave) imageFilmLoadAllInFolder();	//Load all the existing Film in the images output folder, combining them together. It will load only the Film files with the same "base name" as the output image film (including file name, computer node name and frame) to allow adding samples to animations.
		if(film_file_save_load_ == FilmFileSaveLoad::LoadAndSave || film_file_save_load_ == FilmFileSaveLoad::Save) imageFilmFileBackup(); //If the imageFilm is set to Save, at the start rename the previous film file as a "backup" just in case the user has made a mistake and wants to get the previous film back.
	}
}

int ImageFilm::nextPass(int num_view, bool adaptive_aa, std::string integrator_name, bool skip_next_pass)
{
	splitter_mutex_.lock();
	next_area_ = 0;
	splitter_mutex_.unlock();
	n_pass_++;
	images_auto_save_pass_counter_++;
	film_auto_save_pass_counter_++;

	if(skip_next_pass) return 0;

	std::stringstream pass_string;

	Y_DEBUG << "nPass=" << n_pass_ << " imagesAutoSavePassCounter=" << images_auto_save_pass_counter_ << " filmAutoSavePassCounter=" << film_auto_save_pass_counter_ << YENDL;

	if(session__.renderInProgress() && !output_->isPreview())	//avoid saving images/film if we are just rendering material/world/lights preview windows, etc
	{
		ColorOutput *out_2 = env_->getOutput2();

		if((images_auto_save_interval_type_ == AutoSaveIntervalType::Pass) && (images_auto_save_pass_counter_ >= images_auto_save_interval_passes_))
		{
			if(output_ && output_->isImageOutput()) this->flush(num_view, All, output_);
			else if(out_2 && out_2->isImageOutput()) this->flush(num_view, All, out_2);
			images_auto_save_pass_counter_ = 0;
		}

		if((film_file_save_load_ == FilmFileSaveLoad::LoadAndSave || film_file_save_load_ == FilmFileSaveLoad::Save) && (film_auto_save_interval_type_ == AutoSaveIntervalType::Pass) && (film_auto_save_pass_counter_ >= film_auto_save_interval_passes_))
		{
			if((output_ && output_->isImageOutput()) || (out_2 && out_2->isImageOutput()))
			{
				imageFilmSave();
				film_auto_save_pass_counter_ = 0;
			}
		}
	}

	const RenderPasses *render_passes = env_->getRenderPasses();

	Rgba2DImageWeighed_t *sampling_factor_image_pass = getImagePassFromIntPassType(PassIntDebugSamplingFactor);

	if(flags_) flags_->clear();
	else flags_ = new TiledBitArray2D<3>(w_, h_, true);
	std::vector<Rgba> col_ext_passes(image_passes_.size(), Rgba(0.f));
	int variance_half_edge = aa_variance_edge_size_ / 2;

	float aa_thresh_scaled = aa_thesh_;

	int n_resample = 0;

	if(adaptive_aa && aa_thesh_ > 0.f)
	{
		for(int y = 0; y < h_ - 1; ++y)
		{
			for(int x = 0; x < w_ - 1; ++x)
			{
				flags_->clearBit(x, y);
			}
		}

		for(int y = 0; y < h_ - 1; ++y)
		{
			for(int x = 0; x < w_ - 1; ++x)
			{
				//We will only consider the Combined Pass (pass 0) for the AA additional sampling calculations.

				if((*image_passes_.at(0))(x, y).weight_ <= 0.f) flags_->setBit(x, y);	//If after reloading ImageFiles there are pixels that were not yet rendered at all, make sure they are marked to be rendered in the next AA pass

				float mat_sample_factor = 1.f;
				if(sampling_factor_image_pass)
				{
					mat_sample_factor = (*sampling_factor_image_pass)(x, y).normalized().r_;
					if(!background_resampling_ && mat_sample_factor == 0.f) continue;
				}

				Rgba pix_col = (*image_passes_.at(0))(x, y).normalized();
				float pix_col_bri = pix_col.abscol2Bri();

				if(aa_dark_detection_type_ == DarkDetectionType::Linear && aa_dark_threshold_factor_ > 0.f)
				{
					if(aa_dark_threshold_factor_ > 0.f) aa_thresh_scaled = aa_thesh_ * ((1.f - aa_dark_threshold_factor_) + (pix_col_bri * aa_dark_threshold_factor_));
				}
				else if(aa_dark_detection_type_ == DarkDetectionType::Curve)
				{
					aa_thresh_scaled = darkThresholdCurveInterpolate(pix_col_bri);
				}

				if(pix_col.colorDifference((*image_passes_.at(0))(x + 1, y).normalized(), aa_detect_color_noise_) >= aa_thresh_scaled)
				{
					flags_->setBit(x, y); flags_->setBit(x + 1, y);
				}
				if(pix_col.colorDifference((*image_passes_.at(0))(x, y + 1).normalized(), aa_detect_color_noise_) >= aa_thresh_scaled)
				{
					flags_->setBit(x, y); flags_->setBit(x, y + 1);
				}
				if(pix_col.colorDifference((*image_passes_.at(0))(x + 1, y + 1).normalized(), aa_detect_color_noise_) >= aa_thresh_scaled)
				{
					flags_->setBit(x, y); flags_->setBit(x + 1, y + 1);
				}
				if(x > 0 && pix_col.colorDifference((*image_passes_.at(0))(x - 1, y + 1).normalized(), aa_detect_color_noise_) >= aa_thresh_scaled)
				{
					flags_->setBit(x, y); flags_->setBit(x - 1, y + 1);
				}

				if(aa_variance_pixels_ > 0)
				{
					int variance_x = 0, variance_y = 0;//, pixelcount = 0;

					//float window_accum = 0.f, window_avg = 0.f;

					for(int xd = -variance_half_edge; xd < variance_half_edge - 1 ; ++xd)
					{
						int xi = x + xd;
						if(xi < 0) xi = 0;
						else if(xi >= w_ - 1) xi = w_ - 2;

						Rgba cx_0 = (*image_passes_.at(0))(xi, y).normalized();
						Rgba cx_1 = (*image_passes_.at(0))(xi + 1, y).normalized();

						if(cx_0.colorDifference(cx_1, aa_detect_color_noise_) >= aa_thresh_scaled) ++variance_x;
					}

					for(int yd = -variance_half_edge; yd < variance_half_edge - 1 ; ++yd)
					{
						int yi = y + yd;
						if(yi < 0) yi = 0;
						else if(yi >= h_ - 1) yi = h_ - 2;

						Rgba cy_0 = (*image_passes_.at(0))(x, yi).normalized();
						Rgba cy_1 = (*image_passes_.at(0))(x, yi + 1).normalized();

						if(cy_0.colorDifference(cy_1, aa_detect_color_noise_) >= aa_thresh_scaled) ++variance_y;
					}

					if(variance_x + variance_y >= aa_variance_pixels_)
					{
						for(int xd = -variance_half_edge; xd < variance_half_edge; ++xd)
						{
							for(int yd = -variance_half_edge; yd < variance_half_edge; ++yd)
							{
								int xi = x + xd;
								if(xi < 0) xi = 0;
								else if(xi >= w_) xi = w_ - 1;

								int yi = y + yd;
								if(yi < 0) yi = 0;
								else if(yi >= h_) yi = h_ - 1;

								flags_->setBit(xi, yi);
							}
						}
					}
				}
			}
		}

		for(int y = 0; y < h_; ++y)
		{
			for(int x = 0; x < w_; ++x)
			{
				if(flags_->getBit(x, y))
				{
					++n_resample;

					if(session__.isInteractive() && show_mask_)
					{
						float mat_sample_factor = 1.f;
						if(sampling_factor_image_pass)
						{
							mat_sample_factor = (*sampling_factor_image_pass)(x, y).normalized().r_;
							if(!background_resampling_ && mat_sample_factor == 0.f) continue;
						}

						for(size_t idx = 0; idx < image_passes_.size(); ++idx)
						{
							Rgb pix = (*image_passes_[idx])(x, y).normalized();
							float pix_col_bri = pix.abscol2Bri();

							if(pix.r_ < pix.g_ && pix.r_ < pix.b_)
								col_ext_passes[idx].set(0.7f, pix_col_bri, mat_sample_factor > 1.f ? 0.7f : pix_col_bri);
							else
								col_ext_passes[idx].set(pix_col_bri, 0.7f, mat_sample_factor > 1.f ? 0.7f : pix_col_bri);
						}
						output_->putPixel(num_view, x, y, render_passes, col_ext_passes, false);
					}
				}
			}
		}
	}
	else
	{
		n_resample = h_ * w_;
	}

	if(session__.isInteractive())	output_->flush(num_view, render_passes);

	if(session__.renderResumed()) pass_string << "Film loaded + ";

	pass_string << "Rendering pass " << n_pass_ << " of " << n_passes_ << ", resampling " << n_resample << " pixels.";

	Y_INFO << integrator_name << ": " << pass_string.str() << YENDL;

	if(pbar_)
	{
		pbar_->init(w_ * h_);
		session__.setStatusCurrentPassPercent(pbar_->getPercent());
		pbar_->setTag(pass_string.str().c_str());
	}
	completed_cnt_ = 0;

	return n_resample;
}

bool ImageFilm::nextArea(int num_view, RenderArea &a)
{
	if(abort_) return false;

	int ifilterw = (int) ceil(filterw_);

	if(split_)
	{
		int n;
		splitter_mutex_.lock();
		n = next_area_++;
		splitter_mutex_.unlock();

		if(splitter_->getArea(n, a))
		{
			a.sx_0_ = a.x_ + ifilterw;
			a.sx_1_ = a.x_ + a.w_ - ifilterw;
			a.sy_0_ = a.y_ + ifilterw;
			a.sy_1_ = a.y_ + a.h_ - ifilterw;

			if(session__.isInteractive())
			{
				out_mutex_.lock();
				int end_x = a.x_ + a.w_, end_y = a.y_ + a.h_;
				output_->highlightArea(num_view, a.x_, a.y_, end_x, end_y);
				out_mutex_.unlock();
			}

			return true;
		}
	}
	else
	{
		if(area_cnt_) return false;
		a.x_ = cx_0_;
		a.y_ = cy_0_;
		a.w_ = w_;
		a.h_ = h_;
		a.sx_0_ = a.x_ + ifilterw;
		a.sx_1_ = a.x_ + a.w_ - ifilterw;
		a.sy_0_ = a.y_ + ifilterw;
		a.sy_1_ = a.y_ + a.h_ - ifilterw;
		++area_cnt_;
		return true;
	}
	return false;
}

void ImageFilm::finishArea(int num_view, RenderArea &a)
{
	out_mutex_.lock();

	const RenderPasses *render_passes = env_->getRenderPasses();

	int end_x = a.x_ + a.w_ - cx_0_, end_y = a.y_ + a.h_ - cy_0_;

	std::vector<Rgba> col_ext_passes(image_passes_.size(), Rgba(0.f));

	for(int j = a.y_ - cy_0_; j < end_y; ++j)
	{
		for(int i = a.x_ - cx_0_; i < end_x; ++i)
		{
			for(size_t idx = 0; idx < image_passes_.size(); ++idx)
			{
				if(render_passes->intPassTypeFromExtPassIndex(idx) == PassIntAaSamples)
				{
					col_ext_passes[idx] = (*image_passes_[idx])(i, j).weight_;
				}
				else if(render_passes->intPassTypeFromExtPassIndex(idx) == PassIntObjIndexAbs ||
						render_passes->intPassTypeFromExtPassIndex(idx) == PassIntObjIndexAutoAbs ||
						render_passes->intPassTypeFromExtPassIndex(idx) == PassIntMatIndexAbs ||
						render_passes->intPassTypeFromExtPassIndex(idx) == PassIntMatIndexAutoAbs
				       )
				{
					col_ext_passes[idx] = (*image_passes_[idx])(i, j).normalized();
					col_ext_passes[idx].ceil(); //To correct the antialiasing and ceil the "mixed" values to the upper integer
				}
				else
				{
					col_ext_passes[idx] = (*image_passes_[idx])(i, j).normalized();
				}

				col_ext_passes[idx].clampRgb0();
				col_ext_passes[idx].colorSpaceFromLinearRgb(color_space_, gamma_);//FIXME DAVID: what passes must be corrected and what do not?
				if(premult_alpha_ && idx == 0) col_ext_passes[idx].alphaPremultiply();

				//To make sure we don't have any weird Alpha values outside the range [0.f, +1.f]
				if(col_ext_passes[idx].a_ < 0.f) col_ext_passes[idx].a_ = 0.f;
				else if(col_ext_passes[idx].a_ > 1.f) col_ext_passes[idx].a_ = 1.f;
			}

			if(!output_->putPixel(num_view, i, j, render_passes, col_ext_passes)) abort_ = true;
		}
	}

	for(size_t idx = 1; idx < image_passes_.size(); ++idx)
	{
		if(render_passes->intPassTypeFromExtPassIndex(idx) == PassIntDebugFacesEdges)
		{
			generateDebugFacesEdges(num_view, idx, a.x_ - cx_0_, end_x, a.y_ - cy_0_, end_y, true, output_);
		}

		if(render_passes->intPassTypeFromExtPassIndex(idx) == PassIntDebugObjectsEdges || render_passes->intPassTypeFromExtPassIndex(idx) == PassIntToon)
		{
			generateToonAndDebugObjectEdges(num_view, idx, a.x_ - cx_0_, end_x, a.y_ - cy_0_, end_y, true, output_);
		}
	}

	if(session__.isInteractive()) output_->flushArea(num_view, a.x_, a.y_, end_x + cx_0_, end_y + cy_0_, render_passes);

	if(session__.renderInProgress() && !output_->isPreview())	//avoid saving images/film if we are just rendering material/world/lights preview windows, etc
	{
		g_timer__.stop("imagesAutoSaveTimer");
		images_auto_save_timer_ += g_timer__.getTime("imagesAutoSaveTimer");
		if(images_auto_save_timer_ < 0.f) resetImagesAutoSaveTimer(); //to solve some strange very negative value when using yafaray-xml, race condition somewhere?
		g_timer__.start("imagesAutoSaveTimer");

		g_timer__.stop("filmAutoSaveTimer");
		film_auto_save_timer_ += g_timer__.getTime("filmAutoSaveTimer");
		if(film_auto_save_timer_ < 0.f) resetFilmAutoSaveTimer(); //to solve some strange very negative value when using yafaray-xml, race condition somewhere?
		g_timer__.start("filmAutoSaveTimer");

		ColorOutput *out_2 = env_->getOutput2();

		if((images_auto_save_interval_type_ == AutoSaveIntervalType::Time) && (images_auto_save_timer_ > images_auto_save_interval_seconds_))
		{
			Y_DEBUG << "imagesAutoSaveTimer=" << images_auto_save_timer_ << YENDL;
			if(output_ && output_->isImageOutput()) this->flush(num_view, All, output_);
			else if(out_2 && out_2->isImageOutput()) this->flush(num_view, All, out_2);
			resetImagesAutoSaveTimer();
		}

		if((film_file_save_load_ == FilmFileSaveLoad::LoadAndSave || film_file_save_load_ == FilmFileSaveLoad::Save) && (film_auto_save_interval_type_ == AutoSaveIntervalType::Time) && (film_auto_save_timer_ > film_auto_save_interval_seconds_))
		{
			Y_DEBUG << "filmAutoSaveTimer=" << film_auto_save_timer_ << YENDL;
			if((output_ && output_->isImageOutput()) || (out_2 && out_2->isImageOutput()))
			{
				imageFilmSave();
			}
			resetFilmAutoSaveTimer();
		}
	}

	if(pbar_)
	{
		if(++completed_cnt_ == area_cnt_) pbar_->done();
		else pbar_->update(a.w_ * a.h_);
		session__.setStatusCurrentPassPercent(pbar_->getPercent());
	}

	out_mutex_.unlock();
}

void ImageFilm::flush(int num_view, int flags, ColorOutput *out)
{
	const RenderPasses *render_passes = env_->getRenderPasses();

	if(session__.renderFinished())
	{
		out_mutex_.lock();
		Y_INFO << "imageFilm: Flushing buffer (View number " << num_view << ")..." << YENDL;
	}

	ColorOutput *out_1 = out ? out : output_;
	ColorOutput *out_2 = env_->getOutput2();

	if(out_1->isPreview()) out_2 = nullptr;	//disable secondary file output when rendering a Preview (material preview, etc)

	if(out_1 == out_2) out_1 = nullptr;	//if we are already flushing the secondary output (out2) as main output (out1), then disable out1 to avoid duplicated work

	std::stringstream ss_badge;

	if(!logger__.getLoggingTitle().empty()) ss_badge << logger__.getLoggingTitle() << "\n";
	if(!logger__.getLoggingAuthor().empty() && !logger__.getLoggingContact().empty()) ss_badge << logger__.getLoggingAuthor() << " | " << logger__.getLoggingContact() << "\n";
	else if(!logger__.getLoggingAuthor().empty() && logger__.getLoggingContact().empty()) ss_badge << logger__.getLoggingAuthor() << "\n";
	else if(logger__.getLoggingAuthor().empty() && !logger__.getLoggingContact().empty()) ss_badge << logger__.getLoggingContact() << "\n";
	if(!logger__.getLoggingComments().empty()) ss_badge << logger__.getLoggingComments() << "\n";

	std::string compiler = YAFARAY_BUILD_COMPILER;
	if(!YAFARAY_BUILD_PLATFORM.empty()) compiler = YAFARAY_BUILD_PLATFORM + "-" + YAFARAY_BUILD_COMPILER;

	ss_badge << "\nYafaRay (" << YAFARAY_BUILD_VERSION << ")" << " " << YAFARAY_BUILD_OS << " " << YAFARAY_BUILD_ARCHITECTURE << " (" << compiler << ")";

	ss_badge << std::setprecision(2);
	double times = g_timer__.getTimeNotStopping("rendert");
	if(session__.renderFinished()) times = g_timer__.getTime("rendert");
	int timem, timeh;
	g_timer__.splitTime(times, &times, &timem, &timeh);
	ss_badge << " | " << w_ << "x" << h_;
	if(session__.renderInProgress()) ss_badge << " | " << (session__.renderResumed() ? "film loaded + " : "") << "in progress " << std::fixed << std::setprecision(1) << session__.currentPassPercent() << "% of pass: " << session__.currentPass() << " / " << session__.totalPasses();
	else if(session__.renderAborted()) ss_badge << " | " << (session__.renderResumed() ? "film loaded + " : "") << "stopped at " << std::fixed << std::setprecision(1) << session__.currentPassPercent() << "% of pass: " << session__.currentPass() << " / " << session__.totalPasses();
	else
	{
		if(session__.renderResumed())	ss_badge << " | film loaded + " << session__.totalPasses() - 1 << " passes";
		else ss_badge << " | " << session__.totalPasses() << " passes";
	}
	//if(cx0 != 0) ssBadge << ", xstart=" << cx0;
	//if(cy0 != 0) ssBadge << ", ystart=" << cy0;
	ss_badge << " | Render time:";
	if(timeh > 0) ss_badge << " " << timeh << "h";
	if(timem > 0) ss_badge << " " << timem << "m";
	ss_badge << " " << times << "s";

	times = g_timer__.getTimeNotStopping("rendert") + g_timer__.getTime("prepass");
	if(session__.renderFinished()) times = g_timer__.getTime("rendert") + g_timer__.getTime("prepass");
	g_timer__.splitTime(times, &times, &timem, &timeh);
	ss_badge << " | Total time:";
	if(timeh > 0) ss_badge << " " << timeh << "h";
	if(timem > 0) ss_badge << " " << timem << "m";
	ss_badge << " " << times << "s";

	std::stringstream ss_log;
	ss_log << ss_badge.str();
	logger__.setRenderInfo(ss_badge.str());

	if(logger__.getDrawRenderSettings()) ss_badge << " | " << logger__.getRenderSettings();
	if(logger__.getDrawAaNoiseSettings()) ss_badge << "\n" << logger__.getAaNoiseSettings();
	if(output_ && output_->isImageOutput()) ss_badge << " " << output_->getDenoiseParams();
	else if(out_2 && out_2->isImageOutput()) ss_badge << " " << out_2->getDenoiseParams();

	ss_log << " | " << logger__.getRenderSettings();
	ss_log << "\n" << logger__.getAaNoiseSettings();
	if(output_ && output_->isImageOutput()) ss_log << " " << output_->getDenoiseParams();
	else if(out_2 && out_2->isImageOutput()) ss_log << " " << out_2->getDenoiseParams();

	if(logger__.getUseParamsBadge())
	{
		if((out_1 && out_1->isImageOutput()) || (out_2 && out_2->isImageOutput())) drawRenderSettings(ss_badge);
	}

	if(session__.renderFinished())
	{
		Y_PARAMS << "--------------------------------------------------------------------------------" << YENDL;
		for(std::string line; std::getline(ss_log, line, '\n');) if(line != "" && line != "\n") Y_PARAMS << line << YENDL;
		Y_PARAMS << "--------------------------------------------------------------------------------" << YENDL;
	}

#ifndef HAVE_FREETYPE
	Y_WARNING << "imageFilm: Compiled without FreeType support." << YENDL;
	Y_WARNING << "imageFilm: Text on the parameters badge won't be available." << YENDL;
#endif

	float density_factor = 0.f;

	if(estimate_density_ && num_density_samples_ > 0) density_factor = (float)(w_ * h_) / (float) num_density_samples_;

	std::vector<Rgba> col_ext_passes(image_passes_.size(), Rgba(0.f));

	std::vector<Rgba> col_ext_passes_2;	//For secondary file output (when enabled)
	if(out_2) col_ext_passes_2.resize(image_passes_.size(), Rgba(0.f));

	int output_displace_rendered_image_badge_height = 0, out_2_displace_rendered_image_badge_height = 0;

	if(out_1 && out_1->isImageOutput() && logger__.isParamsBadgeTop()) output_displace_rendered_image_badge_height = logger__.getBadgeHeight();

	if(out_2 && out_2->isImageOutput() && logger__.isParamsBadgeTop()) out_2_displace_rendered_image_badge_height = logger__.getBadgeHeight();

	for(int j = 0; j < h_; j++)
	{
		for(int i = 0; i < w_; i++)
		{
			for(size_t idx = 0; idx < image_passes_.size(); ++idx)
			{
				if(render_passes->intPassTypeFromExtPassIndex(idx) == PassIntAaSamples)
				{
					col_ext_passes[idx] = (*image_passes_[idx])(i, j).weight_;
				}
				else if(render_passes->intPassTypeFromExtPassIndex(idx) == PassIntObjIndexAbs ||
						render_passes->intPassTypeFromExtPassIndex(idx) == PassIntObjIndexAutoAbs ||
						render_passes->intPassTypeFromExtPassIndex(idx) == PassIntMatIndexAbs ||
						render_passes->intPassTypeFromExtPassIndex(idx) == PassIntMatIndexAutoAbs
				       )
				{
					col_ext_passes[idx] = (*image_passes_[idx])(i, j).normalized();
					col_ext_passes[idx].ceil(); //To correct the antialiasing and ceil the "mixed" values to the upper integer
				}
				else
				{
					if(flags & Image) col_ext_passes[idx] = (*image_passes_[idx])(i, j).normalized();
					else col_ext_passes[idx] = Rgba(0.f);
				}

				if(estimate_density_ && (flags & Densityimage) && idx == 0 && density_factor > 0.f) col_ext_passes[idx] += Rgba((*density_image_)(i, j) * density_factor, 0.f);

				col_ext_passes[idx].clampRgb0();

				if(out_2) col_ext_passes_2[idx] = col_ext_passes[idx];

				col_ext_passes[idx].colorSpaceFromLinearRgb(color_space_, gamma_);//FIXME DAVID: what passes must be corrected and what do not?

				if(out_2) col_ext_passes_2[idx].colorSpaceFromLinearRgb(color_space_2_, gamma_2_);

				if(premult_alpha_ && idx == 0)
				{
					col_ext_passes[idx].alphaPremultiply();
				}

				if(out_2 && premult_alpha_2_ && idx == 0)
				{
					col_ext_passes_2[idx].alphaPremultiply();
				}

				//To make sure we don't have any weird Alpha values outside the range [0.f, +1.f]
				if(col_ext_passes[idx].a_ < 0.f) col_ext_passes[idx].a_ = 0.f;
				else if(col_ext_passes[idx].a_ > 1.f) col_ext_passes[idx].a_ = 1.f;

				if(out_2)
				{
					if(col_ext_passes_2[idx].a_ < 0.f) col_ext_passes_2[idx].a_ = 0.f;
					else if(col_ext_passes_2[idx].a_ > 1.f) col_ext_passes_2[idx].a_ = 1.f;
				}
			}

			if(out_1) out_1->putPixel(num_view, i, j + output_displace_rendered_image_badge_height, render_passes, col_ext_passes);
			if(out_2) out_2->putPixel(num_view, i, j + out_2_displace_rendered_image_badge_height, render_passes, col_ext_passes_2);
		}
	}

	for(size_t idx = 1; idx < image_passes_.size(); ++idx)
	{
		if(render_passes->intPassTypeFromExtPassIndex(idx) == PassIntDebugFacesEdges)
		{
			generateDebugFacesEdges(num_view, idx, 0, w_, 0, h_, false, out_1, output_displace_rendered_image_badge_height, out_2, out_2_displace_rendered_image_badge_height);
		}

		if(render_passes->intPassTypeFromExtPassIndex(idx) == PassIntDebugObjectsEdges || render_passes->intPassTypeFromExtPassIndex(idx) == PassIntToon)
		{
			generateToonAndDebugObjectEdges(num_view, idx, 0, w_, 0, h_, false, out_1, output_displace_rendered_image_badge_height, out_2, out_2_displace_rendered_image_badge_height);
		}
	}


	if(logger__.getUseParamsBadge() && dp_image_)
	{
		if(out_1 && out_1->isImageOutput())
		{
			int badge_start_y = h_;

			if(logger__.isParamsBadgeTop()) badge_start_y = 0;

			for(int j = badge_start_y; j < badge_start_y + dp_height_; j++)
			{
				for(int i = 0; i < w_; i++)
				{
					for(size_t idx = 0; idx < image_passes_.size(); ++idx)
					{
						Rgba &dpcol = (*dp_image_)(i, j - badge_start_y);
						col_ext_passes[idx] = Rgba(dpcol, 1.f);
					}
					out_1->putPixel(num_view, i, j, render_passes, col_ext_passes);
				}
			}
		}

		if(out_2 && out_2->isImageOutput())
		{
			int badge_start_y = h_;

			if(logger__.isParamsBadgeTop()) badge_start_y = 0;

			for(int j = badge_start_y; j < badge_start_y + dp_height_; j++)
			{
				for(int i = 0; i < w_; i++)
				{
					for(size_t idx = 0; idx < image_passes_.size(); ++idx)
					{
						Rgba &dpcol = (*dp_image_)(i, j - badge_start_y);
						col_ext_passes_2[idx] = Rgba(dpcol, 1.f);
					}
					out_2->putPixel(num_view, i, j, render_passes, col_ext_passes_2);
				}
			}
		}
	}

	if(out_1 && (session__.renderFinished() || out_1->isImageOutput()))
	{
		std::stringstream pass_string;
		if(out_1->isImageOutput()) pass_string << "Saving image files";
		else pass_string << "Flushing output";

		Y_INFO << pass_string.str() << YENDL;

		std::string old_tag;

		if(pbar_)
		{
			old_tag = pbar_->getTag();
			pbar_->setTag(pass_string.str().c_str());
		}

		out_1->flush(num_view, render_passes);

		if(pbar_) pbar_->setTag(old_tag);
	}

	if(out_2 && out_2->isImageOutput())
	{
		std::stringstream pass_string;
		pass_string << "Saving image files";

		Y_INFO << pass_string.str() << YENDL;

		std::string old_tag;

		if(pbar_)
		{
			old_tag = pbar_->getTag();
			pbar_->setTag(pass_string.str().c_str());
		}

		out_2->flush(num_view, render_passes);

		if(pbar_) pbar_->setTag(old_tag);
	}

	if(session__.renderFinished())
	{
		if(!output_->isPreview() && (film_file_save_load_ == FilmFileSaveLoad::LoadAndSave || film_file_save_load_ == FilmFileSaveLoad::Save))
		{
			if((output_ && output_->isImageOutput()) || (out_2 && out_2->isImageOutput()))
			{
				imageFilmSave();
			}
		}

		g_timer__.stop("imagesAutoSaveTimer");
		g_timer__.stop("filmAutoSaveTimer");

		logger__.clearMemoryLog();
		out_mutex_.unlock();
		Y_VERBOSE << "imageFilm: Done." << YENDL;
	}
}

bool ImageFilm::doMoreSamples(int x, int y) const
{
	return (aa_thesh_ > 0.f) ? flags_->getBit(x - cx_0_, y - cy_0_) : true;
}

/* CAUTION! Implemantation of this function needs to be thread safe for samples that
	contribute to pixels outside the area a AND pixels that might get
	contributions from outside area a! (yes, really!) */
void ImageFilm::addSample(ColorPasses &color_passes, int x, int y, float dx, float dy, const RenderArea *a, int num_sample, int aa_pass_number, float inv_aa_max_possible_samples)
{
	const RenderPasses *render_passes = env_->getRenderPasses();

	int dx_0, dx_1, dy_0, dy_1, x_0, x_1, y_0, y_1;

	// get filter extent and make sure we don't leave image area:

	dx_0 = std::max(cx_0_ - x, round2Int__((double) dx - filterw_));
	dx_1 = std::min(cx_1_ - x - 1, round2Int__((double) dx + filterw_ - 1.0));
	dy_0 = std::max(cy_0_ - y, round2Int__((double) dy - filterw_));
	dy_1 = std::min(cy_1_ - y - 1, round2Int__((double) dy + filterw_ - 1.0));

	// get indizes in filter table
	double x_offs = dx - 0.5;

	int x_index[MAX_FILTER_SIZE + 1], y_index[MAX_FILTER_SIZE + 1];

	for(int i = dx_0, n = 0; i <= dx_1; ++i, ++n)
	{
		double d = std::fabs((double(i) - x_offs) * table_scale_);
		x_index[n] = floor2Int__(d);
	}

	double y_offs = dy - 0.5;

	for(int i = dy_0, n = 0; i <= dy_1; ++i, ++n)
	{
		double d = std::fabs((double(i) - y_offs) * table_scale_);
		y_index[n] = floor2Int__(d);
	}

	x_0 = x + dx_0; x_1 = x + dx_1;
	y_0 = y + dy_0; y_1 = y + dy_1;

	image_mutex_.lock();

	for(int j = y_0; j <= y_1; ++j)
	{
		for(int i = x_0; i <= x_1; ++i)
		{
			// get filter value at pixel (x,y)
			int offset = y_index[j - y_0] * FILTER_TABLE_SIZE + x_index[i - x_0];
			float filter_wt = filter_table_[offset];

			// update pixel values with filtered sample contribution
			for(size_t idx = 0; idx < image_passes_.size(); ++idx)
			{
				Rgba col = color_passes(render_passes->intPassTypeFromExtPassIndex(idx));

				col.clampProportionalRgb(aa_clamp_samples_);

				Pixel &pixel = (*image_passes_[idx])(i - cx_0_, j - cy_0_);

				if(premult_alpha_) col.alphaPremultiply();

				if(render_passes->intPassTypeFromExtPassIndex(idx) == PassIntAaSamples)
				{
					pixel.weight_ += inv_aa_max_possible_samples / ((x_1 - x_0 + 1) * (y_1 - y_0 + 1));
				}
				else
				{
					pixel.col_ += (col * filter_wt);
					pixel.weight_ += filter_wt;
				}
			}
			for(size_t idx = 0; idx < aux_image_passes_.size(); ++idx)
			{
				Rgba col = color_passes(render_passes->intPassTypeFromAuxPassIndex(idx));

				col.clampProportionalRgb(aa_clamp_samples_);

				Pixel &pixel = (*aux_image_passes_[idx])(i - cx_0_, j - cy_0_);

				if(premult_alpha_) col.alphaPremultiply();

				if(render_passes->intPassTypeFromAuxPassIndex(idx) == PassIntAaSamples)
				{
					pixel.weight_ += inv_aa_max_possible_samples / ((x_1 - x_0 + 1) * (y_1 - y_0 + 1));
				}
				else
				{
					pixel.col_ += (col * filter_wt);
					pixel.weight_ += filter_wt;
				}
			}
		}
	}

	image_mutex_.unlock();
}

void ImageFilm::addDensitySample(const Rgb &c, int x, int y, float dx, float dy, const RenderArea *a)
{
	if(!estimate_density_) return;

	int dx_0, dx_1, dy_0, dy_1, x_0, x_1, y_0, y_1;

	// get filter extent and make sure we don't leave image area:

	dx_0 = std::max(cx_0_ - x, round2Int__((double) dx - filterw_));
	dx_1 = std::min(cx_1_ - x - 1, round2Int__((double) dx + filterw_ - 1.0));
	dy_0 = std::max(cy_0_ - y, round2Int__((double) dy - filterw_));
	dy_1 = std::min(cy_1_ - y - 1, round2Int__((double) dy + filterw_ - 1.0));


	int x_index[MAX_FILTER_SIZE + 1], y_index[MAX_FILTER_SIZE + 1];

	double x_offs = dx - 0.5;
	for(int i = dx_0, n = 0; i <= dx_1; ++i, ++n)
	{
		double d = std::fabs((double(i) - x_offs) * table_scale_);
		x_index[n] = floor2Int__(d);
	}

	double y_offs = dy - 0.5;
	for(int i = dy_0, n = 0; i <= dy_1; ++i, ++n)
	{
		float d = fabsf((float)((double(i) - y_offs) * table_scale_));
		y_index[n] = floor2Int__(d);
	}

	x_0 = x + dx_0; x_1 = x + dx_1;
	y_0 = y + dy_0; y_1 = y + dy_1;

	density_image_mutex_.lock();

	for(int j = y_0; j <= y_1; ++j)
	{
		for(int i = x_0; i <= x_1; ++i)
		{
			int offset = y_index[j - y_0] * FILTER_TABLE_SIZE + x_index[i - x_0];

			Rgb &pixel = (*density_image_)(i - cx_0_, j - cy_0_);
			pixel += c * filter_table_[offset];
		}
	}

	++num_density_samples_;

	density_image_mutex_.unlock();
}

void ImageFilm::setDensityEstimation(bool enable)
{
	if(enable)
	{
		if(!density_image_) density_image_ = new Rgb2DImage_t(w_, h_);
		else density_image_->clear();
	}
	else
	{
		if(density_image_) delete density_image_;
	}
	estimate_density_ = enable;
}

void ImageFilm::setColorSpace(ColorSpace color_space, float gamma_val)
{
	color_space_ = color_space;
	gamma_ = gamma_val;
}

void ImageFilm::setColorSpace2(ColorSpace color_space, float gamma_val)
{
	color_space_2_ = color_space;
	gamma_2_ = gamma_val;
}

void ImageFilm::setPremult2(bool premult)
{
	premult_alpha_2_ = premult;
}

void ImageFilm::setProgressBar(ProgressBar *pb)
{
	if(pbar_) delete pbar_;
	pbar_ = pb;
}

void ImageFilm::setAaNoiseParams(bool detect_color_noise, const DarkDetectionType &dark_detection_type, float dark_threshold_factor, int variance_edge_size, int variance_pixels, float clamp_samples)
{
	aa_detect_color_noise_ = detect_color_noise;
	aa_dark_detection_type_ = dark_detection_type;
	aa_dark_threshold_factor_ = dark_threshold_factor;
	aa_variance_edge_size_ = variance_edge_size;
	aa_variance_pixels_ = variance_pixels;
	aa_clamp_samples_ = clamp_samples;
}

#if HAVE_FREETYPE

void drawFontBitmap__(const FT_Bitmap *bitmap, Rgba2DImage_t *badge_image, int x, int y, int w, int h)
{
	int i, j, p, q;
	int x_max = std::min(static_cast<int>(x + bitmap->width), badge_image->getWidth());
	int y_max = std::min(static_cast<int>(y + bitmap->rows), badge_image->getHeight());
	Rgb text_color(1.f);

	for(i = x, p = 0; i < x_max; i++, p++)
	{
		for(j = y, q = 0; j < y_max; j++, q++)
		{
			if(i >= w || j >= h) continue;

			int tmp_buf = bitmap->buffer[q * bitmap->width + p];

			if(tmp_buf > 0)
			{
				Rgba &col = (*badge_image)(std::max(0, i), std::max(0, j));
				float alpha = (float) tmp_buf / 255.0;
				col = Rgba(ALPHA_BLEND((Rgb)col, text_color, alpha), col.getA());
			}
		}
	}
}

#endif

void ImageFilm::drawRenderSettings(std::stringstream &ss)
{
	if(dp_image_)
	{
		delete dp_image_;
		dp_image_ = nullptr;
	}

	dp_height_ = logger__.getBadgeHeight();

	dp_image_ = new Rgba2DImage_t(w_, dp_height_);
#ifdef HAVE_FREETYPE
	FT_Library library;
	FT_Face face;

	FT_GlyphSlot slot;
	FT_Vector pen; // untransformed origin

	std::string text_utf_8 = ss.str();
	std::u32string wtext_utf_32 = utf8ToWutf32__(text_utf_8);

	// set font size at default dpi
	float fontsize = 12.5f * logger__.getLoggingFontSizeFactor();

	// initialize library
	if(FT_Init_FreeType(&library))
	{
		Y_ERROR << "imageFilm: FreeType lib couldn't be initialized!" << YENDL;
		return;
	}

	// create face object
	std::string font_path = logger__.getLoggingFontPath();
	if(font_path.empty())
	{
		if(FT_New_Memory_Face(library, (const FT_Byte *)guifont__, guifont_size__, 0, &face))
		{
			Y_ERROR << "imageFilm: FreeType couldn't load the default font!" << YENDL;
			return;
		}
	}
	else if(FT_New_Face(library, font_path.c_str(), 0, &face))
	{
		Y_WARNING << "imageFilm: FreeType couldn't load the font '" << font_path << "', loading default font." << YENDL;

		if(FT_New_Memory_Face(library, (const FT_Byte *)guifont__, guifont_size__, 0, &face))
		{
			Y_ERROR << "imageFilm: FreeType couldn't load the default font!" << YENDL;
			return;
		}
	}

	FT_Select_Charmap(face, ft_encoding_unicode);

	// set character size
	if(FT_Set_Char_Size(face, (FT_F26Dot6)(fontsize * 64.0), 0, 0, 0))
	{
		Y_ERROR << "imageFilm: FreeType couldn't set the character size!" << YENDL;
		return;
	}

	slot = face->glyph;

	int text_offset_y = -1 * (int) ceil(12 * logger__.getLoggingFontSizeFactor());
	int text_interline_offset = (int) ceil(13 * logger__.getLoggingFontSizeFactor());
#endif
	// offsets
	int text_offset_x = 4;

#ifdef HAVE_FREETYPE
	// The pen position in 26.6 cartesian space coordinates
	pen.x = text_offset_x * 64;
	pen.y = text_offset_y * 64;

	// Draw the text
	for(size_t n = 0; n < wtext_utf_32.size(); n++)
	{
		// Set Coordinates for the carrige return
		if(wtext_utf_32[n] == '\n')
		{
			pen.x = text_offset_x * 64;
			pen.y -= text_interline_offset * 64;
			fontsize = 9.5f * logger__.getLoggingFontSizeFactor();
			if(FT_Set_Char_Size(face, (FT_F26Dot6)(fontsize * 64.0), 0, 0, 0))
			{
				Y_ERROR << "imageFilm: FreeType couldn't set the character size!" << YENDL;
				return;
			}

			continue;
		}

		// Set transformation
		FT_Set_Transform(face, 0, &pen);

		// Load glyph image into the slot (erase previous one)
		if(FT_Load_Char(face, wtext_utf_32[n], FT_LOAD_DEFAULT))
		{
			Y_ERROR << "imageFilm: FreeType Couldn't load the glyph image for: '" << wtext_utf_32[n] << "'!" << YENDL;
			continue;
		}

		// Render the glyph into the slot
		FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);

		// Now, draw to our target surface (convert position)
		drawFontBitmap__(&slot->bitmap, dp_image_, slot->bitmap_left, -slot->bitmap_top, w_, h_);

		// increment pen position
		pen.x += slot->advance.x;
		pen.y += slot->advance.y;
	}

	// Cleanup
	FT_Done_Face(face);
	FT_Done_FreeType(library);
#endif

	// Draw logo image
	ParamMap ih_params;

	ih_params["for_output"] = false;

	bool logo_loaded = false;
	ImageHandler *logo = nullptr;

	if(!logger__.getLoggingCustomIcon().empty())
	{
		std::string icon_extension = logger__.getLoggingCustomIcon().substr(logger__.getLoggingCustomIcon().find_last_of(".") + 1);
		std::transform(icon_extension.begin(), icon_extension.end(), icon_extension.begin(), ::tolower);

		std::string image_handler_type = "png";
		if(icon_extension == "jpeg") image_handler_type = "jpg";
		else image_handler_type = icon_extension;

		ih_params["type"] = image_handler_type;
		logo = env_->createImageHandler("logoLoader", ih_params);

		if(logo && logo->loadFromFile(logger__.getLoggingCustomIcon())) logo_loaded = true;
		else Y_WARNING << "imageFilm: custom params badge icon '" << logger__.getLoggingCustomIcon() << "' could not be loaded. Using default YafaRay icon." << YENDL;
	}

	if(!logo_loaded)
	{
		ih_params["type"] = std::string("png");
		logo = env_->createImageHandler("logoLoader", ih_params);
		if(logo && logo->loadFromMemory(yaf_logo_tiny__, yaf_logo_tiny_size__)) logo_loaded = true;
		else Y_WARNING << "imageFilm: default YafaRay params badge icon could not be loaded. No icon will be shown." << YENDL;
	}

	if(logo_loaded)
	{
		if(logo->getWidth() > 80 || logo->getHeight() > 45) Y_WARNING << "imageFilm: custom params badge logo is quite big (" << logo->getWidth() << " x " << logo->getHeight() << "). It could invade other areas in the badge. Please try to keep logo size smaller than 80 x 45, for example." << YENDL;
		int lx, ly;
		int im_width = std::min(logo->getWidth(), w_);
		int im_height = std::min(logo->getHeight(), dp_height_);

		for(lx = 0; lx < im_width; lx++)
			for(ly = 0; ly < im_height; ly++)
				if(logger__.isParamsBadgeTop())(*dp_image_)(w_ - im_width + lx, ly) = logo->getPixel(lx, ly);
				else(*dp_image_)(w_ - im_width + lx, dp_height_ - im_height + ly) = logo->getPixel(lx, ly);

		delete logo;
	}

	Y_VERBOSE << "imageFilm: Rendering parameters badge created." << YENDL;
}

float ImageFilm::darkThresholdCurveInterpolate(float pixel_brightness)
{
	if(pixel_brightness <= 0.10f) return 0.0001f;
	else if(pixel_brightness > 0.10f && pixel_brightness <= 0.20f) return (0.0001f + (pixel_brightness - 0.10f) * (0.0010f - 0.0001f) / 0.10f);
	else if(pixel_brightness > 0.20f && pixel_brightness <= 0.30f) return (0.0010f + (pixel_brightness - 0.20f) * (0.0020f - 0.0010f) / 0.10f);
	else if(pixel_brightness > 0.30f && pixel_brightness <= 0.40f) return (0.0020f + (pixel_brightness - 0.30f) * (0.0035f - 0.0020f) / 0.10f);
	else if(pixel_brightness > 0.40f && pixel_brightness <= 0.50f) return (0.0035f + (pixel_brightness - 0.40f) * (0.0055f - 0.0035f) / 0.10f);
	else if(pixel_brightness > 0.50f && pixel_brightness <= 0.60f) return (0.0055f + (pixel_brightness - 0.50f) * (0.0075f - 0.0055f) / 0.10f);
	else if(pixel_brightness > 0.60f && pixel_brightness <= 0.70f) return (0.0075f + (pixel_brightness - 0.60f) * (0.0100f - 0.0075f) / 0.10f);
	else if(pixel_brightness > 0.70f && pixel_brightness <= 0.80f) return (0.0100f + (pixel_brightness - 0.70f) * (0.0150f - 0.0100f) / 0.10f);
	else if(pixel_brightness > 0.80f && pixel_brightness <= 0.90f) return (0.0150f + (pixel_brightness - 0.80f) * (0.0250f - 0.0150f) / 0.10f);
	else if(pixel_brightness > 0.90f && pixel_brightness <= 1.00f) return (0.0250f + (pixel_brightness - 0.90f) * (0.0400f - 0.0250f) / 0.10f);
	else if(pixel_brightness > 1.00f && pixel_brightness <= 1.20f) return (0.0400f + (pixel_brightness - 1.00f) * (0.0800f - 0.0400f) / 0.20f);
	else if(pixel_brightness > 1.20f && pixel_brightness <= 1.40f) return (0.0800f + (pixel_brightness - 1.20f) * (0.0950f - 0.0800f) / 0.20f);
	else if(pixel_brightness > 1.40f && pixel_brightness <= 1.80f) return (0.0950f + (pixel_brightness - 1.40f) * (0.1000f - 0.0950f) / 0.40f);
	else return 0.1000f;
}

std::string ImageFilm::getFilmPath() const
{
	std::string film_path = session__.getPathImageOutput();
	std::stringstream node;
	node << std::setfill('0') << std::setw(4) << computer_node_;
	film_path += " - node " + node.str();
	film_path += ".film";
	return film_path;
}

bool ImageFilm::imageFilmLoad(const std::string &filename)
{
	Y_INFO << "imageFilm: Loading film from: \"" << filename << YENDL;

	File file(filename);
	if(!file.open("rb"))
	{
		Y_WARNING << "imageFilm file '" << filename << "' not found, aborting load operation";
		return false;
	}

	std::string header;
	file.read(header);
	if(header != "YAF_FILMv1")
	{
		Y_WARNING << "imageFilm file '" << filename << "' does not contain a valid YafaRay image file";
		file.close();
		return false;
	}
	file.read<unsigned int>(computer_node_);
	file.read<unsigned int>(base_sampling_offset_);
	file.read<unsigned int>(sampling_offset_);

	int filmload_check_w;
	file.read<int>(filmload_check_w);
	if(filmload_check_w != w_)
	{
		Y_WARNING << "imageFilm: loading/reusing film check failed. Image width, expected=" << w_ << ", in reused/loaded film=" << filmload_check_w << YENDL;
		return false;
	}

	int filmload_check_h;
	file.read<int>(filmload_check_h);
	if(filmload_check_h != h_)
	{
		Y_WARNING << "imageFilm: loading/reusing film check failed. Image height, expected=" << h_ << ", in reused/loaded film=" << filmload_check_h << YENDL;
		return false;
	}

	int filmload_check_cx_0;
	file.read<int>(filmload_check_cx_0);
	if(filmload_check_cx_0 != cx_0_)
	{
		Y_WARNING << "imageFilm: loading/reusing film check failed. Border cx0, expected=" << cx_0_ << ", in reused/loaded film=" << filmload_check_cx_0 << YENDL;
		return false;
	}

	int filmload_check_cx_1;
	file.read<int>(filmload_check_cx_1);
	if(filmload_check_cx_1 != cx_1_)
	{
		Y_WARNING << "imageFilm: loading/reusing film check failed. Border cx1, expected=" << cx_1_ << ", in reused/loaded film=" << filmload_check_cx_1 << YENDL;
		return false;
	}

	int filmload_check_cy_0;
	file.read<int>(filmload_check_cy_0);
	if(filmload_check_cy_0 != cy_0_)
	{
		Y_WARNING << "imageFilm: loading/reusing film check failed. Border cy0, expected=" << cy_0_ << ", in reused/loaded film=" << filmload_check_cy_0 << YENDL;
		return false;
	}

	int filmload_check_cy_1;
	file.read<int>(filmload_check_cy_1);
	if(filmload_check_cy_1 != cy_1_)
	{
		Y_WARNING << "imageFilm: loading/reusing film check failed. Border cy1, expected=" << cy_1_ << ", in reused/loaded film=" << filmload_check_cy_1 << YENDL;
		return false;
	}

	int image_passes_size;
	file.read<int>(image_passes_size);
	const RenderPasses *render_passes = env_->getRenderPasses();
	if(image_passes_size != render_passes->extPassesSize())
	{
		Y_WARNING << "imageFilm: loading/reusing film check failed. Number of render passes, expected=" << render_passes->extPassesSize() << ", in reused/loaded film=" << image_passes_size << YENDL;
		return false;
	}
	else image_passes_.resize(image_passes_size);

	int aux_image_passes_size;
	file.read<int>(aux_image_passes_size);
	if(aux_image_passes_size != render_passes->auxPassesSize())
	{
		Y_WARNING << "imageFilm: loading/reusing film check failed. Number of auxiliar render passes, expected=" << render_passes->auxPassesSize() << ", in reused/loaded film=" << aux_image_passes_size << YENDL;
		return false;
	}
	else aux_image_passes_.resize(aux_image_passes_size);

	for(auto &img : image_passes_)
	{
		img = new Rgba2DImageWeighed_t(w_, h_);
		for(int y = 0; y < h_; ++y)
		{
			for(int x = 0; x < w_; ++x)
			{
				Pixel &p = (*img)(x, y);
				file.read<float>(p.col_.r_);
				file.read<float>(p.col_.g_);
				file.read<float>(p.col_.b_);
				file.read<float>(p.col_.a_);
				file.read<float>(p.weight_);
			}
		}
	}

	for(auto &img : aux_image_passes_)
	{
		img = new Rgba2DImageWeighed_t(w_, h_);
		for(int y = 0; y < h_; ++y)
		{
			for(int x = 0; x < w_; ++x)
			{
				Pixel &p = (*img)(x, y);
				file.read<float>(p.col_.r_);
				file.read<float>(p.col_.g_);
				file.read<float>(p.col_.b_);
				file.read<float>(p.col_.a_);
				file.read<float>(p.weight_);
			}
		}
	}
	file.close();
	return true;
}

void ImageFilm::imageFilmLoadAllInFolder()
{
	std::stringstream pass_string;
	pass_string << "Loading ImageFilm files";

	Y_INFO << pass_string.str() << YENDL;

	std::string old_tag;

	if(pbar_)
	{
		old_tag = pbar_->getTag();
		pbar_->setTag(pass_string.str().c_str());
	}

	const Path path_image_output = session__.getPathImageOutput();
	std::string dir = path_image_output.getDirectory();
	if(dir.empty()) dir = ".";	//If parent path is empty, set the path to the current folder
	const std::vector<std::string> files_list = File::listFiles(dir);

	const std::string base_image_file_name = path_image_output.getBaseName();
	std::vector<std::string> film_file_paths_list;
	for(const auto &file_name : files_list)
	{
		//Y_DEBUG PR(fileName) PREND;
		if(File::exists(dir + "//" + file_name, true))
		{
			const Path file_path {file_name };
			const std::string ext = file_path.getExtension();
			const std::string base = file_path.getBaseName();
			const int base_rfind_result = base.rfind(base_image_file_name, 0);

			//Y_DEBUG PR(baseImageFileName) PR(fileName) PR(base) PR(ext) PR(base_rfind_result) PREND;
			if(ext == "film" && base_rfind_result == 0)
			{
				film_file_paths_list.push_back(dir + "//" + file_name);
				//Y_DEBUG << "Added: " << dir + "//" + fileName << YENDL;
			}
		}
	}

	std::sort(film_file_paths_list.begin(), film_file_paths_list.end());
	bool any_film_loaded = false;
	for(const auto &film_file : film_file_paths_list)
	{
		ImageFilm *loaded_film = new ImageFilm(w_, h_, cx_0_, cy_0_, *output_, 1.0, FilterType::Box, env_);
		if(!loaded_film->imageFilmLoad(film_file))
		{
			Y_WARNING << "ImageFilm: Could not load film file '" << film_file << "'" << YENDL;
			continue;
		}
		else any_film_loaded = true;

		for(size_t idx = 0; idx < image_passes_.size(); ++idx)
		{
			Rgba2DImageWeighed_t *loaded_image_buffer = loaded_film->image_passes_[idx];
			for(int i = 0; i < w_; ++i)
			{
				for(int j = 0; j < h_; ++j)
				{
					(*image_passes_[idx])(i, j).col_ += (*loaded_image_buffer)(i, j).col_;
					(*image_passes_[idx])(i, j).weight_ += (*loaded_image_buffer)(i, j).weight_;
				}
			}
		}

		for(size_t idx = 0; idx < aux_image_passes_.size(); ++idx)
		{
			Rgba2DImageWeighed_t *loaded_image_buffer = loaded_film->aux_image_passes_[idx];
			for(int i = 0; i < w_; ++i)
			{
				for(int j = 0; j < h_; ++j)
				{
					(*aux_image_passes_[idx])(i, j).col_ += (*loaded_image_buffer)(i, j).col_;
					(*aux_image_passes_[idx])(i, j).weight_ += (*loaded_image_buffer)(i, j).weight_;
				}
			}
		}

		if(sampling_offset_ < loaded_film->sampling_offset_) sampling_offset_ = loaded_film->sampling_offset_;
		if(base_sampling_offset_ < loaded_film->base_sampling_offset_) base_sampling_offset_ = loaded_film->base_sampling_offset_;

		delete loaded_film;

		Y_VERBOSE << "ImageFilm: loaded film '" << film_file << "'" << YENDL;
	}

	if(any_film_loaded) session__.setStatusRenderResumed();

	if(pbar_) pbar_->setTag(old_tag);
}


bool ImageFilm::imageFilmSave()
{
	bool result_ok = true;
	std::stringstream pass_string;
	pass_string << "Saving internal ImageFilm file";

	Y_INFO << pass_string.str() << YENDL;

	std::string old_tag;

	if(pbar_)
	{
		old_tag = pbar_->getTag();
		pbar_->setTag(pass_string.str().c_str());
	}

	const std::string film_path = getFilmPath();
	File file(film_path);
	file.open("wb");
	file.append(std::string("YAF_FILMv1"));
	file.append<unsigned int>(computer_node_);
	file.append<unsigned int>(base_sampling_offset_);
	file.append<unsigned int>(sampling_offset_);
	file.append<int>(w_);
	file.append<int>(h_);
	file.append<int>(cx_0_);
	file.append<int>(cx_1_);
	file.append<int>(cy_0_);
	file.append<int>(cy_1_);
	file.append<int>((int) image_passes_.size());
	file.append<int>((int) aux_image_passes_.size());

	for(const auto &img : image_passes_)
	{
		const int img_w = img->getWidth();
		if(img_w != w_)
		{
			Y_WARNING << "ImageFilm saving problems, film width " << w_ << " different from internal 2D image width " << img_w << YENDL;
			result_ok = false;
			break;
		}
		const int img_h = img->getHeight();
		if(img_h != h_)
		{
			Y_WARNING << "ImageFilm saving problems, film height " << h_ << " different from internal 2D image height " << img_h << YENDL;
			result_ok = false;
			break;
		}

		for(int y = 0; y < h_; ++y)
		{
			for(int x = 0; x < w_; ++x)
			{
				const Pixel &p = (*img)(x, y);
				file.append<float>(p.col_.r_);
				file.append<float>(p.col_.g_);
				file.append<float>(p.col_.b_);
				file.append<float>(p.col_.a_);
				file.append<float>(p.weight_);
			}
		}
	}

	for(const auto &img : aux_image_passes_)
	{
		const int img_w = img->getWidth();
		if(img_w != w_)
		{
			Y_WARNING << "ImageFilm saving problems, film width " << w_ << " different from internal 2D image width " << img_w << YENDL;
			result_ok = false;
			break;
		}
		const int img_h = img->getHeight();
		if(img_h != h_)
		{
			Y_WARNING << "ImageFilm saving problems, film height " << h_ << " different from internal 2D image height " << img_h << YENDL;
			result_ok = false;
			break;
		}

		for(int y = 0; y < h_; ++y)
		{
			for(int x = 0; x < w_; ++x)
			{
				const Pixel &p = (*img)(x, y);
				file.append<float>(p.col_.r_);
				file.append<float>(p.col_.g_);
				file.append<float>(p.col_.b_);
				file.append<float>(p.col_.a_);
				file.append<float>(p.weight_);
			}
		}
	}

	file.close();
	if(pbar_) pbar_->setTag(old_tag);
	return result_ok;
}

void ImageFilm::imageFilmFileBackup() const
{
	std::stringstream pass_string;
	pass_string << "Creating backup of the previous ImageFilm file...";

	Y_INFO << pass_string.str() << YENDL;

	std::string old_tag;

	if(pbar_)
	{
		old_tag = pbar_->getTag();
		pbar_->setTag(pass_string.str().c_str());
	}

	const std::string film_path = getFilmPath();
	const std::string film_path_backup = film_path + "-previous.bak";

	if(File::exists(film_path, true))
	{
		Y_VERBOSE << "imageFilm: Creating backup of previously saved film to: \"" << film_path_backup << "\"" << YENDL;
		const bool result_ok = File::rename(film_path, film_path_backup, true, true);
		if(!result_ok) Y_WARNING << "imageFilm: error during imageFilm file backup" << YENDL;
	}

	if(pbar_) pbar_->setTag(old_tag);
}


//The next edge detection, debug faces/object edges and toon functions will only work if YafaRay is built with OpenCV support
#ifdef HAVE_OPENCV

void edgeImageDetection__(std::vector<cv::Mat> &image_mat, float edge_threshold, int edge_thickness, float smoothness)
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

void ImageFilm::generateDebugFacesEdges(int num_view, int idx_pass, int xstart, int width, int ystart, int height, bool drawborder, ColorOutput *out_1, int out_1_displacement, ColorOutput *out_2, int out_2_displacement)
{
	const RenderPasses *render_passes = env_->getRenderPasses();
	const int faces_edge_thickness = render_passes->faces_edge_thickness_;
	const float faces_edge_threshold = render_passes->faces_edge_threshold_;
	const float faces_edge_smoothness = render_passes->faces_edge_smoothness_;

	Rgba2DImageWeighed_t *normal_image_pass = getImagePassFromIntPassType(PassIntNormalGeom);
	Rgba2DImageWeighed_t *z_depth_image_pass = getImagePassFromIntPassType(PassIntZDepthNorm);

	if(normal_image_pass && z_depth_image_pass)
	{
		std::vector<cv::Mat> image_mat;
		for(int i = 0; i < 4; ++i) image_mat.push_back(cv::Mat(h_, w_, CV_32FC1));

		for(int j = ystart; j < height; ++j)
		{
			for(int i = xstart; i < width; ++i)
			{
				Rgb col_normal = (*normal_image_pass)(i, j).normalized();
				float z_depth = (*z_depth_image_pass)(i, j).normalized().a_;

				image_mat.at(0).at<float>(j, i) = col_normal.getR();
				image_mat.at(1).at<float>(j, i) = col_normal.getG();
				image_mat.at(2).at<float>(j, i) = col_normal.getB();
				image_mat.at(3).at<float>(j, i) = z_depth;
			}
		}

		edgeImageDetection__(image_mat, faces_edge_threshold, faces_edge_thickness, faces_edge_smoothness);

		for(int j = ystart; j < height; ++j)
		{
			for(int i = xstart; i < width; ++i)
			{
				Rgb col_edge = Rgb(image_mat.at(0).at<float>(j, i));

				if(drawborder && (i <= xstart + 1 || j <= ystart + 1 || i >= width - 1 - 1 || j >= height - 1 - 1)) col_edge = Rgba(0.5f, 0.f, 0.f, 1.f);

				if(out_1) out_1->putPixel(num_view, i, j + out_1_displacement, render_passes, idx_pass, col_edge);
				if(out_2) out_2->putPixel(num_view, i, j + out_2_displacement, render_passes, idx_pass, col_edge);
			}
		}
	}
}

void ImageFilm::generateToonAndDebugObjectEdges(int num_view, int idx_pass, int xstart, int width, int ystart, int height, bool drawborder, ColorOutput *out_1, int out_1_displacement, ColorOutput *out_2, int out_2_displacement)
{
	const RenderPasses *render_passes = env_->getRenderPasses();
	const float toon_pre_smooth = render_passes->toon_pre_smooth_;
	const float toon_quantization = render_passes->toon_quantization_;
	const float toon_post_smooth = render_passes->toon_post_smooth_;
	const Rgb toon_edge_color = Rgb(render_passes->toon_edge_color_[0], render_passes->toon_edge_color_[1], render_passes->toon_edge_color_[2]);
	const int object_edge_thickness = render_passes->object_edge_thickness_;
	const float object_edge_threshold = render_passes->object_edge_threshold_;
	const float object_edge_smoothness = render_passes->object_edge_smoothness_;

	Rgba2DImageWeighed_t *normal_image_pass = getImagePassFromIntPassType(PassIntNormalSmooth);
	Rgba2DImageWeighed_t *z_depth_image_pass = getImagePassFromIntPassType(PassIntZDepthNorm);

	if(normal_image_pass && z_depth_image_pass)
	{
		cv::Mat_<cv::Vec3f> image_mat_combined_vec(h_, w_, CV_32FC3);
		std::vector<cv::Mat> image_mat;
		for(int i = 0; i < 4; ++i) image_mat.push_back(cv::Mat(h_, w_, CV_32FC1));
		Rgb col_normal(0.f);
		float z_depth = 0.f;

		for(int j = ystart; j < height; ++j)
		{
			for(int i = xstart; i < width; ++i)
			{
				col_normal = (*normal_image_pass)(i, j).normalized();
				z_depth = (*z_depth_image_pass)(i, j).normalized().a_;

				image_mat_combined_vec(j, i)[0] = (*image_passes_[0])(i, j).normalized().b_;
				image_mat_combined_vec(j, i)[1] = (*image_passes_[0])(i, j).normalized().g_;
				image_mat_combined_vec(j, i)[2] = (*image_passes_[0])(i, j).normalized().r_;

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

		edgeImageDetection__(image_mat, object_edge_threshold, object_edge_thickness, object_edge_smoothness);

		int idx_toon = getImagePassIndexFromIntPassType(PassIntToon);

		for(int j = ystart; j < height; ++j)
		{
			for(int i = xstart; i < width; ++i)
			{
				float edge_value = image_mat.at(0).at<float>(j, i);
				Rgb col_edge = Rgb(edge_value);

				if(drawborder && (i <= xstart + 1 || j <= ystart + 1 || i >= width - 1 - 1 || j >= height - 1 - 1)) col_edge = Rgba(0.5f, 0.f, 0.f, 1.f);

				if(out_1) out_1->putPixel(num_view, i, j + out_1_displacement, render_passes, idx_pass, col_edge);
				if(out_2) out_2->putPixel(num_view, i, j + out_2_displacement, render_passes, idx_pass, col_edge);

				Rgb col_toon(image_mat_combined_vec(j, i)[2], image_mat_combined_vec(j, i)[1], image_mat_combined_vec(j, i)[0]);
				col_toon.blend(toon_edge_color, edge_value);

				if(drawborder && (i <= xstart + 1 || j <= ystart + 1 || i >= width - 1 - 1 || j >= height - 1 - 1)) col_toon = Rgba(0.5f, 0.f, 0.f, 1.f);

				if(idx_toon != -1)
				{
					if(out_1)
					{
						col_toon.colorSpaceFromLinearRgb(color_space_, gamma_);
						out_1->putPixel(num_view, i, j + out_1_displacement, render_passes, idx_toon, col_toon);
					}
					if(out_2)
					{
						col_toon.colorSpaceFromLinearRgb(color_space_2_, gamma_2_);
						out_2->putPixel(num_view, i, j + out_2_displacement, render_passes, idx_toon, col_toon);
					}
				}
			}
		}
	}
}

#else   //If not built with OpenCV, these functions will do nothing

void ImageFilm::generateToonAndDebugObjectEdges(int num_view, int idx_pass, int xstart, int width, int ystart, int height, bool drawborder, ColorOutput *out_1, int out_1_displacement, ColorOutput *out_2, int out_2_displacement) { }

void ImageFilm::generateDebugFacesEdges(int num_view, int idx_pass, int xstart, int width, int ystart, int height, bool drawborder, ColorOutput *out_1, int out_1_displacement, ColorOutput *out_2, int out_2_displacement) { }

#endif


Rgba2DImageWeighed_t *ImageFilm::getImagePassFromIntPassType(int int_pass_type)
{
	for(size_t idx = 1; idx < image_passes_.size(); ++idx)
	{
		if(env_->getScene()->getRenderPasses()->intPassTypeFromExtPassIndex(idx) == int_pass_type) return image_passes_[idx];
	}

	for(size_t idx = 0; idx < aux_image_passes_.size(); ++idx)
	{
		if(env_->getScene()->getRenderPasses()->intPassTypeFromAuxPassIndex(idx) == int_pass_type) return aux_image_passes_[idx];
	}

	return nullptr;
}

int ImageFilm::getImagePassIndexFromIntPassType(int int_pass_type)
{
	for(size_t idx = 1; idx < image_passes_.size(); ++idx)
	{
		if(env_->getScene()->getRenderPasses()->intPassTypeFromExtPassIndex(idx) == int_pass_type) return (int) idx;
	}

	return -1;
}

int ImageFilm::getAuxImagePassIndexFromIntPassType(int int_pass_type)
{
	for(size_t idx = 0; idx < aux_image_passes_.size(); ++idx)
	{
		if(env_->getScene()->getRenderPasses()->intPassTypeFromAuxPassIndex(idx) == int_pass_type) return (int) idx;
	}

	return -1;
}


END_YAFARAY
