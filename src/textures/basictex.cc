
#include <textures/basictex.h>
#include <textures/imagetex.h>

__BEGIN_YAFRAY

noiseGenerator_t* newNoise(const std::string &ntype)
{
	if (ntype=="blender")
		return new blenderNoise_t();
	else if (ntype=="stdperlin")
		return new stdPerlin_t();
	else if (int(ntype.find("voronoi"))!=-1) {
		voronoi_t::voronoiType vt = voronoi_t::V_F1;	// default
		if (ntype=="voronoi_f1")
			vt = voronoi_t::V_F1;
		else if (ntype=="voronoi_f2")
			vt = voronoi_t::V_F2;
		else if (ntype=="voronoi_f3")
			vt = voronoi_t::V_F3;
		else if (ntype=="voronoi_f4")
			vt = voronoi_t::V_F4;
		else if (ntype=="voronoi_f2f1")
			vt = voronoi_t::V_F2F1;
		else if (ntype=="voronoi_crackle")
			vt = voronoi_t::V_CRACKLE;
		return new voronoi_t(vt);
	}
	else if (ntype=="cellnoise")
		return new cellNoise_t();
	// default
	return new newPerlin_t();
}

void textureReadColorRamp(const paraMap_t &params, texture_t * tex)
{
	std::string modeStr, interpolationStr, hue_interpolationStr;
	int ramp_num_items = 0;
	params.getParam("ramp_color_mode", modeStr);
	params.getParam("ramp_hue_interpolation", hue_interpolationStr);
	params.getParam("ramp_interpolation", interpolationStr);
	params.getParam("ramp_num_items", ramp_num_items);
	
	if(ramp_num_items > 0)
	{
		tex->colorRampCreate(modeStr, interpolationStr, hue_interpolationStr);
		
		for(int i = 0; i < ramp_num_items; ++i)
		{
			std::stringstream paramName;
			colorA_t color(0.f, 0.f, 0.f, 1.f);
			float alpha = 1.f;
			float position = 0.f;
			paramName << "ramp_item_" << i << "_color";
			params.getParam(paramName.str(), color);
			paramName.str("");
			paramName.clear();
			
			paramName << "ramp_item_" << i << "_alpha";
			params.getParam(paramName.str(), alpha);
			paramName.str("");
			paramName.clear();
			color.A = alpha;
			
			paramName << "ramp_item_" << i << "_position";
			params.getParam(paramName.str(), position);
			paramName.str("");
			paramName.clear();
			tex->colorRampAddItem(color, position);
		}
	}
}


//-----------------------------------------------------------------------------------------
// Clouds Texture
//-----------------------------------------------------------------------------------------

textureClouds_t::textureClouds_t(int dep, float sz, bool hd,
		const color_t &c1, const color_t &c2,
		const std::string &ntype, const std::string &btype)
		:depth(dep), size(sz), hard(hd), color1(c1), color2(c2)
{
	bias = 0;	// default, no bias
	if (btype=="positive") bias=1;
	else if (btype=="negative") bias=2;
	nGen = newNoise(ntype);
}

textureClouds_t::~textureClouds_t()
{
	if (nGen) delete nGen;
	nGen = nullptr;
}

float textureClouds_t::getFloat(const point3d_t &p) const
{
	float v = turbulence(nGen, p, depth, size, hard);
	if (bias) {
		v *= v;
		if (bias==1) return -v;	// !!!
	}
	return applyIntensityContrastAdjustments(v);
}

colorA_t textureClouds_t::getColor(const point3d_t &p, bool from_postprocessed) const
{
	if(!color_ramp) return applyColorAdjustments(color1 + getFloat(p)*(color2 - color1));
	else return applyColorAdjustments(color_ramp->get_color_interpolated(getFloat(p)));
}

texture_t *textureClouds_t::factory(paraMap_t &params,
		renderEnvironment_t &render)
{
	color_t color1(0.0), color2(1.0);
	int depth = 2;
	std::string _ntype, _btype;
	const std::string *ntype = &_ntype, *btype=&_btype;
	float size = 1;
	bool hard = false;
	float intensity = 1.f, contrast = 1.f, saturation = 1.f, hue = 0.f, factor_red = 1.f, factor_green = 1.f, factor_blue = 1.f;
	bool clamp = false;
	bool use_color_ramp = false;
	
	params.getParam("noise_type", ntype);
	params.getParam("color1", color1);
	params.getParam("color2", color2);
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
	
	textureClouds_t * tex = new textureClouds_t(depth, size, hard, color1, color2, *ntype, *btype);
	tex->setAdjustments(intensity, contrast, saturation, hue, clamp, factor_red, factor_green, factor_blue);
	
	if(use_color_ramp) textureReadColorRamp(params, tex);
	
	return tex;
}

