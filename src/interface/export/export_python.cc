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

#include "interface/export/export_python.h"
#include "common/logger.h"
#include "scene/scene.h"
#include "geometry/matrix.h"
#include "geometry/primitive/face_indices.h"
#include "param/param.h"
#include "render/progress_bar.h"

namespace yafaray {

ExportPython::ExportPython(const char *fname, const ::yafaray_LoggerCallback logger_callback, void *callback_data, ::yafaray_DisplayConsole logger_display_console) : Interface(logger_callback, callback_data, logger_display_console), file_name_(std::string(fname))
{
	file_.open(file_name_.c_str());
	if(!file_.is_open())
	{
		logger_.logError("CExport: Couldn't open ", file_name_);
		return;
	}
	else logger_.logInfo("CExport: Writing scene to: ", file_name_);
	file_ << "# Python3 file generated by libYafaRay Python Export\n";
	file_ << "# To run this file, execute the following line with Python v3.3 or higher (use \"python\" or \"python3\" depending on the operating system):\n";
	file_ << "# python3 (yafaray_python_file_name.py)\n";
	file_ << "# Alternatively if using a portable python or the \"libyafaray4_bindings\" library is not in the standard python library folder and the above does not work, try the following instead:\n";
	file_ << "# LD_LIBRARY_PATH=(path to folder with python and libyafaray libs) PYTHONPATH=(path to folder with python and libyafaray libs) python3 (yafaray_python_file_name.py)\n\n";
	file_ << "import libyafaray4_bindings\n\n";
	file_ << "yi = libyafaray4_bindings.Interface()\n";
	file_ << "yi.setConsoleVerbosityLevel(yi.logLevelFromString(\"debug\"))\n";
	file_ << "yi.setLogVerbosityLevel(yi.logLevelFromString(\"debug\"))\n";
	file_ << "yi.setConsoleLogColorsEnabled(True)\n";
	file_ << "yi.enablePrintDateTime(True)\n\n";
}

void ExportPython::createScene() noexcept
{
	writeParamMap(*params_);
	params_->clear();
	file_ << "yi.createScene()\n";
	file_ << "yi.paramsClearAll()\n\n";
}

void ExportPython::clearAll() noexcept
{
	if(logger_.isVerbose()) logger_.logVerbose("CExport: cleaning up...");
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

void ExportPython::defineLayer() noexcept
{
	writeParamMap(*params_);
	params_->clear();
	file_ << "yi.defineLayer()\n";
	file_ << "yi.paramsClearAll()\n\n";
}

bool ExportPython::initObject(size_t object_id, size_t material_id) noexcept
{
	file_ << "yi.initObject()\n";
	file_ << "yi.paramsClearAll()\n\n";
	return true;
}

int ExportPython::addVertex(size_t object_id, Point3f &&vertex, unsigned char time_step) noexcept
{
	file_ << "yi.addVertex(" << vertex[Axis::X] << ", " << vertex[Axis::Y] << ", " << vertex[Axis::Z];
	if(time_step > 0) file_ << ", " << time_step;
	file_ << ")\n";
	return 0;
}

int ExportPython::addVertex(size_t object_id, Point3f &&vertex, Point3f &&orco, unsigned char time_step) noexcept
{
	file_ << "yi.addVertexWithOrco(" << vertex[Axis::X] << ", " << vertex[Axis::Y] << ", " << vertex[Axis::Z] << ", " << orco[Axis::X] << ", " << orco[Axis::Y] << ", " << orco[Axis::Z];
	if(time_step > 0) file_ << ", " << time_step;
	file_ << ")\n";
	return 0;
}

void ExportPython::addVertexNormal(size_t object_id, Vec3f &&normal, unsigned char time_step) noexcept
{
	file_ << "yi.addNormal(" << normal[Axis::X] << ", " << normal[Axis::Y] << ", " << normal[Axis::Z];
	if(time_step > 0) file_ << ", " << time_step;
	file_ << ")\n";
}

bool ExportPython::addFace(size_t object_id, const FaceIndices<int> &face_indices, size_t material_id) noexcept
{
	const int num_vertices{face_indices.numVertices()};
	const bool has_uv{face_indices.hasUv()};
	if(has_uv)
	{
		if(num_vertices == 3) file_ << "yi.addTriangleWithUv(";
		else file_ << "yi.addQuadWithUv(";
	}
	else
	{
		if(num_vertices == 3) file_ << "yi.addTriangle(";
		else file_ << "yi.addQuad(";
	}

	for(int i = 0; i < num_vertices; ++i)
	{
		if(i > 0) file_ << ", ";
		file_ << face_indices[i].vertex_;
	}
	if(has_uv)
	{
		for(int i = 0; i < num_vertices; ++i)
		{
			file_ << ", " << face_indices[i].uv_;
		}
	}
	file_ << ")\n";
	return true;
}

int ExportPython::addUv(size_t object_id, Uv<float> &&uv) noexcept
{
	file_ << "yi.addUv(" << uv.u_ << ", " << uv.v_ << ")\n";
	return n_uvs_++;
}

bool ExportPython::smoothVerticesNormals(size_t object_id, double angle) noexcept
{
	file_ << "yi.smoothObjectMesh(\"" << object_id << "\", " << angle << ")\n";
	return true;
}

void ExportPython::writeMatrix(const Matrix4f &m, std::ofstream &file) noexcept
{

	file <<
			m[0][0] << ", " << m[0][1] << ", " << m[0][2] << ", " << m[0][3] << ", " <<
			m[1][0] << ", " << m[1][1] << ", " << m[1][2] << ", " << m[1][3] << ", " <<
			m[2][0] << ", " << m[2][1] << ", " << m[2][2] << ", " << m[2][3] << ", " <<
			m[3][0] << ", " << m[3][1] << ", " << m[3][2] << ", " << m[3][3];
}

void ExportPython::writeParam(const std::string &name, const Parameter &param, std::ofstream &file, ColorSpace color_space, float gamma) noexcept
{
	const Parameter::Type type = param.type();
	if(type == Parameter::Type::Int)
	{
		int i = 0;
		param.getVal(i);
		file << "yi.paramsSetInt(\"" << name << "\", " << i << ")\n";
	}
	else if(type == Parameter::Type::Bool)
	{
		bool b = false;
		param.getVal(b);
		file << "yi.paramsSetBool(\"" << name << "\", " << (b ? "True" : "False") << ")\n";
	}
	else if(type == Parameter::Type::Float)
	{
		double f = 0.0;
		param.getVal(f);
		file << "yi.paramsSetFloat(\"" << name << "\", " << f << ")\n";
	}
	else if(type == Parameter::Type::String)
	{
		std::string s;
		param.getVal(s);
		file << "yi.paramsSetString(\"" << name << "\", \"" << s << "\")\n";
	}
	else if(type == Parameter::Type::Vector)
	{
		Point3f p{{0.f, 0.f, 0.f}};
		param.getVal(p);
		file << "yi.paramsSetVector(\"" << name << "\", " << p[Axis::X] << ", " << p[Axis::Y] << ", " << p[Axis::Z] << ")\n";
	}
	else if(type == Parameter::Type::Color)
	{
		Rgba c(0.f);
		param.getVal(c);
		c.colorSpaceFromLinearRgb(color_space, gamma);    //Color values are encoded to the desired color space before saving them to the XML file
		file << "yi.paramsSetColor(\"" << name << "\", " << c.r_ << ", " << c.g_ << ", " << c.b_ << ", " << c.a_ << ")\n";
	}
	else if(type == Parameter::Type::Matrix)
	{
		Matrix4f m;
		param.getVal(m);
		file << "yi.paramsSetMatrix(";
		writeMatrix(m, file);
		file << ")\n";
	}
	else
	{
		std::cerr << "unknown parameter type!\n";
	}
}

size_t ExportPython::createInstance() noexcept
{
	file_ << "yi.createInstance()\n";
	return current_instance_id_++;
}

bool ExportPython::addInstanceObject(size_t instance_id, size_t base_object_id) noexcept
{
	file_ << "yi.addInstanceObject(" << instance_id << ", \"" << base_object_id << "\")\n"; //FIXME Should I use the variable name "instance_id" for export instead?
	return true;
}

bool ExportPython::addInstanceOfInstance(size_t instance_id, size_t base_instance_id) noexcept
{
	file_ << "yi.addInstanceOfInstance(" << instance_id << ", " << base_instance_id << ")\n"; //FIXME Should I use the variable name "instance_id" for export instead?
	return true;
}

bool ExportPython::addInstanceMatrix(size_t instance_id, Matrix4f &&obj_to_world, float time) noexcept
{
	file_ << "yi.addInstanceMatrix(" << instance_id << ", "; //FIXME Should I use the variable name "instance_id" for export instead?
	writeMatrix(obj_to_world, file_);
	file_ << ", " << time << ")\n";
	return true;
}

void ExportPython::writeParamMap(const ParamMap &param_map, int indent) noexcept
{
	//const std::string tabs(indent, '\t');
	for(const auto &[param_name, param] : param_map)
	{
	//	file_ << tabs;
		writeParam(param_name, param, file_, color_space_, gamma_);
	}
}

void ExportPython::writeParamList(int indent) noexcept
{
	//const std::string tabs(indent, '\t');
	for(const auto &param : nodes_params_)
	{
		file_ << "yi.paramsPushList()\n";
		writeParamMap(param, 0);
	}
	file_ << "yi.paramsEndList()\n";
}

std::pair<size_t, ParamResult> ExportPython::createLight(const std::string &name) noexcept
{
	writeParamMap(*params_);
	params_->clear();
	file_ << "yi.createLight(\"" << name << "\")\n";
	file_ << "yi.paramsClearAll()\n\n";
	return {};
}

std::pair<size_t, ParamResult> ExportPython::createTexture(const std::string &name) noexcept
{
	writeParamMap(*params_);
	params_->clear();
	file_ << "yi.createTexture(\"" << name << "\")\n";
	file_ << "yi.paramsClearAll()\n\n";
	return {};
}

std::pair<size_t, ParamResult> ExportPython::createMaterial(const std::string &name) noexcept
{
	writeParamMap(*params_);
	writeParamList(1);
	params_->clear();
	nodes_params_.clear();
	file_ << "yi.createMaterial(\"" << name << "\")\n";
	file_ << "yi.paramsClearAll()\n\n";
	return {};
}
std::pair<size_t, ParamResult> ExportPython::createCamera(const std::string &name) noexcept
{
	writeParamMap(*params_);
	params_->clear();
	file_ << "yi.createCamera(\"" << name << "\")\n";
	file_ << "yi.paramsClearAll()\n\n";
	return {};
}

ParamResult ExportPython::defineBackground() noexcept
{
	writeParamMap(*params_);
	params_->clear();
	file_ << "yi.defineBackground()\n";
	file_ << "yi.paramsClearAll()\n\n";
	return {};
}

ParamResult ExportPython::defineSurfaceIntegrator() noexcept
{
	writeParamMap(*params_);
	params_->clear();
	file_ << "yi.defineSurfaceIntegrator()\n";
	file_ << "yi.paramsClearAll()\n\n";
	return {};
}

ParamResult ExportPython::defineVolumeIntegrator() noexcept
{
	writeParamMap(*params_);
	params_->clear();
	file_ << "yi.defineVolumeIntegrator()\n";
	file_ << "yi.paramsClearAll()\n\n";
	return {};
}

std::pair<size_t, ParamResult> ExportPython::createVolumeRegion(const std::string &name) noexcept
{
	writeParamMap(*params_);
	params_->clear();
	file_ << "yi.createVolumeRegion(\"" << name << "\")\n";
	file_ << "yi.paramsClearAll()\n\n";
	return {};
}

std::pair<size_t, ParamResult> ExportPython::createOutput(const std::string &name) noexcept
{
	writeParamMap(*params_);
	params_->clear();
	file_ << "yi.createOutput(\"" << name << "\")\n";
	file_ << "yi.paramsClearAll()\n\n";
	return {};
}

std::pair<size_t, ParamResult> ExportPython::createRenderView(const std::string &name) noexcept
{
	writeParamMap(*params_);
	params_->clear();
	file_ << "yi.createRenderView(\"" << name << "\")\n";
	file_ << "yi.paramsClearAll()\n\n";
	return {};
}

std::pair<size_t, ParamResult> ExportPython::createImage(const std::string &name) noexcept
{
	writeParamMap(*params_);
	params_->clear();
	file_ << "yi.createImage(\"" << name << "\")\n";
	file_ << "yi.paramsClearAll()\n\n";
	return {};
}

std::pair<size_t, ParamResult> ExportPython::createObject(const std::string &name) noexcept
{
	n_uvs_ = 0;
	writeParamMap(*params_);
	params_->clear();
	file_ << "yi.createObject(\"" << name << "\")\n";
	file_ << "yi.paramsClearAll()\n\n";
	++next_obj_;
	return {next_obj_ - 1, ParamResult{}};
}

void ExportPython::setupRender() noexcept
{
	writeParamMap(*params_);
	params_->clear();
	file_ << "yi.setupRender()\n";
	file_ << "yi.paramsClearAll()\n\n";
}

void ExportPython::render(std::unique_ptr<ProgressBar> progress_bar) noexcept
{
	file_ << "# Creating image output #\n";
	file_ << "yi.paramsSetString(\"image_path\", \"./test01-output1.tga\")\n";
	file_ << "yi.paramsSetString(\"color_space\", \"sRGB\")\n";
	file_ << "yi.paramsSetString(\"badge_position\", \"top\")\n";
	file_ << "yi.createOutput(\"output1_tga\")\n";
	file_ << "yi.paramsClearAll()\n\n";

	file_ << "yi.render(0, 0)\n";
	file_ << "del yi\n";
	params_->clear();
	nodes_params_.clear();
	file_.flush();
	file_.close();
}

void ExportPython::setColorSpace(const std::string& color_space_string, float gamma_val) noexcept
{
	color_space_.initFromString(color_space_string);
	gamma_ = gamma_val;
}

} //namespace yafaray
