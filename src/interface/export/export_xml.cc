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

#include "interface/export/export_xml.h"
#include "common/logger.h"
#include "scene/scene.h"
#include "geometry/matrix.h"
#include "geometry/primitive/face_indices.h"
#include "param/param.h"
#include "render/progress_bar.h"
#include "common/version_build_info.h"

namespace yafaray {

ExportXml::ExportXml(const char *fname, const ::yafaray_LoggerCallback logger_callback, void *callback_data, ::yafaray_DisplayConsole logger_display_console) : Interface(logger_callback, callback_data, logger_display_console), file_name_(std::string(fname))
{
	file_.open(file_name_.c_str());
	if(!file_.is_open())
	{
		logger_.logError("XmlExport: Couldn't open ", file_name_);
		return;
	}
	else logger_.logInfo("XmlExport: Writing scene to: ", file_name_);
	file_ << std::boolalpha;
	file_ << "<?xml version=\"1.0\"?>\n";
	file_ << "<yafaray_xml format_version=\"" << buildinfo::getVersionMajor() << "." << buildinfo::getVersionMinor() << "." << buildinfo::getVersionPatch() << "\">\n\n";
}

void ExportXml::createScene() noexcept
{
	file_ << "<scene>\n";
	writeParamMap(*params_);
	params_->clear();
	file_ << "</scene>\n";
}

void ExportXml::clearAll() noexcept
{
	if(logger_.isVerbose()) logger_.logVerbose("XmlExport: cleaning up...");
	if(file_.is_open())
	{
		file_.flush();
		file_.close();
	}
	params_->clear();
	nodes_params_.clear();
	cparams_ = params_.get();
	next_obj_ = 0;
}

void ExportXml::defineLayer() noexcept
{
	file_ << "\n<layer>\n";
	writeParamMap(*params_);
	params_->clear();
	file_ << "</layer>\n";
}

bool ExportXml::initObject(size_t object_id, size_t material_id) noexcept
{
	file_ << "</object>\n";
	return true;
}

int ExportXml::addVertex(size_t object_id, Point3f &&vertex, unsigned char time_step) noexcept
{
	file_ << "\t<p x=\"" << vertex[Axis::X] << "\" y=\"" << vertex[Axis::Y] << "\" z=\"" << vertex[Axis::Z];
	if(time_step > 0) file_ << "\" t=\"" << time_step;
	file_ << "\"/>\n";
	return 0;
}

int ExportXml::addVertex(size_t object_id, Point3f &&vertex, Point3f &&orco, unsigned char time_step) noexcept
{
	file_ << "\t<p x=\"" << vertex[Axis::X] << "\" y=\"" << vertex[Axis::Y] << "\" z=\"" << vertex[Axis::Z]
			  << "\" ox=\"" << orco[Axis::X] << "\" oy=\"" << orco[Axis::Y] << "\" oz=\"" << orco[Axis::Z];
	if(time_step > 0) file_ << "\" t=\"" << time_step;
	file_ << "\"/>\n";
	return 0;
}

void ExportXml::addVertexNormal(size_t object_id, Vec3f &&normal, unsigned char time_step) noexcept
{
	file_ << "\t<n x=\"" << normal[Axis::X] << "\" y=\"" << normal[Axis::Y] << "\" z=\"" << normal[Axis::Z];
	if(time_step > 0) file_ << "\" t=\"" << time_step;
	file_ << "\"/>\n";
}

bool ExportXml::addFace(size_t object_id, const FaceIndices<int> &face_indices, size_t material_id) noexcept
{
	file_ << "\t<f";
	const int num_vertices{face_indices.numVertices()};
	for(int i = 0; i < num_vertices; ++i)
	{
		const unsigned char vertex_char = 'a' + i;
		file_ << " " << vertex_char << "=\"" << face_indices[i].vertex_ << "\"";
	}
	if(face_indices.hasUv())
	{
		for(size_t i = 0; i < num_vertices; ++i)
		{
			const unsigned char vertex_char = 'a' + i;
			file_ << " uv_" << vertex_char << "=\"" << face_indices[i].uv_ << "\"";
		}
	}
	file_ << "/>\n";
	return true;
}

int ExportXml::addUv(size_t object_id, Uv<float> &&uv) noexcept
{
	file_ << "\t<uv u=\"" << uv.u_ << "\" v=\"" << uv.v_ << "\"/>\n";
	return n_uvs_++;
}

bool ExportXml::smoothVerticesNormals(size_t object_id, double angle) noexcept
{
	file_ << "<smooth object_id=\"" << object_id << "\" angle=\"" << angle << "\"/>\n";
	return true;
}

void ExportXml::writeMatrix(const std::string &name, const Matrix4f &m, std::ofstream &file) noexcept
{
	file << "<" << name << " m00=\"" << m[0][0] << "\" m01=\"" << m[0][1] << "\" m02=\"" << m[0][2] << "\" m03=\"" << m[0][3] << "\""
		 << " m10=\"" << m[1][0] << "\" m11=\"" << m[1][1] << "\" m12=\"" << m[1][2] << "\" m13=\"" << m[1][3] << "\""
		 << " m20=\"" << m[2][0] << "\" m21=\"" << m[2][1] << "\" m22=\"" << m[2][2] << "\" m23=\"" << m[2][3] << "\""
		 << " m30=\"" << m[3][0] << "\" m31=\"" << m[3][1] << "\" m32=\"" << m[3][2] << "\" m33=\"" << m[3][3] << "\"/>";
}

void ExportXml::writeParam(const std::string &name, const Parameter &param, std::ofstream &file, ColorSpace color_space, float gamma) noexcept
{
	const Parameter::Type type = param.type();
	if(type == Parameter::Type::Int)
	{
		int i = 0;
		param.getVal(i);
		file << "<" << name << " ival=\"" << i << "\"/>\n";
	}
	else if(type == Parameter::Type::Bool)
	{
		bool b = false;
		param.getVal(b);
		file << "<" << name << " bval=\"" << b << "\"/>\n";
	}
	else if(type == Parameter::Type::Float)
	{
		double f = 0.0;
		param.getVal(f);
		file << "<" << name << " fval=\"" << f << "\"/>\n";
	}
	else if(type == Parameter::Type::String)
	{
		std::string s;
		param.getVal(s);
		file << "<" << name << " sval=\"" << s << "\"/>\n";
	}
	else if(type == Parameter::Type::Vector)
	{
		Point3f p{{0.f, 0.f, 0.f}};
		param.getVal(p);
		file << "<" << name << " x=\"" << p[Axis::X] << "\" y=\"" << p[Axis::Y] << "\" z=\"" << p[Axis::Z] << "\"/>\n";
	}
	else if(type == Parameter::Type::Color)
	{
		Rgba c(0.f);
		param.getVal(c);
		c.colorSpaceFromLinearRgb(color_space, gamma);    //Color values are encoded to the desired color space before saving them to the XML file
		file << "<" << name << " r=\"" << c.r_ << "\" g=\"" << c.g_ << "\" b=\"" << c.b_ << "\" a=\"" << c.a_ << "\"/>\n";
	}
	else if(type == Parameter::Type::Matrix)
	{
		Matrix4f m;
		param.getVal(m);
		writeMatrix(name, m, file);
	}
	else
	{
		std::cerr << "unknown parameter type!\n";
	}
}

size_t ExportXml::createInstance() noexcept
{
	file_ << "\n<createInstance></createInstance>\n";
	return current_instance_id_++;
}

bool ExportXml::addInstanceObject(size_t instance_id, size_t base_object_id) noexcept
{
	file_ << "\n<addInstanceObject instance_id=\"" << instance_id << "\" base_object_id=\"" << base_object_id << "\"></addInstanceObject>\n";
	return true;
}

bool ExportXml::addInstanceOfInstance(size_t instance_id, size_t base_instance_id) noexcept
{
	file_ << "\n<addInstanceOfInstance instance_id=\"" << instance_id << "\" base_instance_id=\"" << base_instance_id << "\"></addInstanceOfInstance>\n";
	return true;
}

bool ExportXml::addInstanceMatrix(size_t instance_id, Matrix4f &&obj_to_world, float time) noexcept
{
	file_ << "\n<addInstanceMatrix instance_id=\"" << instance_id << "\" time=\"" << time << "\">\n\t";
	writeMatrix("transform", obj_to_world, file_);
	file_ << "\n</addInstanceMatrix>\n";
	return true;
}

void ExportXml::writeParamMap(const ParamMap &param_map, int indent) noexcept
{
	const std::string tabs(indent, '\t');
	for(const auto &[param_name, param] : param_map)
	{
		file_ << tabs;
		writeParam(param_name, param, file_, color_space_, gamma_);
	}
}

void ExportXml::writeParamList(int indent) noexcept
{
	const std::string tabs(indent, '\t');
	for(const auto &param : nodes_params_)
	{
		file_ << tabs << "<list_element>\n";
		writeParamMap(param, indent + 1);
		file_ << tabs << "</list_element>\n";
	}
}

std::pair<size_t, ParamResult> ExportXml::createLight(const std::string &name) noexcept
{
	file_ << "\n<light name=\"" << name << "\">\n";
	writeParamMap(*params_);
	file_ << "</light>\n";
	return {};
}

std::pair<size_t, ParamResult> ExportXml::createTexture(const std::string &name) noexcept
{
	file_ << "\n<texture name=\"" << name << "\">\n";
	writeParamMap(*params_);
	file_ << "</texture>\n";
	return {};
}

std::pair<size_t, ParamResult> ExportXml::createMaterial(const std::string &name) noexcept
{
	file_ << "\n<material name=\"" << name << "\">\n";
	writeParamMap(*params_);
	writeParamList(1);
	file_ << "</material>\n";
	return {};
}
std::pair<size_t, ParamResult> ExportXml::createCamera(const std::string &name) noexcept
{
	file_ << "\n<camera name=\"" << name << "\">\n";
	writeParamMap(*params_);
	file_ << "</camera>\n";
	return {};
}

ParamResult ExportXml::defineBackground() noexcept
{
	file_ << "\n<background>\n";
	writeParamMap(*params_);
	file_ << "</background>\n";
	return {};
}

ParamResult ExportXml::defineSurfaceIntegrator() noexcept
{
	file_ << "\n<surface_integrator>\n";
	writeParamMap(*params_);
	file_ << "</surface_integrator>\n";
	return {};
}

ParamResult ExportXml::defineVolumeIntegrator() noexcept
{
	file_ << "\n<volume_integrator>\n";
	writeParamMap(*params_);
	file_ << "</volume_integrator>\n";
	return {};
}

std::pair<size_t, ParamResult> ExportXml::createVolumeRegion(const std::string &name) noexcept
{
	file_ << "\n<volumeregion name=\"" << name << "\">\n";
	writeParamMap(*params_);
	file_ << "</volumeregion>\n";
	return {};
}

std::pair<size_t, ParamResult> ExportXml::createOutput(const std::string &name) noexcept
{
	file_ << "\n<output name=\"" << name << "\">\n";
	writeParamMap(*params_);
	file_ << "</output>\n";
	return {};
}

std::pair<size_t, ParamResult> ExportXml::createRenderView(const std::string &name) noexcept
{
	std::string render_view_name{name};
	if(render_view_name.empty()) render_view_name = "render_view";
	file_ << "\n<render_view name=\"" << render_view_name << "\">\n";
	writeParamMap(*params_);
	file_ << "</render_view>\n";
	return {};
}

std::pair<size_t, ParamResult> ExportXml::createImage(const std::string &name) noexcept
{
	file_ << "\n<image name=\"" << name << "\">\n";
	writeParamMap(*params_);
	file_ << "</image>\n";
	return {};
}

std::pair<size_t, ParamResult> ExportXml::createObject(const std::string &name) noexcept
{
	n_uvs_ = 0;
	file_ << "\n<object>\n";
	file_ << "\t<parameters name=\"" << name << "\">\n";
	writeParamMap(*params_, 2);
	file_ << "\t</parameters>\n";
	++next_obj_;
	return {next_obj_ - 1, ParamResult{}};
}

void ExportXml::setupRender() noexcept
{
	file_ << "\n<render>\n";
	writeParamMap(*params_);
	file_ << "</render>\n";
}

void ExportXml::render(std::unique_ptr<ProgressBar> progress_bar) noexcept
{
	file_ << "</yafaray_xml>\n";
	file_.flush();
	file_.close();
}

void ExportXml::setColorSpace(const std::string& color_space_string, float gamma_val) noexcept
{
	color_space_.initFromString(color_space_string);
	gamma_ = gamma_val;
}

} //namespace yafaray