//-----------------------------------------------------------------------------------------
// Simple Marble Texture
//-----------------------------------------------------------------------------------------

textureMarble_t::textureMarble_t(int oct, float sz, const color_t &c1, const color_t &c2,
			float _turb, float shp, bool hrd, const std::string &ntype, const std::string &shape)
	:octaves(oct), color1(c1), color2(c2), turb(_turb), size(sz), hard(hrd)
{
	sharpness = 1.0;
	if (shp>1) sharpness = 1.0/shp;
	nGen = newNoise(ntype);
	wshape = SIN;
	if (shape=="saw") wshape = SAW;
	else if (shape=="tri") wshape = TRI;
}

float textureMarble_t::getFloat(const point3d_t &p) const
{
	float w = (p.x + p.y + p.z)*5.0
					+ ((turb==0.0) ? 0.0 : turb*turbulence(nGen, p, octaves, size, hard));
	switch (wshape) {
		case SAW:
			w *= (float)(0.5*M_1_PI);
			w -= floor(w);
			break;
		case TRI:
			w *= (float)(0.5*M_1_PI);
			w = std::fabs((float)2.0*(w-floor(w))-(float)1.0);
			break;
		default:
		case SIN:
			w = (float)0.5 + (float)0.5*fSin(w);
	}
	return applyIntensityContrastAdjustments(fPow(w, sharpness));
}

colorA_t textureMarble_t::getColor(const point3d_t &p, bool from_postprocessed) const
{
	if(!color_ramp) return applyColorAdjustments(color1 + getFloat(p)*(color2 - color1));
	else return applyColorAdjustments(color_ramp->get_color_interpolated(getFloat(p)));
}

texture_t *textureMarble_t::factory(paraMap_t &params,
		renderEnvironment_t &render)
{
	color_t col1(0.0), col2(1.0);
	int oct = 2;
	float turb=1.0, shp=1.0, sz=1.0;
	bool hrd = false;
	std::string _ntype, _shape;
	const std::string *ntype=&_ntype, *shape=&_shape;
	float intensity = 1.f, contrast = 1.f, saturation = 1.f, hue = 0.f, factor_red = 1.f, factor_green = 1.f, factor_blue = 1.f;
	bool clamp = false;
	bool use_color_ramp = false;
	
	params.getParam("noise_type", ntype);
	params.getParam("color1", col1);
	params.getParam("color2", col2);
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
	
	textureMarble_t * tex = new textureMarble_t(oct, sz, col1, col2, turb, shp, hrd, *ntype, *shape);
	tex->setAdjustments(intensity, contrast, saturation, hue, clamp, factor_red, factor_green, factor_blue);
	if(use_color_ramp) textureReadColorRamp(params, tex);
	
	return tex;
}


//-----------------------------------------------------------------------------------------
// Simple Wood Texture
//-----------------------------------------------------------------------------------------

textureWood_t::textureWood_t(int oct, float sz, const color_t &c1, const color_t &c2, float _turb,
		bool hrd, const std::string &ntype, const std::string &wtype, const std::string &shape)
	:octaves(oct), color1(c1), color2(c2), turb(_turb), size(sz), hard(hrd)
{
	rings = (wtype=="rings");
	nGen = newNoise(ntype);
	wshape = SIN;
	if (shape=="saw") wshape = SAW;
	else if (shape=="tri") wshape = TRI;
}

float textureWood_t::getFloat(const point3d_t &p) const
{
	float w;
	if (rings)
		w = fSqrt(p.x*p.x + p.y*p.y + p.z*p.z)*20.0;
	else
		w = (p.x + p.y + p.z)*10.0;
	w += (turb==0.0) ? 0.0 : turb*turbulence(nGen, p, octaves, size, hard);
	switch (wshape) {
		case SAW:
			w *= (float)(0.5*M_1_PI);
			w -= floor(w);
			break;
		case TRI:
			w *= (float)(0.5*M_1_PI);
			w = std::fabs((float)2.0*(w-floor(w))-(float)1.0);
			break;
		default:
		case SIN:
			w = (float)0.5 + (float)0.5*fSin(w);
	}
	return applyIntensityContrastAdjustments(w);
}

