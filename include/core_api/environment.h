#pragma once

#ifndef YAFARAY_ENVIRONMENT_H
#define YAFARAY_ENVIRONMENT_H

#include <yafray_constants.h>
#include "renderpasses.h"
#include "dynamic_library.h"
#include <list>

BEGIN_YAFRAY
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
class Object3D;
class ImageFilm;
class Scene;
class ColorOutput;
class ProgressBar;
class ImageHandler;
class ParamMap;

class YAFRAYCORE_EXPORT RenderEnvironment final
{
	public:
		RenderEnvironment();
		~RenderEnvironment();

		void loadPlugins(const std::string &path);
		bool getPluginPath(std::string &path);

		typedef Light 		*LightFactory_t(ParamMap &, RenderEnvironment &);
		typedef Material 		*MaterialFactory_t(ParamMap &, std::list<ParamMap> &, RenderEnvironment &);
		typedef Texture 		*TextureFactory_t(ParamMap &, RenderEnvironment &);
		typedef Object3D 		*ObjectFactory_t(ParamMap &, RenderEnvironment &);
		typedef Camera 		*CameraFactory_t(ParamMap &, RenderEnvironment &);
		typedef Background 	*BackgroundFactory_t(ParamMap &, RenderEnvironment &);
		typedef Integrator 	*IntegratorFactory_t(ParamMap &, RenderEnvironment &);
		typedef ShaderNode 	*ShaderFactory_t(const ParamMap &, RenderEnvironment &);
		typedef VolumeHandler *VolumeFactory_t(const ParamMap &, RenderEnvironment &);
		typedef VolumeRegion	*VolumeregionFactory_t(ParamMap &, RenderEnvironment &);
		typedef ImageHandler	*ImagehandlerFactory_t(ParamMap &, RenderEnvironment &);

		Material *getMaterial(const std::string &name) const;
		Texture *getTexture(const std::string &name) const;
		ShaderNode *getShaderNode(const std::string &name) const;
		Camera *getCamera(const std::string &name) const;
		ShaderFactory_t *getShaderNodeFactory(const std::string &name) const;
		Background 	*getBackground(const std::string &name) const;
		Integrator 	*getIntegrator(const std::string &name) const;
		Scene 		*getScene() const { return current_scene_; }

		Light 		*createLight(const std::string &name, ParamMap &params);
		Texture 		*createTexture(const std::string &name, ParamMap &params);
		Material 	*createMaterial(const std::string &name, ParamMap &params, std::list<ParamMap> &eparams);
		Object3D 	*createObject(const std::string &name, ParamMap &params);
		Camera 		*createCamera(const std::string &name, ParamMap &params);
		Background 	*createBackground(const std::string &name, ParamMap &params);
		Integrator 	*createIntegrator(const std::string &name, ParamMap &params);
		ShaderNode 	*createShaderNode(const std::string &name, ParamMap &params);
		VolumeHandler *createVolumeH(const std::string &name, const ParamMap &params);
		VolumeRegion	*createVolumeRegion(const std::string &name, ParamMap &params);
		ImageFilm	*createImageFilm(const ParamMap &params, ColorOutput &output);
		ImageHandler *createImageHandler(const std::string &name, ParamMap &params, bool add_to_table = true);

		void 			setScene(Scene *scene) { current_scene_ = scene; };
		bool			setupScene(Scene &scene, const ParamMap &params, ColorOutput &output, ProgressBar *pb = nullptr);
		void			setupRenderPasses(const ParamMap &params);
		void			setupLoggingAndBadge(const ParamMap &params);
		const			RenderPasses *getRenderPasses() const { return &render_passes_; }
		const 			std::map<std::string, Camera *> *getCameraTable() const { return &cameras_; }
		void			setOutput2(ColorOutput *out_2) { output_2_ = out_2; }
		ColorOutput	*getOutput2() const { return output_2_; }

		void clearAll();

		void registerFactory(const std::string &name, LightFactory_t *f);
		void registerFactory(const std::string &name, MaterialFactory_t *f);
		void registerFactory(const std::string &name, TextureFactory_t *f);
		void registerFactory(const std::string &name, ObjectFactory_t *f);
		void registerFactory(const std::string &name, CameraFactory_t *f);
		void registerFactory(const std::string &name, BackgroundFactory_t *f);
		void registerFactory(const std::string &name, IntegratorFactory_t *f);
		void registerFactory(const std::string &name, ShaderFactory_t *f);
		void registerFactory(const std::string &name, VolumeFactory_t *f);
		void registerFactory(const std::string &name, VolumeregionFactory_t *f);
		void registerImageHandler(const std::string &name, const std::string &valid_extensions, const std::string &full_name, ImagehandlerFactory_t *f);
		std::vector<std::string> listImageHandlers();
		std::vector<std::string> listImageHandlersFullName();

		std::string getImageFormatFromFullName(const std::string &fullname);
		std::string getImageFormatFromExtension(const std::string &extension);
		std::string getImageFullNameFromFormat(const std::string &format);

	private:
		std::list< DynamicLoadedLibrary > 	plugin_handlers_;
		std::map<std::string, LightFactory_t *> 	light_factory_;
		std::map<std::string, MaterialFactory_t *> 	material_factory_;
		std::map<std::string, TextureFactory_t *> 	texture_factory_;
		std::map<std::string, ObjectFactory_t *> 	object_factory_;
		std::map<std::string, CameraFactory_t *> 	camera_factory_;
		std::map<std::string, BackgroundFactory_t *> background_factory_;
		std::map<std::string, IntegratorFactory_t *> integrator_factory_;
		std::map<std::string, ShaderFactory_t *> 	shader_factory_;
		std::map<std::string, VolumeFactory_t *> 	volume_factory_;
		std::map<std::string, VolumeregionFactory_t *> 	volumeregion_factory_;
		std::map<std::string, ImagehandlerFactory_t *> 	imagehandler_factory_;

		std::map<std::string, Light *> 	lights_;
		std::map<std::string, Material *> 	materials_;
		std::map<std::string, Texture *> 	textures_;
		std::map<std::string, Object3D *> 	objects_;
		std::map<std::string, Camera *> 	cameras_;
		std::map<std::string, Background *> backgrounds_;
		std::map<std::string, Integrator *> integrators_;
		std::map<std::string, ShaderNode *> shaders_;
		std::map<std::string, VolumeHandler *> volumes_;
		std::map<std::string, VolumeRegion *> volumeregions_;
		std::map<std::string, ImageHandler *> imagehandlers_;
		std::map<std::string, std::string> imagehandlers_fullnames_;
		std::map<std::string, std::string> imagehandlers_extensions_;

		Scene *current_scene_;
		RenderPasses render_passes_;
		ColorOutput *output_2_; //secondary color output to export to file at the same time it's exported to Blender
};

END_YAFRAY
#endif // YAFARAY_ENVIRONMENT_H
