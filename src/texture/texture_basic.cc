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

#include "texture/texture_basic.h"
#include "texture/texture_image.h"
#include "common/param.h"

BEGIN_YAFARAY

NoiseGenerator *newNoise_global(const std::string &ntype)
{
	if(ntype == "blender")
		return new BlenderNoiseGenerator();
	else if(ntype == "stdperlin")
		return new StdPerlinNoiseGenerator();
	else if(int(ntype.find("voronoi")) != -1)
	{
		VoronoiNoiseGenerator::VoronoiType vt = VoronoiNoiseGenerator::Vf1;	// default
		if(ntype == "voronoi_f1")
			vt = VoronoiNoiseGenerator::Vf1;
		else if(ntype == "voronoi_f2")
			vt = VoronoiNoiseGenerator::Vf2;
		else if(ntype == "voronoi_f3")
			vt = VoronoiNoiseGenerator::Vf3;
		else if(ntype == "voronoi_f4")
			vt = VoronoiNoiseGenerator::Vf4;
		else if(ntype == "voronoi_f2f1")
			vt = VoronoiNoiseGenerator::Vf2F1;
		else if(ntype == "voronoi_crackle")
			vt = VoronoiNoiseGenerator::VCrackle;
		return new VoronoiNoiseGenerator(vt);
	}
	else if(ntype == "cellnoise")
		return new CellNoiseGenerator();
	// default
	return new NewPerlinNoiseGenerator();
}

void textureReadColorRamp_global(const ParamMap &params, Texture *tex)
{
	std::string mode_str, interpolation_str, hue_interpolation_str;
	int ramp_num_items = 0;
	params.getParam("ramp_color_mode", mode_str);
	params.getParam("ramp_hue_interpolation", hue_interpolation_str);
	params.getParam("ramp_interpolation", interpolation_str);
	params.getParam("ramp_num_items", ramp_num_items);

	if(ramp_num_items > 0)
	{
		tex->colorRampCreate(mode_str, interpolation_str, hue_interpolation_str);

		for(int i = 0; i < ramp_num_items; ++i)
		{
			std::stringstream param_name;
			Rgba color(0.f, 0.f, 0.f, 1.f);
			float alpha = 1.f;
			float position = 0.f;
			param_name << "ramp_item_" << i << "_color";
			params.getParam(param_name.str(), color);
			param_name.str("");
			param_name.clear();

			param_name << "ramp_item_" << i << "_alpha";
			params.getParam(param_name.str(), alpha);
			param_name.str("");
			param_name.clear();
			color.a_ = alpha;

			param_name << "ramp_item_" << i << "_position";
			params.getParam(param_name.str(), position);
			param_name.str("");
			param_name.clear();
			tex->colorRampAddItem(color, position);
		}
	}
}


//-----------------------------------------------------------------------------------------
// Clouds Texture
//-----------------------------------------------------------------------------------------

CloudsTexture::CloudsTexture(int dep, float sz, bool hd,
							 const Rgb &c_1, const Rgb &c_2,
							 const std::string &ntype, const std::string &btype)
	: depth_(dep), size_(sz), hard_(hd), color_1_(c_1), color_2_(c_2)
{
	bias_ = BiasType::None;
	if(btype == "positive") bias_ = BiasType::Positive;
	else if(btype == "negative") bias_ = BiasType::Negative;
	n_gen_ = newNoise_global(ntype);
}

CloudsTexture::~CloudsTexture()
{
	delete n_gen_;
}

float CloudsTexture::getFloat(const Point3 &p, const MipMapParams *mipmap_params) const
{
	float v = turbulence_global(n_gen_, p, depth_, size_, hard_);
	if(bias_ != BiasType::None)
	{
		v *= v;
		if(bias_ == BiasType::Positive) return -v; //FIXME why?
	}
	return applyIntensityContrastAdjustments(v);
}

