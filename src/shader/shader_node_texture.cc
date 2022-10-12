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

#include "shader/shader_node_texture.h"
#include "texture/mipmap_params.h"
#include "camera/camera.h"
#include "scene/scene.h"
#include "geometry/surface.h"

namespace yafaray {

TextureMapperNode::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_LOAD(texture_);
	PARAM_LOAD(transform_);
	PARAM_LOAD(scale_);
	PARAM_LOAD(offset_);
	PARAM_LOAD(do_scalar_);
	PARAM_LOAD(bump_strength_);
	PARAM_LOAD(proj_x_);
	PARAM_LOAD(proj_y_);
	PARAM_LOAD(proj_z_);
	PARAM_ENUM_LOAD(texco_);
	PARAM_ENUM_LOAD(mapping_);
}

ParamMap TextureMapperNode::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(texture_);
	PARAM_SAVE(transform_);
	PARAM_SAVE(scale_);
	PARAM_SAVE(offset_);
	PARAM_SAVE(do_scalar_);
	PARAM_SAVE(bump_strength_);
	PARAM_SAVE(proj_x_);
	PARAM_SAVE(proj_y_);
	PARAM_SAVE(proj_z_);
	PARAM_ENUM_SAVE(texco_);
	PARAM_ENUM_SAVE(mapping_);
	PARAM_SAVE_END;
}

ParamMap TextureMapperNode::getAsParamMap(bool only_non_default) const
{
	ParamMap result{ShaderNode::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<ShaderNode *, ParamError> TextureMapperNode::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {})};
	std::string texname;
	if(param_map.getParam(Params::texture_meta_.name(), texname).notOk())
	{
		logger.logError("TextureMapper: No texture given for texture mapper!");
		return {nullptr, ParamError{ParamError::Flags::ErrorWhileCreating}};
	}
	const Texture *tex = scene.getTexture(texname);
	if(!tex)
	{
		logger.logError("TextureMapper: texture '", texname, "' does not exist!");
		return {nullptr, ParamError{ParamError::Flags::ErrorWhileCreating}};
	}
	auto result {new TextureMapperNode(logger, param_error, param_map, tex)};
	if(param_error.notOk()) logger.logWarning(param_error.print<TextureMapperNode>(name, {"type"}));
	return {result, param_error};
}

TextureMapperNode::TextureMapperNode(Logger &logger, ParamError &param_error, const ParamMap &param_map, const Texture *texture) :
		ShaderNode{logger, param_error, param_map}, params_{param_error, param_map}, tex_(texture)
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
	//logger.logParams(getAsParamMap(false).print()); //TEST CODE ONLY, REMOVE!!
	if(tex_->discrete())
	{
		const auto [u, v, w] = tex_->resolution();
		d_u_ = 1.f / static_cast<float>(u);
		d_v_ = 1.f / static_cast<float>(v);
		if(tex_->isThreeD()) d_w_ = 1.f / static_cast<float>(w);
		else d_w_ = 0.f;
	}
	else
	{
		constexpr float step = 0.0002f;
		d_u_ = d_v_ = d_w_ = step;
	}

	p_du_ = {{d_u_, 0.f, 0.f}};
	p_dv_ = {{0.f, d_v_, 0.f}};
	p_dw_ = {{0.f, 0.f, d_w_}};

	bump_strength_ /= params_.scale_.length();
	if(!tex_->isNormalmap()) bump_strength_ /= 100.0f;
}

// Map the texture to a cylinder
Point3f TextureMapperNode::tubeMap(const Point3f &p)
{
	Point3f res;
	res[Axis::Y] = p[Axis::Z];
	const float d = p[Axis::X] * p[Axis::X] + p[Axis::Y] * p[Axis::Y];
	if(d > 0.f)
	{
		res[Axis::Z] = 1.f / math::sqrt(d);
		res[Axis::X] = -std::atan2(p[Axis::X], p[Axis::Y]) * math::div_1_by_pi<>;
	}
	else res[Axis::X] = res[Axis::Z] = 0.f;
	return res;
}

// Map the texture to a sphere
Point3f TextureMapperNode::sphereMap(const Point3f &p)
{
	Point3f res{{0.f, 0.f, 0.f}};
	const float d = p[Axis::X] * p[Axis::X] + p[Axis::Y] * p[Axis::Y] + p[Axis::Z] * p[Axis::Z];
	if(d > 0.f)
	{
		res[Axis::Z] = math::sqrt(d);
		if((p[Axis::X] != 0.f) && (p[Axis::Y] != 0.f)) res[Axis::X] = -std::atan2(p[Axis::X], p[Axis::Y]) * math::div_1_by_pi<>;
		res[Axis::Y] = 1.f - 2.f * (math::acos(p[Axis::Z] / res[Axis::Z]) * math::div_1_by_pi<>);
	}
	return res;
}

