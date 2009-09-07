
#include <textures/layernode.h>

__BEGIN_YAFRAY

color_t texture_rgb_blend(const color_t &tex, const color_t &out, CFLOAT fact, CFLOAT facg, mix_modes blendtype);
CFLOAT texture_value_blend(CFLOAT tex, CFLOAT out, CFLOAT fact, CFLOAT facg, mix_modes blendtype, bool flip);

layerNode_t::layerNode_t(unsigned tflag, CFLOAT col_fac, CFLOAT val_fac, CFLOAT def_val, colorA_t def_col, mix_modes mmod):
			input(0), upperLayer(0), texflag(tflag), colfac(col_fac), valfac(val_fac), default_val(def_val),
			default_col(def_col), mode(mmod), do_color(false), do_scalar(false), color_input(false)
{}

void layerNode_t::eval(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp)const
{
	colorA_t rcol, texcolor;
	CFLOAT rval, Tin=0.f, Ta=1.f, stencilTin = 1.f;
	// == get result of upper layer (or base values) ==
	rcol = (upperLayer) ? upperLayer->getColor(stack) : upper_col;
	rval = (upperLayer) ? upperLayer->getScalar(stack) : upper_val;
	stencilTin = rcol.A;
	
	// == get texture input color ==
	bool TEX_RGB = color_input;
	if(color_input)
	{
		texcolor = input->getColor(stack);
		//Tin=texcolor.energy();
		Ta=texcolor.A;
	}
	else Tin = input->getScalar(stack);
	//bool Talpha = true;
	
	if(texflag & TXF_RGBTOINT){
		Tin = 0.35f*texcolor.getR() + 0.45f*texcolor.getG() + 0.2f*texcolor.getB();
		TEX_RGB = false;
	}
	if(texflag & TXF_NEGATIVE){
		if (TEX_RGB) texcolor = colorA_t(1.f)-texcolor;
		Tin = 1.f-Tin;
	}
	CFLOAT fact;
	if(texflag & TXF_STENCIL)
	{
		if(TEX_RGB) // only scalar input affects stencil...?
		{
			fact = Ta;
			Ta *= stencilTin;
			stencilTin *= fact;
		}
		else
		{
			fact = Tin;
			Tin *= stencilTin;
			stencilTin *= fact;
		}
	}
	
	// color type modulation
	if (do_color)
	{
		if(!TEX_RGB)	texcolor = default_col;
		else			Tin = Ta;
		
		rcol = texture_rgb_blend(texcolor, rcol, Tin, stencilTin * colfac, mode);
		rcol.clampRGB0();
	}
	
	// intensity type modulation
	if (do_scalar)
	{
		if(TEX_RGB)
		{
			if(use_alpha)
			{
				Tin = Ta;
				if(texflag & TXF_NEGATIVE) Tin = 1.f - Tin;
			}
			else Tin = 0.35f*texcolor.getR() + 0.45f*texcolor.getG() + 0.2f*texcolor.getB();
		}
		
		rval = texture_value_blend(default_val, rval, Tin, stencilTin * valfac, mode, (do_scalar<0));
		if(rval<0.f) rval=0.f;
	}
	rcol.A = stencilTin;
	stack[this->ID] = nodeResult_t(rcol, rval);
}

void layerNode_t::eval(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi)const
{
	eval(stack, state, sp);
}

void layerNode_t::evalDerivative(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp)const
{
	colorA_t texcolor;
	CFLOAT rdu=0.f, rdv=0.f, tdu, tdv;
	CFLOAT stencilTin = 1.f;
	// == get result of upper layer (or base values) ==
	if(upperLayer)
	{
		colorA_t ucol = upperLayer->getColor(stack);
		rdu = ucol.R, rdv = ucol.G;
		stencilTin = ucol.A;
	}
	
	// == get texture input derivative ==
	texcolor = input->getColor(stack);
	tdu = texcolor.R;
	tdv = texcolor.G;
	
	if(texflag & TXF_NEGATIVE)
	{
		tdu = -tdu;
		tdv = -tdv;
	}
	/* if(texflag & TXF_STENCIL)
	{
		CFLOAT fact;
		if(TEX_RGB) // only scalar input affects stencil...?
		{
			fact = Ta;
			Ta *= stencilTin;
			stencilTin *= fact;
		}
		else
		{
			fact = Tin;
			Tin *= stencilTin;
			stencilTin *= fact;
		}
	} */
	
	// derivative modulation
	
	//rdu = texture_value_blend(default_val, rdu, Tin, stencilTin * valfac, mode, (do_scalar<0));
	//rdv = texture_value_blend(default_val, rdv, Tin, stencilTin * valfac, mode, (do_scalar<0));
	rdu += tdu;
	rdv += tdv;
	
	//rcol.A = stencilTin;
	stack[this->ID] = nodeResult_t(colorA_t(rdu, rdv, 0.f, stencilTin), 0.f);
}

bool layerNode_t::isViewDependant() const
{
	bool viewDep = false;
	if(input) viewDep = viewDep || input->isViewDependant();
	if(upperLayer) viewDep = viewDep || upperLayer->isViewDependant();
	return viewDep;
}

