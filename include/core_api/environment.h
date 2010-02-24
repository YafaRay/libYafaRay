#ifndef Y_ENVIRON_H
#define Y_ENVIRON_H

#include <yafray_config.h>

#include "params.h"
//#include "light.h"
//#include "material.h"
//#include "texture.h"
//#include "background.h"
//#include "camera.h"
#include "yafsystem.h"
#include <list>

__BEGIN_YAFRAY
class light_t;
class material_t;
class volumeHandler_t;
class VolumeRegion;
class texture_t;
class camera_t;
class background_t;
class integrator_t;
class shaderNode_t;
class integrator_t;
class object3d_t;
class imageFilm_t;
class scene_t;
class colorOutput_t;
class progressBar_t;

class YAFRAYCORE_EXPORT renderEnvironment_t
{
	public:
		void loadPlugins(const std::string &path);
		bool getPluginPath(std::string &path);
		
		typedef light_t 		*light_factory_t(paraMap_t &,renderEnvironment_t &);
		typedef material_t 		*material_factory_t(paraMap_t &,std::list<paraMap_t> &, renderEnvironment_t &);
		typedef texture_t 		*texture_factory_t(paraMap_t &,renderEnvironment_t &);
		typedef object3d_t 		*object_factory_t(paraMap_t &,renderEnvironment_t &);
		typedef camera_t 		*camera_factory_t(paraMap_t &,renderEnvironment_t &);
		typedef background_t 	*background_factory_t(paraMap_t &,renderEnvironment_t &);
		typedef integrator_t 	*integrator_factory_t(paraMap_t &,renderEnvironment_t &);
		typedef shaderNode_t 	*shader_factory_t(const paraMap_t &,renderEnvironment_t &);
		typedef volumeHandler_t *volume_factory_t(const paraMap_t &,renderEnvironment_t &);
		typedef VolumeRegion	*volumeregion_factory_t(paraMap_t &,renderEnvironment_t &);
//		typedef filter_t 	*filter_factory_t(paraMap_t &,renderEnvironment_t &);
//		typedef pluginInfo_t info_t();
		
/*		template <class T>
		T* getParam(const std::string &name, T &val)
		{
			std::map<std::string,parameter_t>::iterator i=dicc.find(name);
			if(i != dicc.end() ) return i->second.getVal(val);
			return false;
		}
*/		
		virtual material_t *getMaterial(const std::string &name)const;
		virtual texture_t *getTexture(const std::string &name)const;
		virtual shaderNode_t *getShaderNode(const std::string &name)const;
		camera_t *getCamera(const std::string &name)const;
		shader_factory_t* getShaderNodeFactory(const std::string &name)const;
		background_t* 	getBackground(const std::string &name)const;
		integrator_t* 	getIntegrator(const std::string &name)const;
		scene_t * 		getScene() { return curren_scene; };
		
		light_t* 		createLight		(const std::string &name, paraMap_t &params);
		texture_t* 		createTexture	(const std::string &name, paraMap_t &params);
		material_t* 	createMaterial	(const std::string &name, paraMap_t &params, std::list<paraMap_t> &eparams);
		object3d_t* 	createObject	(const std::string &name, paraMap_t &params);
		camera_t* 		createCamera	(const std::string &name, paraMap_t &params);
		background_t* 	createBackground(const std::string &name, paraMap_t &params);
		integrator_t* 	createIntegrator(const std::string &name, paraMap_t &params);
		shaderNode_t* 	createShaderNode(const std::string &name, paraMap_t &params);
		volumeHandler_t* createVolumeH(const std::string &name, const paraMap_t &params);
		VolumeRegion*	createVolumeRegion(const std::string &name, paraMap_t &params);
		imageFilm_t*	createImageFilm(const paraMap_t &params, colorOutput_t &output);
		void 			setScene(scene_t *scene) { curren_scene=scene; };
		bool			setupScene(scene_t &scene, const paraMap_t &params, colorOutput_t &output, progressBar_t *pb = 0);
		void clearAll();

		virtual void registerFactory(const std::string &name,light_factory_t *f);
		virtual void registerFactory(const std::string &name,material_factory_t *f);
		virtual void registerFactory(const std::string &name,texture_factory_t *f);
		virtual void registerFactory(const std::string &name,object_factory_t *f);
		virtual void registerFactory(const std::string &name,camera_factory_t *f);
//		virtual void registerFactory(const std::string &name,filter_factory_t *f);
		virtual void registerFactory(const std::string &name,background_factory_t *f);
		virtual void registerFactory(const std::string &name,integrator_factory_t *f);
		virtual void registerFactory(const std::string &name,shader_factory_t *f);
		virtual void registerFactory(const std::string &name,volume_factory_t *f);
		virtual void registerFactory(const std::string &name,volumeregion_factory_t *f);

// 		void addToParamsString(const char *params);
// 		const char *getParamsString();
// 		void clearParamsString();
//		void setDrawParams(bool b);
//		bool getDrawParams();
		int Debug;

		renderEnvironment_t();
		virtual ~renderEnvironment_t();
		
	protected:
		std::list< sharedlibrary_t > 	pluginHandlers;
		std::map<std::string,light_factory_t *> 	light_factory;
		std::map<std::string,material_factory_t *> 	material_factory;
		std::map<std::string,texture_factory_t *> 	texture_factory;
		std::map<std::string,object_factory_t *> 	object_factory;
		std::map<std::string,camera_factory_t *> 	camera_factory;
//		std::map<std::string,filter_factory_t *> filter_factory;
		std::map<std::string,background_factory_t *> background_factory;
		std::map<std::string,integrator_factory_t *> integrator_factory;
		std::map<std::string,shader_factory_t *> 	shader_factory;
		std::map<std::string,volume_factory_t *> 	volume_factory;
		std::map<std::string,volumeregion_factory_t *> 	volumeregion_factory;
		
		std::map<std::string,light_t *> 	light_table;
		std::map<std::string,material_t *> 	material_table;
		std::map<std::string,texture_t *> 	texture_table;
		std::map<std::string,object3d_t *> 	object_table;
		std::map<std::string,camera_t *> 	camera_table;
//		std::map<std::string,filter_t *> filter_table;
		std::map<std::string,background_t *> background_table;
		std::map<std::string,integrator_t *> integrator_table;
		std::map<std::string,shaderNode_t *> shader_table;
		std::map<std::string,volumeHandler_t *> volume_table;
		std::map<std::string,VolumeRegion *> volumeregion_table;
//		bool drawParamsString;
//		std::string paramsString;
		scene_t *curren_scene;
};

__END_YAFRAY
#endif // Y_ENVIRON_H