Rgba CloudsTexture::getColor(const Point3 &p, const MipMapParams *mipmap_params) const
{
	if(!color_ramp_) return applyColorAdjustments(color_1_ + getFloat(p) * (color_2_ - color_1_));
	else return applyColorAdjustments(color_ramp_->getColorInterpolated(getFloat(p)));
}

Texture *CloudsTexture::factory(ParamMap &params,
								const Scene &scene)
{
	Rgb color_1(0.0), color_2(1.0);
	int depth = 2;
	std::string ntype, btype;
	float size = 1;
	bool hard = false;
	float intensity = 1.f, contrast = 1.f, saturation = 1.f, hue = 0.f, factor_red = 1.f, factor_green = 1.f, factor_blue = 1.f;
	bool clamp = false;
	bool use_color_ramp = false;

	params.getParam("noise_type", ntype);
	params.getParam("color1", color_1);
	params.getParam("color2", color_2);
	params.getParam("depth", depth);
	params.getParam("size", size);
	params.getParam("hard", hard);
	params.getParam("bias", btype);

	params.getParam("adj_mult_factor_red", factor_red);
	params.getParam("adj_mult_factor_green", factor_green);
	params.getParam("adj_mult_factor_blue", factor_blue);
	params.getParam("adj_intensity", intensity);
	params.getParam("adj_contrast", contrast);
	params.getParam("adj_saturation", saturation);
	params.getParam("adj_hue", hue);
	params.getParam("adj_clamp", clamp);

	params.getParam("use_color_ramp", use_color_ramp);

	CloudsTexture *tex = new CloudsTexture(depth, size, hard, color_1, color_2, ntype, btype);
	tex->setAdjustments(intensity, contrast, saturation, hue, clamp, factor_red, factor_green, factor_blue);

	if(use_color_ramp) textureReadColorRamp_global(params, tex);

	return tex;
}

//-----------------------------------------------------------------------------------------
// Simple Marble Texture
//-----------------------------------------------------------------------------------------

MarbleTexture::MarbleTexture(int oct, float sz, const Rgb &c_1, const Rgb &c_2,
							 float turb, float shp, bool hrd, const std::string &ntype, const std::string &shape)
	: octaves_(oct), color_1_(c_1), color_2_(c_2), turb_(turb), size_(sz), hard_(hrd)
{
	sharpness_ = 1.f;
	if(shp > 1) sharpness_ = 1.f / shp;
	n_gen_ = newNoise_global(ntype);
	if(shape == "saw") wshape_ = Shape::Saw;
	else if(shape == "tri") wshape_ = Shape::Tri;
	else wshape_ = Shape::Sin;
}

float MarbleTexture::getFloat(const Point3 &p, const MipMapParams *mipmap_params) const
{
	float w = (p.x_ + p.y_ + p.z_) * 5.f + ((turb_ == 0.f) ? 0.f : turb_ * turbulence_global(n_gen_, p, octaves_, size_, hard_));
	switch(wshape_)
	{
		case Shape::Saw:
			w *= 0.5f * M_1_PI;
			w -= floor(w);
			break;
		case Shape::Tri:
			w *= 0.5f * M_1_PI;
			w = std::abs(2.f * (w - floor(w)) -1.f);
			break;
		default:
		case Shape::Sin:
			w = 0.5f + 0.5f * math::sin(w);
	}
	return applyIntensityContrastAdjustments(math::pow(w, sharpness_));
}

Rgba MarbleTexture::getColor(const Point3 &p, const MipMapParams *mipmap_params) const
{
	if(!color_ramp_) return applyColorAdjustments(color_1_ + getFloat(p) * (color_2_ - color_1_));
	else return applyColorAdjustments(color_ramp_->getColorInterpolated(getFloat(p)));
}