colorA_t textureWood_t::getColor(const point3d_t &p, bool from_postprocessed) const
{
	if(!color_ramp) return applyColorAdjustments(color1 + getFloat(p)*(color2 - color1));
	else return applyColorAdjustments(color_ramp->get_color_interpolated(getFloat(p)));
}

texture_t *textureWood_t::factory(paraMap_t &params,
		renderEnvironment_t &render)
{
	color_t col1(0.0), col2(1.0);
	int oct = 2;
	float turb=1.0, sz=1.0, old_rxy;
	bool hrd = false;
	std::string _ntype, _wtype, _shape;
	const std::string *ntype=&_ntype, *wtype=&_wtype, *shape=&_shape;
	float intensity = 1.f, contrast = 1.f, saturation = 1.f, hue = 0.f, factor_red = 1.f, factor_green = 1.f, factor_blue = 1.f;
	bool clamp = false;
	bool use_color_ramp = false;
	
	params.getParam("noise_type", ntype);
	params.getParam("color1", col1);
	params.getParam("color2", col2);
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
	
	if (params.getParam("ringscale_x", old_rxy) || params.getParam("ringscale_y", old_rxy))
		Y_WARNING << "TextureWood: 'ringscale_x' and 'ringscale_y' are obsolete, use 'size' instead" << yendl;
		
	textureWood_t * tex = new textureWood_t(oct, sz, col1, col2, turb, hrd, *ntype, *wtype, *shape);
	tex->setAdjustments(intensity, contrast, saturation, hue, clamp, factor_red, factor_green, factor_blue);
	if(use_color_ramp) textureReadColorRamp(params, tex);
	
	return tex;
}

//-----------------------------------------------------------------------------------------
/* even simpler RGB cube, goes r in x, g in y and b in z inside the unit cube.  */
//-----------------------------------------------------------------------------------------

colorA_t rgbCube_t::getColor(const point3d_t &p, bool from_postprocessed) const
{
	colorA_t col = colorA_t(p.x, p.y, p.z);
	col.clampRGB01();
	
	if(adjustments_set) return applyAdjustments(col);
	else return col;
}
	
float rgbCube_t::getFloat(const point3d_t &p) const
{
	color_t col = color_t(p.x, p.y, p.z);
	col.clampRGB01();
	return applyIntensityContrastAdjustments(col.energy());
}

texture_t* rgbCube_t::factory(paraMap_t &params,renderEnvironment_t &render)
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

	rgbCube_t * tex = new rgbCube_t();
	tex->setAdjustments(intensity, contrast, saturation, hue, clamp, factor_red, factor_green, factor_blue);
	if(use_color_ramp) textureReadColorRamp(params, tex);
	
	return tex;
}

//-----------------------------------------------------------------------------------------
// voronoi block
//-----------------------------------------------------------------------------------------

textureVoronoi_t::textureVoronoi_t(const color_t &c1, const color_t &c2,
		int ct,
		float _w1, float _w2, float _w3, float _w4,
		float mex, float sz,
		float isc, const std::string &dname)
		:w1(_w1), w2(_w2), w3(_w3), w4(_w4), size(sz), coltype(ct)
{
	voronoi_t::dMetricType dm = voronoi_t::DIST_REAL;
	if (dname=="squared")
		dm = voronoi_t::DIST_SQUARED;
	else if (dname=="manhattan")
		dm = voronoi_t::DIST_MANHATTAN;
	else if (dname=="chebychev")
		dm = voronoi_t::DIST_CHEBYCHEV;
	else if (dname=="minkovsky_half")
		dm = voronoi_t::DIST_MINKOVSKY_HALF;
	else if (dname=="minkovsky_four")
		dm = voronoi_t::DIST_MINKOVSKY_FOUR;
	else if (dname=="minkovsky")
		dm = voronoi_t::DIST_MINKOVSKY;
	vGen.setDistM(dm);
	vGen.setMinkovskyExponent(mex);
	aw1 = std::fabs(_w1);
	aw2 = std::fabs(_w2);
	aw3 = std::fabs(_w3);
	aw4 = std::fabs(_w4);
	iscale = aw1 + aw2 + aw3 + aw4;
	if (iscale!=0) iscale = isc/iscale;
}

float textureVoronoi_t::getFloat(const point3d_t &p) const
{
	float da[4];
	point3d_t pa[4];
	vGen.getFeatures(p*size, da, pa);
	return applyIntensityContrastAdjustments(iscale * std::fabs(w1*vGen.getDistance(0, da) + w2*vGen.getDistance(1, da)
			+ w3*vGen.getDistance(2, da) + w4*vGen.getDistance(3, da)));
}