// Map the texture to a cube
Point3f TextureMapperNode::cubeMap(const Point3f &p, const Vec3f &n)
{
	const std::array<std::array<Axis, 3>, 3> ma {{ {Axis::Y, Axis::Z, Axis::X}, {Axis::X, Axis::Z, Axis::Y}, {Axis::X, Axis::Y, Axis::Z} }};
	// int axis = std::abs(n.x) > std::abs(n.y) ? (std::abs(n.x) > std::abs(n.z) ? 0 : 2) : (std::abs(n.y) > std::abs(n.z) ? 1 : 2);
	// no functionality changes, just more readable code
	Axis axis;
	if(std::abs(n[Axis::Z]) >= std::abs(n[Axis::X]) && std::abs(n[Axis::Z]) >= std::abs(n[Axis::Y])) axis = Axis::Z;
	else if(std::abs(n[Axis::Y]) >= std::abs(n[Axis::X]) && std::abs(n[Axis::Y]) >= std::abs(n[Axis::Z])) axis = Axis::Y;
	else axis = Axis::X;
	const auto axis_id = axis::getId(axis);
	return {{ p[ma[axis_id][0]], p[ma[axis_id][1]], p[ma[axis_id][2]] }};
}

// Map the texture to a plane, but it should not be used by now as it does nothing, it's just for completeness' sake
Point3f TextureMapperNode::flatMap(const Point3f &p)
{
	return p;
}

Point3f TextureMapperNode::doMapping(const Point3f &p, const Vec3f &n) const
{
	Point3f texpt{p};
	// Texture coordinates standardized, if needed, to -1..1
	switch(params_.texco_.value())
	{
		case Coords::Uv: texpt = {{2.f * texpt[Axis::X] - 1.f, 2.f * texpt[Axis::Y] - 1.f, texpt[Axis::Z]}}; break;
		default: break;
	}
	// Texture axis mapping
	const std::array<float, 4> texmap {0.f, texpt[Axis::X], texpt[Axis::Y], texpt[Axis::Z] };
	texpt[Axis::X] = texmap[map_x_];
	texpt[Axis::Y] = texmap[map_y_];
	texpt[Axis::Z] = texmap[map_z_];
	// Texture coordinates mapping
	switch(params_.mapping_.value())
	{
		case Projection::Tube: texpt = tubeMap(texpt); break;
		case Projection::Sphere: texpt = sphereMap(texpt); break;
		case Projection::Cube: texpt = cubeMap(texpt, n); break;
		case Projection::Plain: // texpt = flatmap(texpt); break;
		default: break;
	}
	// Texture scale and offset
	texpt = multiplyComponents(texpt, params_.scale_) + offset_;
	return texpt;
}

std::pair<Point3f, Vec3f> TextureMapperNode::getCoords(const SurfacePoint &sp, const Camera *camera) const
{
	switch(params_.texco_.value())
	{
		case Coords::Uv: return { {{ sp.uv_.u_, sp.uv_.v_, 0.f }}, sp.ng_ };
		case Coords::Orco: return { sp.orco_p_, sp.orco_ng_ };
		case Coords::Transformed: return { params_.transform_ * sp.p_, params_.transform_ * sp.ng_ };  // apply 4x4 matrix of object for mapping also to true surface normals
		case Coords::Window: return { camera->screenproject(sp.p_), sp.ng_ };
		case Coords::Normal: {
			const auto [cam_x, cam_y, cam_z] = camera->getAxes();
			return {{{sp.n_ * cam_x, -sp.n_ * cam_y, 0.f}}, sp.ng_};
			}
		case Coords::Stick: // Not implemented yet use GLOB
		case Coords::Stress: // Not implemented yet use GLOB
		case Coords::Tangent: // Not implemented yet use GLOB
		case Coords::Reflect: // Not implemented yet use GLOB
		case Coords::Global: // GLOB mapped as default
		default: return { sp.p_, sp.ng_ };
	}
}