Texture *MarbleTexture::factory(ParamMap &params,
								const Scene &scene)
{
	Rgb col_1(0.0), col_2(1.0);
	int oct = 2;
	float turb = 1.0, shp = 1.0, sz = 1.0;
	bool hrd = false;
	std::string ntype, shape;
	float intensity = 1.f, contrast = 1.f, saturation = 1.f, hue = 0.f, factor_red = 1.f, factor_green = 1.f, factor_blue = 1.f;
	bool clamp = false;
	bool use_color_ramp = false;

	params.getParam("noise_type", ntype);
	params.getParam("color1", col_1);
	params.getParam("color2", col_2);
	params.getParam("depth", oct);
	params.getParam("turbulence", turb);
	params.getParam("sharpness", shp);
	params.getParam("size", sz);
	params.getParam("hard", hrd);
	params.getParam("shape", shape);
	params.getParam("adj_mult_factor_red", factor_red);
	params.getParam("adj_mult_factor_green", factor_green);
	params.getParam("adj_mult_factor_blue", factor_blue);
	params.getParam("adj_intensity", intensity);
	params.getParam("adj_contrast", contrast);
	params.getParam("adj_saturation", saturation);
	params.getParam("adj_hue", hue);
	params.getParam("adj_clamp", clamp);

	params.getParam("use_color_ramp", use_color_ramp);

	MarbleTexture *tex = new MarbleTexture(oct, sz, col_1, col_2, turb, shp, hrd, ntype, shape);
	tex->setAdjustments(intensity, contrast, saturation, hue, clamp, factor_red, factor_green, factor_blue);
	if(use_color_ramp) textureReadColorRamp_global(params, tex);

	return tex;
}


//-----------------------------------------------------------------------------------------
// Simple Wood Texture
//-----------------------------------------------------------------------------------------

WoodTexture::WoodTexture(int oct, float sz, const Rgb &c_1, const Rgb &c_2, float turb,
						 bool hrd, const std::string &ntype, const std::string &wtype, const std::string &shape)
	: octaves_(oct), color_1_(c_1), color_2_(c_2), turb_(turb), size_(sz), hard_(hrd)
{
	rings_ = (wtype == "rings");
	n_gen_ = newNoise_global(ntype);
	if(shape == "saw") wshape_ = Shape::Saw;
	else if(shape == "tri") wshape_ = Shape::Tri;
	else wshape_ = Shape::Sin;
}

float WoodTexture::getFloat(const Point3 &p, const MipMapParams *mipmap_params) const
{
	float w;
	if(rings_)
		w = math::sqrt(p.x_ * p.x_ + p.y_ * p.y_ + p.z_ * p.z_) * 20.f;
	else
		w = (p.x_ + p.y_ + p.z_) * 10.f;
	w += (turb_ == 0.0) ? 0.0 : turb_ * turbulence_global(n_gen_, p, octaves_, size_, hard_);
	switch(wshape_)
	{
		case Shape::Saw:
			w *= 0.5f * M_1_PI;
			w -= floor(w);
			break;
		case Shape::Tri:
			w *= 0.5f * M_1_PI;
			w = std::abs(2.f * (w - floor(w)) - 1.f);
			break;
		default:
		case Shape::Sin:
			w = 0.5f + 0.5f * math::sin(w);
	}
	return applyIntensityContrastAdjustments(w);
}

Rgba WoodTexture::getColor(const Point3 &p, const MipMapParams *mipmap_params) const
{
	if(!color_ramp_) return applyColorAdjustments(color_1_ + getFloat(p) * (color_2_ - color_1_));
	else return applyColorAdjustments(color_ramp_->getColorInterpolated(getFloat(p)));
}