colorA_t textureVoronoi_t::getColor(const point3d_t &p, bool from_postprocessed) const
{
	float da[4];
	point3d_t pa[4];
	vGen.getFeatures(p*size, da, pa);
	float inte = iscale * std::fabs(w1*vGen.getDistance(0, da) + w2*vGen.getDistance(1, da)
			+ w3*vGen.getDistance(2, da) + w4*vGen.getDistance(3, da));
	colorA_t col(0.0);
	if(color_ramp) return applyColorAdjustments(color_ramp->get_color_interpolated(inte));
	else if (coltype) {
		col += aw1 * cellNoiseColor(vGen.getPoint(0, pa));
		col += aw2 * cellNoiseColor(vGen.getPoint(1, pa));
		col += aw3 * cellNoiseColor(vGen.getPoint(2, pa));
		col += aw4 * cellNoiseColor(vGen.getPoint(3, pa));
		if (coltype>=2) {
			float t1 = (vGen.getDistance(1, da) - vGen.getDistance(0, da))*10.0;
			if (t1>1) t1=1;
			if (coltype==3) t1*=inte; else t1*=iscale;
			col *= t1;
		}
		else col *= iscale;
		return applyAdjustments(col);
	}
	else return applyColorAdjustments(colorA_t(inte, inte, inte, inte));
}

texture_t *textureVoronoi_t::factory(paraMap_t &params, renderEnvironment_t &render)
{
	color_t col1(0.0), col2(1.0);
	std::string _cltype, _dname;
	const std::string *cltype=&_cltype, *dname=&_dname;
	float fw1=1, fw2=0, fw3=0, fw4=0;
	float mex=2.5;	// minkovsky exponent
	float isc=1;	// intensity scale
	float sz=1;	// size
	int ct=0;	// default "int" color type (intensity)
	float intensity = 1.f, contrast = 1.f, saturation = 1.f, hue = 0.f, factor_red = 1.f, factor_green = 1.f, factor_blue = 1.f;
	bool clamp = false;
	bool use_color_ramp = false;
	
	params.getParam("color1", col1);
	params.getParam("color2", col2);
	
	params.getParam("color_type", cltype);
	if (*cltype=="col1") ct=1;
	else if (*cltype=="col2") ct=2;
	else if (*cltype=="col3") ct=3;
	
	params.getParam("weight1", fw1);
	params.getParam("weight2", fw2);
	params.getParam("weight3", fw3);
	params.getParam("weight4", fw4);
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
	
	textureVoronoi_t * tex = new textureVoronoi_t(col1, col2, ct, fw1, fw2, fw3, fw4, mex, sz, isc, *dname);
	tex->setAdjustments(intensity, contrast, saturation, hue, clamp, factor_red, factor_green, factor_blue);
	if(use_color_ramp) textureReadColorRamp(params, tex);
	
	return tex;
}

//-----------------------------------------------------------------------------------------
// Musgrave block
//-----------------------------------------------------------------------------------------

textureMusgrave_t::textureMusgrave_t(const color_t &c1, const color_t &c2,
				float H, float lacu, float octs, float offs, float gain,
				float _size, float _iscale,
				const std::string &ntype, const std::string &mtype)
				:color1(c1), color2(c2), size(_size), iscale(_iscale)
{
	nGen = newNoise(ntype);
	if (mtype=="multifractal")
		mGen = new mFractal_t(H, lacu, octs, nGen);
	else if (mtype=="heteroterrain")
		mGen = new heteroTerrain_t(H, lacu, octs, offs, nGen);
	else if (mtype=="hybridmf")
		mGen = new hybridMFractal_t(H, lacu, octs, offs, gain, nGen);
	else if (mtype=="ridgedmf")
		mGen = new ridgedMFractal_t(H, lacu, octs, offs, gain, nGen);
	else	// 'fBm' default
		mGen = new fBm_t(H, lacu, octs, nGen);
}

textureMusgrave_t::~textureMusgrave_t()
{
	if (nGen) {
		delete nGen;
		nGen = nullptr;
	}
	if (mGen) {
		delete mGen;
		mGen = nullptr;
	}
}

float textureMusgrave_t::getFloat(const point3d_t &p) const
{
	return applyIntensityContrastAdjustments(iscale * (*mGen)(p*size));
}