void TextureMapperNode::eval(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const
{
	std::unique_ptr<const MipMapParams> mip_map_params;
	auto [texpt, ng] = getCoords(sp, camera);
	if((tex_->getInterpolationType() == InterpolationType::Trilinear || tex_->getInterpolationType() == InterpolationType::Ewa) && sp.differentials_)
	{
		const Point3f texpt_orig {texpt};
		texpt = doMapping(texpt_orig, ng);
		if(params_.texco_.value() == Coords::Uv && sp.has_uv_)
		{
			const auto [d_dx, d_dy]{sp.getUVdifferentialsXY()};
			const Point3f texpt_diffx{1.0e+2f * (doMapping(texpt_orig + 1.0e-2f * Point3f{{d_dx.u_, d_dx.v_, 0.f}}, ng) - texpt)};
			const Point3f texpt_diffy{1.0e+2f * (doMapping(texpt_orig + 1.0e-2f * Point3f{{d_dy.u_, d_dy.v_, 0.f}}, ng) - texpt)};
			mip_map_params = std::make_unique<const MipMapParams>(texpt_diffx[Axis::X], texpt_diffx[Axis::Y], texpt_diffy[Axis::X], texpt_diffy[Axis::Y]);
		}
	}
	else texpt = doMapping(texpt, ng);
	node_tree_data[getId()] = {
			tex_->getColor(texpt, mip_map_params.get()),
			params_.do_scalar_ ? tex_->getFloat(texpt, mip_map_params.get()) : 0.f
	};
}

// Normal perturbation

void TextureMapperNode::evalDerivative(NodeTreeData &node_tree_data, const SurfacePoint &sp, const Camera *camera) const
{
	float du = 0.0f, dv = 0.0f;
	auto [texpt, ng] = getCoords(sp, camera);
	if(tex_->discrete() && sp.has_uv_ && params_.texco_.value() == Coords::Uv)
	{
		texpt = doMapping(texpt, ng);
		Vec3f norm;

		if(tex_->isNormalmap())
		{
			// Get color from normal map texture
			const Rgba color = tex_->getRawColor(texpt);
			// Assign normal map RGB colors to vector norm
			norm = {{ color.getR(), color.getG(), color.getB() }};
			norm = (2.f * norm) - Vec3f{1.f}; //FIXME DAVID: does the Vec3 portion make sense?

			// Convert norm into shading space
			du = norm * sp.ds_.u_;
			dv = norm * sp.ds_.v_;
		}
		else
		{
			const Point3f i_0{texpt - p_du_};
			const Point3f i_1{texpt + p_du_};
			const Point3f j_0{texpt - p_dv_};
			const Point3f j_1{texpt + p_dv_};
			const float dfdu = (tex_->getFloat(i_0) - tex_->getFloat(i_1)) / d_u_;
			const float dfdv = (tex_->getFloat(j_0) - tex_->getFloat(j_1)) / d_v_;

			// now we got the derivative in UV-space, but need it in shading space:
			Vec3f vec_u{sp.ds_.u_};
			Vec3f vec_v{sp.ds_.v_};
			vec_u[Axis::Z] = dfdu;
			vec_v[Axis::Z] = dfdv;
			// now we have two vectors NU/NV/df; Solve plane equation to get 1/0/df and 0/1/df (i.e. dNUdf and dNVdf)
			norm = vec_u ^ vec_v;
		}

		norm.normalize();

		if(std::abs(norm[Axis::Z]) > 1e-30f)
		{
			const float nf = 1.f / norm[Axis::Z] * bump_strength_; // normalizes z to 1, why?
			du = norm[Axis::X] * nf;
			dv = norm[Axis::Y] * nf;
		}
		else du = dv = 0.f;
	}
	else
	{
		if(tex_->isNormalmap())
		{
			texpt = doMapping(texpt, ng);

			// Get color from normal map texture
			const Rgba color = tex_->getRawColor(texpt);

			// Assign normal map RGB colors to vector norm
			Vec3f norm {{color.getR(), color.getG(), color.getB() }};
			norm = (2.f * norm) - Vec3f{1.f}; //FIXME DAVID: does the Vec3 portion make sense?

			// Convert norm into shading space
			du = norm * sp.ds_.u_;
			dv = norm * sp.ds_.v_;

			norm.normalize();

			if(std::abs(norm[Axis::Z]) > 1e-30f)
			{
				const float nf = 1.f / norm[Axis::Z] * bump_strength_; // normalizes z to 1, why?
				du = norm[Axis::X] * nf;
				dv = norm[Axis::Y] * nf;
			}
			else du = dv = 0.f;
		}
		else
		{
			// no uv coords -> procedurals usually, this mapping only depends on NU/NV which is fairly arbitrary
			// weird things may happen when objects are rotated, i.e. incorrect bump change
			const Point3f i_0{doMapping(texpt - d_u_ * sp.uvn_.u_, ng)};
			const Point3f i_1{doMapping(texpt + d_u_ * sp.uvn_.u_, ng)};
			const Point3f j_0{doMapping(texpt - d_v_ * sp.uvn_.v_, ng)};
			const Point3f j_1{doMapping(texpt + d_v_ * sp.uvn_.v_, ng)};

			du = (tex_->getFloat(i_0) - tex_->getFloat(i_1)) / d_u_;
			dv = (tex_->getFloat(j_0) - tex_->getFloat(j_1)) / d_v_;
			du *= bump_strength_;
			dv *= bump_strength_;

			if(params_.texco_.value() != Coords::Uv)
			{
				du = -du;
				dv = -dv;
			}
		}
	}
	node_tree_data[getId()] = { {du, dv, 0.f, 0.f}, 0.f };
}

} //namespace yafaray