Texture *WoodTexture::factory(ParamMap &params,
							  const Scene &scene)
{
	Rgb col_1(0.0), col_2(1.0);
	int oct = 2;
	float turb = 1.0, sz = 1.0, old_rxy;
	bool hrd = false;
	std::string ntype, wtype, shape;
	float intensity = 1.f, contrast = 1.f, saturation = 1.f, hue = 0.f, factor_red = 1.f, factor_green = 1.f, factor_blue = 1.f;
	bool clamp = false;
	bool use_color_ramp = false;

	params.getParam("noise_type", ntype);
	params.getParam("color1", col_1);
	params.getParam("color2", col_2);
	params.getParam("depth", oct);
	params.getParam("turbulence", turb);
	params.getParam("size", sz);
	params.getParam("hard", hrd);
	params.getParam("wood_type", wtype);
	params.getParam("shape", shape);

	params.getParam("adj_mult_factor_red", factor_red);
	params.getParam("adj_mult_factor_green", factor_green);
	params.getParam("adj_mult_factor_blue", factor_blue);
	params.getParam("adj_intensity", intensity);
	params.getParam("adj_contrast", contrast);
	params.getParam("adj_saturation", saturation);
	params.getParam("adj_hue", hue);
	params.getParam("adj_clamp", clamp);

	params.getParam("use_color_ramp", use_color_ramp);

	if(params.getParam("ringscale_x", old_rxy) || params.getParam("ringscale_y", old_rxy))
		Y_WARNING << "TextureWood: 'ringscale_x' and 'ringscale_y' are obsolete, use 'size' instead" << YENDL;

	WoodTexture *tex = new WoodTexture(oct, sz, col_1, col_2, turb, hrd, ntype, wtype, shape);
	tex->setAdjustments(intensity, contrast, saturation, hue, clamp, factor_red, factor_green, factor_blue);
	if(use_color_ramp) textureReadColorRamp_global(params, tex);

	return tex;
}

//-----------------------------------------------------------------------------------------
/* even simpler RGB cube, goes r in x, g in y and b in z inside the unit cube.  */
//-----------------------------------------------------------------------------------------

Rgba RgbCubeTexture::getColor(const Point3 &p, const MipMapParams *mipmap_params) const
{
	Rgba col = Rgba(p.x_, p.y_, p.z_);
	col.clampRgb01();
	if(adjustments_set_) return applyAdjustments(col);
	else return col;
}

float RgbCubeTexture::getFloat(const Point3 &p, const MipMapParams *mipmap_params) const
{
	Rgb col = Rgb(p.x_, p.y_, p.z_);
	col.clampRgb01();
	return applyIntensityContrastAdjustments(col.energy());
}

Texture *RgbCubeTexture::factory(ParamMap &params, const Scene &scene)
{
	float intensity = 1.f, contrast = 1.f, saturation = 1.f, hue = 0.f, factor_red = 1.f, factor_green = 1.f, factor_blue = 1.f;
	bool clamp = false;
	bool use_color_ramp = false;

	params.getParam("adj_mult_factor_red", factor_red);
	params.getParam("adj_mult_factor_green", factor_green);
	params.getParam("adj_mult_factor_blue", factor_blue);
	params.getParam("adj_intensity", intensity);
	params.getParam("adj_contrast", contrast);
	params.getParam("adj_saturation", saturation);
	params.getParam("adj_hue", hue);
	params.getParam("adj_clamp", clamp);

	params.getParam("use_color_ramp", use_color_ramp);

	RgbCubeTexture *tex = new RgbCubeTexture();
	tex->setAdjustments(intensity, contrast, saturation, hue, clamp, factor_red, factor_green, factor_blue);
	if(use_color_ramp) textureReadColorRamp_global(params, tex);

	return tex;
}

//-----------------------------------------------------------------------------------------
// voronoi block
//-----------------------------------------------------------------------------------------