colorA_t textureMusgrave_t::getColor(const point3d_t &p, bool from_postprocessed) const
{
	if(!color_ramp) return applyColorAdjustments(color1 + getFloat(p)*(color2 - color1));
	else return applyColorAdjustments(color_ramp->get_color_interpolated(getFloat(p)));
}

texture_t *textureMusgrave_t::factory(paraMap_t &params, renderEnvironment_t &render)
{
	color_t col1(0.0), col2(1.0);
	std::string _ntype, _mtype;
	const std::string *ntype=&_ntype, *mtype=&_mtype;
	float H=1, lacu=2, octs=2, offs=1, gain=1, size=1, iscale=1;
	float intensity = 1.f, contrast = 1.f, saturation = 1.f, hue = 0.f, factor_red = 1.f, factor_green = 1.f, factor_blue = 1.f;
	bool clamp = false;
	bool use_color_ramp = false;
	
	params.getParam("color1", col1);
	params.getParam("color2", col2);
	
	params.getParam("musgrave_type", mtype);
	params.getParam("noise_type", ntype);
	
	params.getParam("H", H);
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

	textureMusgrave_t * tex = new textureMusgrave_t(col1, col2, H, lacu, octs, offs, gain, size, iscale, *ntype, *mtype);
	tex->setAdjustments(intensity, contrast, saturation, hue, clamp, factor_red, factor_green, factor_blue);
	if(use_color_ramp) textureReadColorRamp(params, tex);
	
	return tex;
}

//-----------------------------------------------------------------------------------------
// Distored Noise block
//-----------------------------------------------------------------------------------------

textureDistortedNoise_t::textureDistortedNoise_t(const color_t &c1, const color_t &c2,
			float _distort, float _size,
			const std::string &noiseb1, const std::string noiseb2)
			:color1(c1), color2(c2), distort(_distort), size(_size)
{
	nGen1 = newNoise(noiseb1);
	nGen2 = newNoise(noiseb2);
}

textureDistortedNoise_t::~textureDistortedNoise_t()
{
	if (nGen1) {
		delete nGen1;
		nGen1 = nullptr;
	}
	if (nGen2) {
		delete nGen2;
		nGen2 = nullptr;
	}
}

float textureDistortedNoise_t::getFloat(const point3d_t &p) const
{
	// get a random vector and scale the randomization
	const point3d_t ofs(13.5, 13.5, 13.5);
	point3d_t tp(p*size);
	point3d_t rv(getSignedNoise(nGen1, tp+ofs), getSignedNoise(nGen1, tp), getSignedNoise(nGen1, tp-ofs));
	return applyIntensityContrastAdjustments(getSignedNoise(nGen2, tp+rv*distort));	// distorted-domain noise
}

colorA_t textureDistortedNoise_t::getColor(const point3d_t &p, bool from_postprocessed) const
{
	if(!color_ramp) return applyColorAdjustments(color1 + getFloat(p)*(color2 - color1));
	else return applyColorAdjustments(color_ramp->get_color_interpolated(getFloat(p)));
}

texture_t *textureDistortedNoise_t::factory(paraMap_t &params, renderEnvironment_t &render)
{
	color_t col1(0.0), col2(1.0);
	std::string _ntype1, _ntype2;
	const std::string *ntype1=&_ntype1, *ntype2=&_ntype2;
	float dist=1, size=1;
	float intensity = 1.f, contrast = 1.f, saturation = 1.f, hue = 0.f, factor_red = 1.f, factor_green = 1.f, factor_blue = 1.f;
	bool clamp = false;
	bool use_color_ramp = false;
	
	params.getParam("color1", col1);
	params.getParam("color2", col2);
	
	params.getParam("noise_type1", ntype1);
	params.getParam("noise_type2", ntype2);
	
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
	
	textureDistortedNoise_t * tex = new textureDistortedNoise_t(col1, col2, dist, size, *ntype1, *ntype2);
	tex->setAdjustments(intensity, contrast, saturation, hue, clamp, factor_red, factor_green, factor_blue);
	if(use_color_ramp) textureReadColorRamp(params, tex);
	
	return tex;
}

//-----------------------------------------------------------------------------------------
// Blend Texture
//-----------------------------------------------------------------------------------------


