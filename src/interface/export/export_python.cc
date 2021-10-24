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
#include "geometry/matrix4.h"
#include "common/param.h"

BEGIN_YAFARAY

ExportPython::ExportPython(const char *fname, const ::yafaray_LoggerCallback_t logger_callback, void *callback_data, ::yafaray_DisplayConsole_t logger_display_console) : Interface(logger_callback, callback_data, logger_display_console), file_name_(std::string(fname))
{
	file_.open(file_name_.c_str());
	if(!file_.is_open())
	{
		logger_->logError("CExport: Couldn't open ", file_name_);
		return;
	}
	else logger_->logInfo("CExport: Writing scene to: ", file_name_);
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
	if(logger_->isVerbose()) logger_->logVerbose("CExport: cleaning up...");
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

bool ExportPython::startGeometry() noexcept { return true; }

bool ExportPython::endGeometry() noexcept { return true; }

unsigned int ExportPython::getNextFreeId() noexcept
{
	return ++next_obj_;
}

bool ExportPython::endObject() noexcept
{
	file_ << "yi.endObject()\n";
	file_ << "yi.paramsClearAll()\n\n";
	return true;
}

int ExportPython::addVertex(double x, double y, double z) noexcept
{
	file_ << "yi.addVertex(" << x << ", " << y << ", " << z << ")\n";
	return 0;
}

int ExportPython::addVertex(double x, double y, double z, double ox, double oy, double oz) noexcept
{
	file_ << "yi.addVertexWithOrco(" << x << ", " << y << ", " << z << ", " << ox << ", " << oy << ", " << oz << ")\n";
	return 0;
}

void ExportPython::addNormal(double x, double y, double z) noexcept
{
	file_ << "yi.addNormal(" << x << ", " << y << ", " << z << ")\n";
}

void ExportPython::setCurrentMaterial(const char *name) noexcept
{
	const std::string name_str(name);
	if(name_str != current_material_) //need to set current material
	{
		file_ << "yi.setCurrentMaterial(\"" << name_str << "\")\n";
		current_material_ = name_str;
	}
}

bool ExportPython::addFace(int a, int b, int c) noexcept
{
	file_ << "yi.addTriangle(" << a << ", " << b << ", " << c << ")\n";
	return true;
}

bool ExportPython::addFace(int a, int b, int c, int uv_a, int uv_b, int uv_c) noexcept
{
	file_ << "yi.addTriangleWithUv(" << a << ", " << b << ", " << c << ", " << uv_a << ", " << uv_b << ", " << uv_c << ")\n";
	return true;
}

int ExportPython::addUv(float u, float v) noexcept
{
	file_ << "yi.addUv(" << u << ", " << v << ")\n";
	return n_uvs_++;
}

bool ExportPython::smoothMesh(const char *name, double angle) noexcept
{
	file_ << "yi.smoothMesh(\"" << name << "\", " << angle << ")\n";
	return true;
}

void ExportPython::writeMatrix(const std::string &name, const Matrix4 &m, std::ofstream &file) noexcept
{

	file << "\"" << name << "\", " <<
			m[0][0] << ", " << m[0][1] << ", " << m[0][2] << ", " << m[0][3] << ", " <<
			m[1][0] << ", " << m[1][1] << ", " << m[1][2] << ", " << m[1][3] << ", " <<
			m[2][0] << ", " << m[2][1] << ", " << m[2][2] << ", " << m[2][3] << ", " <<
			m[3][0] << ", " << m[3][1] << ", " << m[3][2] << ", " << m[3][3];
}

void ExportPython::writeParam(const std::string &name, const Parameter &param, std::ofstream &file, ColorSpace color_space, float gamma) noexcept
{
	const Parameter::Type type = param.type();
	if(type == Parameter::Int)
	{
		int i = 0;
		param.getVal(i);
		file << "yi.paramsSetInt(\"" << name << "\", " << i << ")\n";
	}
	else if(type == Parameter::Bool)
	{
		bool b = false;
		param.getVal(b);
		file << "yi.paramsSetBool(\"" << name << "\", " << (b ? "True" : "False") << ")\n";
	}
	else if(type == Parameter::Float)
	{
		double f = 0.0;
		param.getVal(f);
		file << "yi.paramsSetFloat(\"" << name << "\", " << f << ")\n";
	}
	else if(type == Parameter::String)
	{
		std::string s;
		param.getVal(s);
		if(!s.empty()) file << "yi.paramsSetString(\"" << name << "\", \"" << s << "\")\n";
	}
	else if(type == Parameter::Vector)
	{
		Point3 p(0.f);
		param.getVal(p);
		file << "yi.paramsSetVector(\"" << name << "\", " << p.x_ << ", " << p.y_ << ", " << p.z_ << ")\n";
	}
	else if(type == Parameter::Color)
	{
		Rgba c(0.f);
		param.getVal(c);
		c.colorSpaceFromLinearRgb(color_space, gamma);    //Color values are encoded to the desired color space before saving them to the XML file
		file << "yi.paramsSetColor(\"" << name << "\", " << c.r_ << ", " << c.g_ << ", " << c.b_ << ", " << c.a_ << ")\n";
	}
	else if(type == Parameter::Matrix)
	{
		Matrix4 m;
		param.getVal(m);
		file << "yi.paramsSetMatrix(";
		writeMatrix(name, m, file);
		file << ")\n";
	}
	else
	{
		std::cerr << "unknown parameter type!\n";
	}
}

bool ExportPython::addInstance(const char *base_object_name, const Matrix4 &obj_to_world) noexcept
{
	file_ << "yi.addInstance(";
	writeMatrix(base_object_name, obj_to_world, file_);
	file_ << ")\n";
	return true;
}

void ExportPython::writeParamMap(const ParamMap &param_map, int indent) noexcept
{
	//const std::string tabs(indent, '\t');
	for(const auto &param : param_map)
	{
	//	file_ << tabs;
		writeParam(param.first, param.second, file_, color_space_, gamma_);
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

Light *ExportPython::createLight(const char *name) noexcept
{
	writeParamMap(*params_);
	params_->clear();
	file_ << "yi.createLight(\"" << name << "\")\n";
	file_ << "yi.paramsClearAll()\n\n";
	return nullptr;
}

Texture *ExportPython::createTexture(const char *name) noexcept
{
	writeParamMap(*params_);
	params_->clear();
	file_ << "yi.createTexture(\"" << name << "\")\n";
	file_ << "yi.paramsClearAll()\n\n";
	return nullptr;
}

Material *ExportPython::createMaterial(const char *name) noexcept
{
	writeParamMap(*params_);
	writeParamList(1);
	params_->clear();
	nodes_params_.clear();
	file_ << "yi.createMaterial(\"" << name << "\")\n";
	file_ << "yi.paramsClearAll()\n\n";
	return nullptr;
}
Camera *ExportPython::createCamera(const char *name) noexcept
{
	writeParamMap(*params_);
	params_->clear();
	file_ << "yi.createCamera(\"" << name << "\")\n";
	file_ << "yi.paramsClearAll()\n\n";
	return nullptr;
}
Background *ExportPython::createBackground(const char *name) noexcept
{
	writeParamMap(*params_);
	params_->clear();
	file_ << "yi.createBackground(\"" << name << "\")\n";
	file_ << "yi.paramsClearAll()\n\n";
	return nullptr;
}
Integrator *ExportPython::createIntegrator(const char *name) noexcept
{
	writeParamMap(*params_);
	params_->clear();
	file_ << "yi.createIntegrator(\"" << name << "\")\n";
	file_ << "yi.paramsClearAll()\n\n";
	return nullptr;
}

VolumeRegion *ExportPython::createVolumeRegion(const char *name) noexcept
{
	writeParamMap(*params_);
	params_->clear();
	file_ << "yi.createVolumeRegion(\"" << name << "\")\n";
	file_ << "yi.paramsClearAll()\n\n";
	return nullptr;
}

ImageOutput *ExportPython::createOutput(const char *name) noexcept
{
	writeParamMap(*params_);
	params_->clear();
	file_ << "yi.createOutput(\"" << name << "\")\n";
	file_ << "yi.paramsClearAll()\n\n";
	return nullptr;
}

RenderView *ExportPython::createRenderView(const char *name) noexcept
{
	writeParamMap(*params_);
	params_->clear();
	file_ << "yi.createRenderView(\"" << name << "\")\n";
	file_ << "yi.paramsClearAll()\n\n";
	return nullptr;
}

Image *ExportPython::createImage(const char *name) noexcept
{
	writeParamMap(*params_);
	params_->clear();
	file_ << "yi.createImage(\"" << name << "\")\n";
	file_ << "yi.paramsClearAll()\n\n";
	return nullptr;
}

Object *ExportPython::createObject(const char *name) noexcept
{
	n_uvs_ = 0;
	writeParamMap(*params_);
	params_->clear();
	file_ << "yi.createObject(\"" << name << "\")\n";
	file_ << "yi.paramsClearAll()\n\n";
	++next_obj_;
	return nullptr;
}

void ExportPython::setupRender() noexcept
{
	writeParamMap(*params_);
	params_->clear();
	file_ << "yi.setupRender()\n";
	file_ << "yi.paramsClearAll()\n\n";
}

void ExportPython::render(std::shared_ptr<ProgressBar> progress_bar) noexcept
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

void ExportPython::setColorSpace(std::string color_space_string, float gamma_val) noexcept
{
	color_space_ = Rgb::colorSpaceFromName(color_space_string, ColorSpace::Srgb);
	gamma_ = gamma_val;
}

END_YAFARAY