VoronoiTexture::VoronoiTexture(const Rgb &c_1, const Rgb &c_2,
							   const ColorMode &color_mode,
							   float w_1, float w_2, float w_3, float w_4,
							   float mex, float sz,
							   float isc, const std::string &dname)
	: w_1_(w_1), w_2_(w_2), w_3_(w_3), w_4_(w_4), size_(sz), color_mode_(color_mode)
{
	VoronoiNoiseGenerator::DMetricType dm = VoronoiNoiseGenerator::DistReal;
	if(dname == "squared")
		dm = VoronoiNoiseGenerator::DistSquared;
	else if(dname == "manhattan")
		dm = VoronoiNoiseGenerator::DistManhattan;
	else if(dname == "chebychev")
		dm = VoronoiNoiseGenerator::DistChebychev;
	else if(dname == "minkovsky_half")
		dm = VoronoiNoiseGenerator::DistMinkovskyHalf;
	else if(dname == "minkovsky_four")
		dm = VoronoiNoiseGenerator::DistMinkovskyFour;
	else if(dname == "minkovsky")
		dm = VoronoiNoiseGenerator::DistMinkovsky;
	v_gen_.setDistM(dm);
	v_gen_.setMinkovskyExponent(mex);
	aw_1_ = std::abs(w_1);
	aw_2_ = std::abs(w_2);
	aw_3_ = std::abs(w_3);
	aw_4_ = std::abs(w_4);
	intensity_scale_ = aw_1_ + aw_2_ + aw_3_ + aw_4_;
	if(intensity_scale_ != 0.f) intensity_scale_ = isc / intensity_scale_;
}

float VoronoiTexture::getFloat(const Point3 &p, const MipMapParams *mipmap_params) const
{
	float da[4];
	Point3 pa[4];
	v_gen_.getFeatures(p * size_, da, pa);
	return applyIntensityContrastAdjustments(intensity_scale_ * std::abs(w_1_ * v_gen_.getDistance(0, da) + w_2_ * v_gen_.getDistance(1, da) + w_3_ * v_gen_.getDistance(2, da) + w_4_ * v_gen_.getDistance(3, da)));
}

Rgba VoronoiTexture::getColor(const Point3 &p, const MipMapParams *mipmap_params) const
{
	float da[4];
	Point3 pa[4];
	v_gen_.getFeatures(p * size_, da, pa);
	const float inte = intensity_scale_ * std::abs(w_1_ * v_gen_.getDistance(0, da) + w_2_ * v_gen_.getDistance(1, da) + w_3_ * v_gen_.getDistance(2, da) + w_4_ * v_gen_.getDistance(3, da));
	Rgba col(0.0);
	if(color_ramp_) return applyColorAdjustments(color_ramp_->getColorInterpolated(inte));
	else if(color_mode_ != ColorMode::IntensityWithoutColor)
	{
		col += aw_1_ * cellNoiseColor_global(v_gen_.getPoint(0, pa));
		col += aw_2_ * cellNoiseColor_global(v_gen_.getPoint(1, pa));
		col += aw_3_ * cellNoiseColor_global(v_gen_.getPoint(2, pa));
		col += aw_4_ * cellNoiseColor_global(v_gen_.getPoint(3, pa));
		if(color_mode_ == ColorMode::PositionOutline || color_mode_ == ColorMode::PositionOutlineIntensity)
		{
			float t_1 = (v_gen_.getDistance(1, da) - v_gen_.getDistance(0, da)) * 10.f;
			if(t_1 > 1) t_1 = 1;
			if(color_mode_ == ColorMode::PositionOutlineIntensity) t_1 *= inte;
			else t_1 *= intensity_scale_;
			col *= t_1;
		}
		else col *= intensity_scale_;
		return applyAdjustments(col);
	}
	else return applyColorAdjustments(Rgba(inte, inte, inte, inte));
}