textureBlend_t::textureBlend_t(const std::string &stype, bool use_flip_axis)
{
	m_use_flip_axis = use_flip_axis;
	
	if(stype == "lin") blendProgressionType = TEX_BLEND_LINEAR;
	else if(stype == "quad") blendProgressionType = TEX_BLEND_QUADRATIC;
	else if(stype == "ease") blendProgressionType = TEX_BLEND_EASING;
	else if(stype == "diag") blendProgressionType = TEX_BLEND_DIAGONAL;
	else if(stype == "sphere") blendProgressionType = TEX_BLEND_SPHERICAL;
	else if(stype == "halo" || stype == "quad_sphere") blendProgressionType = TEX_BLEND_QUADRATIC_SPHERE;
	else if(stype == "radial") blendProgressionType = TEX_BLEND_RADIAL;
	else blendProgressionType = TEX_BLEND_LINEAR;
}

textureBlend_t::~textureBlend_t()
{
}

float textureBlend_t::getFloat(const point3d_t &p) const
{
	float blend = 0.f;
	
	float coord1 = p.x;
	float coord2 = p.y;
	
	if(m_use_flip_axis)
	{
		coord1 = p.y;
		coord2 = p.x;
	}
	
	if(blendProgressionType == TEX_BLEND_QUADRATIC)
	{
		// Transform -1..1 to 0..1
		blend = 0.5f * ( coord1 + 1.f );
		if(blend < 0.f) blend = 0.f;
		else blend *= blend;
	}
	else if(blendProgressionType == TEX_BLEND_EASING)
	{
		blend = 0.5f * ( coord1 + 1.f );
		if(blend <= 0.f) blend = 0.f;
		else if(blend >= 1.f) blend = 1.f;
		else
		{
			blend = (3.f * blend * blend - 2.f * blend * blend * blend);
		}
	}
	else if(blendProgressionType == TEX_BLEND_DIAGONAL)
	{
		blend = 0.25f * (2.f + coord1 + coord2);
	}
	else if(blendProgressionType == TEX_BLEND_SPHERICAL || blendProgressionType == TEX_BLEND_QUADRATIC_SPHERE)
	{
		blend = 1.f - fSqrt(coord1 * coord1 + coord2 * coord2 + p.z * p.z);
		if(blend < 0.f) blend = 0.f;
		if(blendProgressionType == TEX_BLEND_QUADRATIC_SPHERE) blend *= blend;
	}
	else if(blendProgressionType == TEX_BLEND_RADIAL)
	{
		blend = (atan2f(coord2, coord1) / (float)(2.f * M_PI) + 0.5f);
	}
	else  //linear by default
	{
		// Transform -1..1 to 0..1
		blend = 0.5f * ( coord1 + 1.f );
	}
	// Clipping to 0..1
	blend = std::max(0.f, std::min(blend, 1.f));

	return applyIntensityContrastAdjustments(blend);
}

colorA_t textureBlend_t::getColor(const point3d_t &p, bool from_postprocessed) const
{
	if(!color_ramp) return applyColorAdjustments(color_t(getFloat(p)));
	else return applyColorAdjustments(color_ramp->get_color_interpolated(getFloat(p)));
}

texture_t *textureBlend_t::factory(paraMap_t &params, renderEnvironment_t &render)
{
	std::string _stype;
	const std::string *stype=&_stype;
	float intensity = 1.f, contrast = 1.f, saturation = 1.f, hue = 0.f, factor_red = 1.f, factor_green = 1.f, factor_blue = 1.f;
	bool clamp = false;
	bool use_color_ramp = false;
	bool use_flip_axis = false;
	
	params.getParam("stype", _stype);
	params.getParam("use_color_ramp", use_color_ramp);
	params.getParam("use_flip_axis", use_flip_axis);
	
	textureBlend_t * tex = new textureBlend_t(*stype, use_flip_axis);
	tex->setAdjustments(intensity, contrast, saturation, hue, clamp, factor_red, factor_green, factor_blue);
	if(use_color_ramp) textureReadColorRamp(params, tex);
	
	return tex;
}

extern "C"
{
	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("blend",		textureBlend_t::factory);
		render.registerFactory("clouds", 	textureClouds_t::factory);
		render.registerFactory("marble", 	textureMarble_t::factory);
		render.registerFactory("wood", 		textureWood_t::factory);
		render.registerFactory("voronoi", 	textureVoronoi_t::factory);
		render.registerFactory("musgrave", 	textureMusgrave_t::factory);
		render.registerFactory("distorted_noise", textureDistortedNoise_t::factory);
		render.registerFactory("rgb_cube",	rgbCube_t::factory);
		render.registerFactory("image",		textureImage_t::factory);
	}
}

__END_YAFRAY