bool layerNode_t::configInputs(const paraMap_t &params, const nodeFinder_t &find)
{
	const std::string *name=0;
	if( params.getParam("input", name) )
	{
		input = find(*name);
		if(!input){ std::cout << "layerNode_t::configInputs: couldn't get input " << *name << std::endl; return false; }
	}
	else { std::cout << "layerNode_t::configInputs: input not set\n"; return false; }
	
	if( params.getParam("upper_layer", name) )
	{
		upperLayer = find(*name);
		if(!upperLayer){ std::cout << "layerNode_t::configInputs: couldn't get upper_layer " << *name << std::endl; return false; }
	}
	else
	{
		if(!params.getParam("upper_color", upper_col))
		{
			upper_col = color_t(0.f);
			//std::cout << "layerNode_t::configInputs: warning, neither input nor color set for layer input\n";
		}
		if(!params.getParam("upper_value", upper_val))
		{
			upper_val = 0.f;
			//std::cout << "layerNode_t::configInputs: warning, neither input nor value set for layer input\n";
		}
	}
	return true;
}

bool layerNode_t::getDependencies(std::vector<const shaderNode_t*> &dep) const
{
	// input actually needs to exist, but well...
	if(input) dep.push_back(input);
	if(upperLayer) dep.push_back(upperLayer);
	return !dep.empty();
}

// void layerNode_t::getDerivative(const surfacePoint_t &sp, float &du, float &dv)const;

shaderNode_t* layerNode_t::factory(const paraMap_t &params,renderEnvironment_t &render)
{
	color_t def_col(1.f);
	bool do_color=true, do_scalar=false, color_input=true, use_alpha=false;
	bool stencil=false, noRGB=false, negative=false;
	double def_val=1.0, colfac=1.0, valfac=1.0;
	int mode=0;
	
	params.getParam("mode", mode);
	params.getParam("def_col", def_col);
	params.getParam("colfac", colfac);
	params.getParam("def_val", def_val);
	params.getParam("valfac", valfac);
	params.getParam("do_color", do_color);
	params.getParam("do_scalar", do_scalar);
	params.getParam("color_input", color_input);
	params.getParam("use_alpha", use_alpha);
	params.getParam("noRGB", noRGB);
	params.getParam("stencil", stencil);
	params.getParam("negative", negative);
	
	unsigned int flags = 0;
	if(noRGB) flags |= TXF_RGBTOINT;
	if(stencil) flags |= TXF_STENCIL;
	if(negative) flags |= TXF_NEGATIVE;
	if(use_alpha) flags |= TXF_ALPHAMIX;
	
	layerNode_t *node = new layerNode_t(flags, colfac, valfac, def_val, def_col, (mix_modes)mode);
	node->do_color = do_color;
	node->do_scalar = do_scalar;
	node->color_input = color_input;
	node->use_alpha = use_alpha;
	
	return node;
}

//////////////////


color_t texture_rgb_blend(const color_t &tex, const color_t &out, CFLOAT fact, CFLOAT facg, mix_modes blendtype)
{
	
	switch(blendtype) {
		case MN_MULT:
			fact *= facg;
			return (color_t(1.f-facg) + fact*tex)*out;

		case MN_SCREEN: {
			color_t white(1.0);
			fact *= facg;
			return white - (color_t(1.f-facg) + fact*(white-tex)) * (white-out);
		}

		case MN_SUB:
			fact = -fact;
		case MN_ADD:
			fact *= facg;
			return fact*tex + out;

		case MN_DIV: {
			fact *= facg;
			color_t itex(tex);
			itex.invertRGB();
			return (1.f-fact)*out + fact*out*itex;
		}

		case MN_DIFF: {
			fact *= facg;
			color_t tmo(tex-out);
			tmo.absRGB();
			return (1.f-fact)*out + fact*tmo;
		}

		case MN_DARK: {
			fact *= facg;
			color_t col(fact*tex);
			col.darkenRGB(out);
			return col;
		}

		case MN_LIGHT: {
			fact *= facg;
			color_t col(fact*tex);
			col.lightenRGB(out);
			return col;
		}

		default:
		case MN_MIX:
			fact *= facg;
			return fact*tex + (1.f-fact)*out;
	}

}

CFLOAT texture_value_blend(CFLOAT tex, CFLOAT out, CFLOAT fact, CFLOAT facg, mix_modes blendtype, bool flip)
{
	fact *= facg;
	CFLOAT facm = 1.f-fact;
	if (flip) std::swap(fact, facm);

	switch(blendtype) {
		case MN_MULT:
			facm = 1.f-facg;
			return (facm+fact*tex)*out;

		case MN_SCREEN:
			facm = 1.f-facg;
			return 1.f-(facm+fact*(1.f-tex))*(1.f-out);

		case MN_SUB:
			fact = -fact;
		case MN_ADD:
			return fact*tex + out;

		case MN_DIV:
			if (tex==0.f) return 0.f;
			return facm*out + fact*out/tex;

		case MN_DIFF:
			return facm*out + fact*std::fabs(tex-out);

		case MN_DARK: {
			CFLOAT col = fact*tex;
			if (col<out) return col;
			return out;
		}

		case MN_LIGHT: {
			CFLOAT col = fact*tex;
			if (col>out) return col;
			return out;
		}

		default:
		case MN_MIX:
			return fact*tex + facm*out;
	}
}


__END_YAFRAY