Texture *VoronoiTexture::factory(ParamMap &params, const Scene &scene)
{
	Rgb col_1(0.0), col_2(1.0);
	std::string cltype, dname;
	float fw_1 = 1, fw_2 = 0, fw_3 = 0, fw_4 = 0;
	float mex = 2.5;	// minkovsky exponent
	float isc = 1;	// intensity scale
	float sz = 1;	// size
	float intensity = 1.f, contrast = 1.f, saturation = 1.f, hue = 0.f, factor_red = 1.f, factor_green = 1.f, factor_blue = 1.f;
	bool clamp = false;
	bool use_color_ramp = false;

	params.getParam("color1", col_1);
	params.getParam("color2", col_2);

	params.getParam("color_mode", cltype);
	ColorMode color_mode = ColorMode::IntensityWithoutColor;
	if(cltype == "position") color_mode = ColorMode::Position;
	else if(cltype == "position-outline") color_mode = ColorMode::PositionOutline;
	else if(cltype == "position-outline-intensity") color_mode = ColorMode::PositionOutlineIntensity;

	params.getParam("weight1", fw_1);
	params.getParam("weight2", fw_2);
	params.getParam("weight3", fw_3);
	params.getParam("weight4", fw_4);
	params.getParam("mk_exponent", mex);

	params.getParam("intensity", isc);
	params.getParam("size", sz);

	params.getParam("distance_metric", dname);

	params.getParam("adj_mult_factor_red", factor_red);
	params.getParam("adj_mult_factor_green", factor_green);
	params.getParam("adj_mult_factor_blue", factor_blue);
	params.getParam("adj_intensity", intensity);
	params.getParam("adj_contrast", contrast);
	params.getParam("adj_saturation", saturation);
	params.getParam("adj_hue", hue);
	params.getParam("adj_clamp", clamp);

	params.getParam("use_color_ramp", use_color_ramp);

	VoronoiTexture *tex = new VoronoiTexture(col_1, col_2, color_mode, fw_1, fw_2, fw_3, fw_4, mex, sz, isc, dname);
	tex->setAdjustments(intensity, contrast, saturation, hue, clamp, factor_red, factor_green, factor_blue);
	if(use_color_ramp) textureReadColorRamp_global(params, tex);

	return tex;
}

//-----------------------------------------------------------------------------------------
// Musgrave block
//-----------------------------------------------------------------------------------------

MusgraveTexture::MusgraveTexture(const Rgb &c_1, const Rgb &c_2,
								 float h, float lacu, float octs, float offs, float gain,
								 float size, float iscale,
								 const std::string &ntype, const std::string &mtype)
	: color_1_(c_1), color_2_(c_2), size_(size), iscale_(iscale)
{
	n_gen_ = newNoise_global(ntype);
	if(mtype == "multifractal")
		m_gen_ = new MFractalMusgrave(h, lacu, octs, n_gen_);
	else if(mtype == "heteroterrain")
		m_gen_ = new HeteroTerrainMusgrave(h, lacu, octs, offs, n_gen_);
	else if(mtype == "hybridmf")
		m_gen_ = new HybridMFractalMusgrave(h, lacu, octs, offs, gain, n_gen_);
	else if(mtype == "ridgedmf")
		m_gen_ = new RidgedMFractalMusgrave(h, lacu, octs, offs, gain, n_gen_);
	else	// 'fBm' default
		m_gen_ = new FBmMusgrave(h, lacu, octs, n_gen_);
}

MusgraveTexture::~MusgraveTexture()
{
	delete n_gen_;
	delete m_gen_;
}

float MusgraveTexture::getFloat(const Point3 &p, const MipMapParams *mipmap_params) const
{
	return applyIntensityContrastAdjustments(iscale_ * (*m_gen_)(p * size_));
}

Rgba MusgraveTexture::getColor(const Point3 &p, const MipMapParams *mipmap_params) const
{
	if(!color_ramp_) return applyColorAdjustments(color_1_ + getFloat(p) * (color_2_ - color_1_));
	else return applyColorAdjustments(color_ramp_->getColorInterpolated(getFloat(p)));
}

