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
 */

#ifndef YAFARAY_ENVIRONMENT_H
#define YAFARAY_ENVIRONMENT_H

#include "constants.h"
#include "renderpasses.h"
#include <list>

BEGIN_YAFARAY
class Light;
class Material;
class VolumeHandler;
class VolumeRegion;
class Texture;
class Camera;
class Background;
class Integrator;
class ShaderNode;
class Integrator;
class ObjectGeometric;
class ImageFilm;
class Scene;
class ColorOutput;
class ProgressBar;
class ImageHandler;
class ParamMap;

class LIBYAFARAY_EXPORT RenderEnvironment final
{
	public:
		RenderEnvironment();
		~RenderEnvironment();

		Material *getMaterial(const std::string &name) const;
		Texture *getTexture(const std::string &name) const;
		ShaderNode *getShaderNode(const std::string &name) const;
		Camera *getCamera(const std::string &name) const;
		Background 	*getBackground(const std::string &name) const;
		Integrator 	*getIntegrator(const std::string &name) const;
		Scene 		*getScene() const { return current_scene_; }

		Light 		*createLight(const std::string &name, ParamMap &params);
		Texture 		*createTexture(const std::string &name, ParamMap &params);
		Material 	*createMaterial(const std::string &name, ParamMap &params, std::list<ParamMap> &eparams);
		ObjectGeometric 	*createObject(const std::string &name, ParamMap &params);
		Camera 		*createCamera(const std::string &name, ParamMap &params);
		Background 	*createBackground(const std::string &name, ParamMap &params);
		Integrator 	*createIntegrator(const std::string &name, ParamMap &params);
		ShaderNode 	*createShaderNode(const std::string &name, ParamMap &params);
		VolumeHandler *createVolumeH(const std::string &name, const ParamMap &params);
		VolumeRegion	*createVolumeRegion(const std::string &name, ParamMap &params);
		ImageFilm	*createImageFilm(const ParamMap &params, ColorOutput &output);
		ImageHandler *createImageHandler(const std::string &name, ParamMap &params);

		void 			setScene(Scene *scene) { current_scene_ = scene; };
		bool			setupScene(Scene &scene, const ParamMap &params, ColorOutput &output, ProgressBar *pb = nullptr);
		void			setupRenderPasses(const ParamMap &params);
		void			setupLoggingAndBadge(const ParamMap &params);
		const			RenderPasses *getRenderPasses() const { return &render_passes_; }
		const 			std::map<std::string, Camera *> *getCameraTable() const { return &cameras_; }
		void			setOutput2(ColorOutput *out_2) { output_2_ = out_2; }
		ColorOutput	*getOutput2() const { return output_2_; }

		void clearAll();

	private:
		std::map<std::string, Light *> 	lights_;
		std::map<std::string, Material *> 	materials_;
		std::map<std::string, Texture *> 	textures_;
		std::map<std::string, ObjectGeometric *> 	objects_;
		std::map<std::string, Camera *> 	cameras_;
		std::map<std::string, Background *> backgrounds_;
		std::map<std::string, Integrator *> integrators_;
		std::map<std::string, ShaderNode *> shaders_;
		std::map<std::string, VolumeHandler *> volumes_;
		std::map<std::string, VolumeRegion *> volumeregions_;
		std::map<std::string, ImageHandler *> imagehandlers_;

		Scene *current_scene_;
		RenderPasses render_passes_;
		ColorOutput *output_2_; //secondary color output to export to file at the same time it's exported to Blender
};

END_YAFARAY
#endif // YAFARAY_ENVIRONMENT_H