Texture *MusgraveTexture::factory(ParamMap &params, const Scene &scene)
{
	Rgb col_1(0.0), col_2(1.0);
	std::string ntype, mtype;
	float h = 1, lacu = 2, octs = 2, offs = 1, gain = 1, size = 1, iscale = 1;
	float intensity = 1.f, contrast = 1.f, saturation = 1.f, hue = 0.f, factor_red = 1.f, factor_green = 1.f, factor_blue = 1.f;
	bool clamp = false;
	bool use_color_ramp = false;

	params.getParam("color1", col_1);
	params.getParam("color2", col_2);

	params.getParam("musgrave_type", mtype);
	params.getParam("noise_type", ntype);

	params.getParam("H", h);
	params.getParam("lacunarity", lacu);
	params.getParam("octaves", octs);
	params.getParam("offset", offs);
	params.getParam("gain", gain);
	params.getParam("size", size);
	params.getParam("intensity", iscale);

	params.getParam("adj_mult_factor_red", factor_red);
	params.getParam("adj_mult_factor_green", factor_green);
	params.getParam("adj_mult_factor_blue", factor_blue);
	params.getParam("adj_intensity", intensity);
	params.getParam("adj_contrast", contrast);
	params.getParam("adj_saturation", saturation);
	params.getParam("adj_hue", hue);
	params.getParam("adj_clamp", clamp);

	params.getParam("use_color_ramp", use_color_ramp);

	MusgraveTexture *tex = new MusgraveTexture(col_1, col_2, h, lacu, octs, offs, gain, size, iscale, ntype, mtype);
	tex->setAdjustments(intensity, contrast, saturation, hue, clamp, factor_red, factor_green, factor_blue);
	if(use_color_ramp) textureReadColorRamp_global(params, tex);

	return tex;
}

//-----------------------------------------------------------------------------------------
// Distored Noise block
//-----------------------------------------------------------------------------------------

DistortedNoiseTexture::DistortedNoiseTexture(const Rgb &c_1, const Rgb &c_2,
											 float distort, float size,
											 const std::string &noiseb_1, const std::string noiseb_2)
	: color_1_(c_1), color_2_(c_2), distort_(distort), size_(size)
{
	n_gen_1_ = newNoise_global(noiseb_1);
	n_gen_2_ = newNoise_global(noiseb_2);
}

DistortedNoiseTexture::~DistortedNoiseTexture()
{
	delete n_gen_1_;
	delete n_gen_2_;
}

float DistortedNoiseTexture::getFloat(const Point3 &p, const MipMapParams *mipmap_params) const
{
	// get a random vector and scale the randomization
	const Point3 ofs(13.5, 13.5, 13.5);
	const Point3 tp(p * size_);
	const Point3 rv(getSignedNoise_global(n_gen_1_, tp + ofs), getSignedNoise_global(n_gen_1_, tp), getSignedNoise_global(n_gen_1_, tp - ofs));
	return applyIntensityContrastAdjustments(getSignedNoise_global(n_gen_2_, tp + rv * distort_));	// distorted-domain noise
}

Rgba DistortedNoiseTexture::getColor(const Point3 &p, const MipMapParams *mipmap_params) const
{
	if(!color_ramp_) return applyColorAdjustments(color_1_ + getFloat(p) * (color_2_ - color_1_));
	else return applyColorAdjustments(color_ramp_->getColorInterpolated(getFloat(p)));
}

Texture *DistortedNoiseTexture::factory(ParamMap &params, const Scene &scene)
{
	Rgb col_1(0.0), col_2(1.0);
	std::string ntype_1, ntype_2;
	float dist = 1, size = 1;
	float intensity = 1.f, contrast = 1.f, saturation = 1.f, hue = 0.f, factor_red = 1.f, factor_green = 1.f, factor_blue = 1.f;
	bool clamp = false;
	bool use_color_ramp = false;

	params.getParam("color1", col_1);
	params.getParam("color2", col_2);

	params.getParam("noise_type1", ntype_1);
	params.getParam("noise_type2", ntype_2);

	params.getParam("distort", dist);
	params.getParam("size", size);

	params.getParam("adj_mult_factor_red", factor_red);
	params.getParam("adj_mult_factor_green", factor_green);
	params.getParam("adj_mult_factor_blue", factor_blue);
	params.getParam("adj_intensity", intensity);
	params.getParam("adj_contrast", contrast);
	params.getParam("adj_saturation", saturation);
	params.getParam("adj_hue", hue);
	params.getParam("adj_clamp", clamp);

	params.getParam("use_color_ramp", use_color_ramp);

	DistortedNoiseTexture *tex = new DistortedNoiseTexture(col_1, col_2, dist, size, ntype_1, ntype_2);
	tex->setAdjustments(intensity, contrast, saturation, hue, clamp, factor_red, factor_green, factor_blue);
	if(use_color_ramp) textureReadColorRamp_global(params, tex);

	return tex;
}

//-----------------------------------------------------------------------------------------
// Blend Texture
//-----------------------------------------------------------------------------------------


BlendTexture::BlendTexture(const std::string &stype, bool use_flip_axis)
{
	use_flip_axis_ = use_flip_axis;
	if(stype == "lin") progression_type_ = Linear;
	else if(stype == "quad") progression_type_ = Quadratic;
	else if(stype == "ease") progression_type_ = Easing;
	else if(stype == "diag") progression_type_ = Diagonal;
	else if(stype == "sphere") progression_type_ = Spherical;
	else if(stype == "halo" || stype == "quad_sphere") progression_type_ = QuadraticSphere;
	else if(stype == "radial") progression_type_ = Radial;
	else progression_type_ = Linear;
}

float BlendTexture::getFloat(const Point3 &p, const MipMapParams *mipmap_params) const
{
	float blend = 0.f;
	float coord_1 = p.x_;
	float coord_2 = p.y_;

	if(use_flip_axis_)
	{
		coord_1 = p.y_;
		coord_2 = p.x_;
	}

	if(progression_type_ == Quadratic)
	{
		// Transform -1..1 to 0..1
		blend = 0.5f * (coord_1 + 1.f);
		if(blend < 0.f) blend = 0.f;
		else blend *= blend;
	}
	else if(progression_type_ == Easing)
	{
		blend = 0.5f * (coord_1 + 1.f);
		if(blend <= 0.f) blend = 0.f;
		else if(blend >= 1.f) blend = 1.f;
		else
		{
			blend = (3.f * blend * blend - 2.f * blend * blend * blend);
		}
	}
	else if(progression_type_ == Diagonal)
	{
		blend = 0.25f * (2.f + coord_1 + coord_2);
	}
	else if(progression_type_ == Spherical || progression_type_ == QuadraticSphere)
	{
		blend = 1.f - math::sqrt(coord_1 * coord_1 + coord_2 * coord_2 + p.z_ * p.z_);
		if(blend < 0.f) blend = 0.f;
		if(progression_type_ == QuadraticSphere) blend *= blend;
	}
	else if(progression_type_ == Radial)
	{
		blend = (atan2f(coord_2, coord_1) / (float)(2.f * M_PI) + 0.5f);
	}
	else  //linear by default
	{
		// Transform -1..1 to 0..1
		blend = 0.5f * (coord_1 + 1.f);
	}
	// Clipping to 0..1
	blend = std::max(0.f, std::min(blend, 1.f));
	return applyIntensityContrastAdjustments(blend);
}

Rgba BlendTexture::getColor(const Point3 &p, const MipMapParams *mipmap_params) const
{
	if(!color_ramp_) return applyColorAdjustments(Rgb(getFloat(p)));
	else return applyColorAdjustments(color_ramp_->getColorInterpolated(getFloat(p)));
}

Texture *BlendTexture::factory(ParamMap &params, const Scene &scene)
{
	std::string stype;
	float intensity = 1.f, contrast = 1.f, saturation = 1.f, hue = 0.f, factor_red = 1.f, factor_green = 1.f, factor_blue = 1.f;
	bool clamp = false;
	bool use_color_ramp = false;
	bool use_flip_axis = false;

	params.getParam("stype", stype);
	params.getParam("use_color_ramp", use_color_ramp);
	params.getParam("use_flip_axis", use_flip_axis);

	BlendTexture *tex = new BlendTexture(stype, use_flip_axis);
	tex->setAdjustments(intensity, contrast, saturation, hue, clamp, factor_red, factor_green, factor_blue);
	if(use_color_ramp) textureReadColorRamp_global(params, tex);

	return tex;
}

END_YAFARAY
