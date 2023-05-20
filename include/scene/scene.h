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

#ifndef LIBYAFARAY_SCENE_H
#define LIBYAFARAY_SCENE_H

#include "common/items.h"
#include "param/param.h"
#include <list>

namespace yafaray {

class Object;
template <typename IndexType> class FaceIndices;
class Instance;
class Light;
class Material;
class VolumeHandler;
class VolumeRegion;
class Texture;
class Background;
class ShaderNode;
class ImageFilm;
class Scene;
class ImageOutput;
class Format;
class SurfaceIntegrator;
class VolumeIntegrator;
class SurfacePoint;
template <typename T, size_t N> class SquareMatrix;
typedef SquareMatrix<float, 4> Matrix4f;
class Rgb;
class Rgba;
class Accelerator;
template <typename T, size_t N> class Point;
typedef Point<float, 3> Point3f;
typedef Point<int, 2> Point2i;
template <typename T, size_t N> class Vec;
typedef Vec<float, 3> Vec3f;
typedef Vec<int, 2> Size2i;
template<typename T> class Bound;
struct ParamResult;
class Logger;
class Image;
class RenderControl;
class RenderMonitor;
template <typename T> struct Uv;
enum class DarkDetectionType : unsigned char;

class Scene final
{
	public:
		inline static std::string getClassName() { return "Scene"; }
		Scene(Logger &logger, const std::string &name);
		~Scene();
		std::string getName() const { return name_; }
		[[nodiscard]] std::string exportToString(size_t indent_level, yafaray_ContainerExportType container_export_type, bool only_export_non_default_parameters) const;
		void setAcceleratorParamMap(const ParamMap &param_map);
		int addVertex(size_t object_id, Point3f &&p, unsigned char time_step);
		int addVertex(size_t object_id, Point3f &&p, Point3f &&orco, unsigned char time_step);
		void addVertexNormal(size_t object_id, Vec3f &&n, unsigned char time_step);
		bool addFace(size_t object_id, const FaceIndices<int> &face_indices, size_t material_id);
		int addUv(size_t object_id, Uv<float> &&uv);
		bool smoothVerticesNormals(size_t object_id, float angle);
		std::pair<size_t, ParamResult> createObject(const std::string &name, const ParamMap &param_map);
		bool initObject(size_t object_id, size_t material_id);
		size_t createInstance();
		bool addInstanceObject(size_t instance_id, size_t object_id);
		bool addInstanceOfInstance(size_t instance_id, size_t base_instance_id);
		bool addInstanceMatrix(size_t instance_id, Matrix4f &&obj_to_world, float time);
		std::pair<const Instance *, ResultFlags> getInstance(size_t instance_id) const;
		yafaray_SceneModifiedFlags checkAndClearSceneModifiedFlags();
		bool preprocess(const RenderControl &render_control, yafaray_SceneModifiedFlags scene_modified_flags);
		std::tuple<const Object *, size_t, ResultFlags> getObject(const std::string &name) const;
		std::pair<const Object *, ResultFlags> getObject(size_t object_id) const;
		const Items<Object> &getObjects() const { return objects_; }
		const Accelerator *getAccelerator() const { return accelerator_.get(); }
		void createDefaultMaterial();
		const Background *getBackground() const;
		Bound<float> getSceneBound() const;
		std::pair<size_t, ResultFlags> getMaterial(const std::string &name) const;
		const Items<Material> &getMaterials() const { return materials_; }
		std::pair<Size2i, bool> getImageSize(size_t image_id) const;
		std::pair<Rgba, bool> getImageColor(size_t image_id, const Point2i &point) const;
		bool setImageColor(size_t image_id, const Point2i &point, const Rgba &col);
		std::tuple<Image *, size_t, ResultFlags> getImage(const std::string &name) const;
		const Items<VolumeRegion> &getVolumeRegions() const { return volume_regions_; }
		const Items<Light> &getLights() const { return lights_; }
		Items<Light> &getLights() { return lights_; }
		std::tuple<const Light *, size_t, ResultFlags> getLight(const std::string &name) const;
		std::pair<const Light *, ResultFlags> getLight(size_t object_id) const;
		std::pair<size_t, ParamResult> createLight(const std::string &name, const ParamMap &param_map);
		bool disableLight(const std::string &name);
		const Items<Texture> &getTextures() const { return textures_; }
		const Items<Image> &getImages() const { return images_; }
		std::tuple<const Texture *, size_t, ResultFlags> getTexture(const std::string &name) const;
		std::pair<size_t, ParamResult> createTexture(const std::string &name, const ParamMap &param_map);
		std::pair<size_t, ParamResult> createMaterial(const std::string &name, const ParamMap &param_map, const std::list<ParamMap> &param_map_list_nodes);
		ParamResult defineBackground(const ParamMap &param_map);
		std::pair<size_t, ParamResult> createVolumeRegion(const std::string &name, const ParamMap &param_map);
		std::pair<size_t, ParamResult> createImage(const std::string &name, const ParamMap &param_map);
		bool mipMapInterpolationRequired() const { return mipmap_interpolation_required_; }
		size_t getObjectIndexHighest() const { return object_index_highest_; }
		size_t getMaterialIndexHighest() const { return material_index_highest_; }

	private:
		std::string name_{"Renderer"};
		std::unique_ptr<Bound<float>> scene_bound_; //!< bounding box of all (finite) scene geometry
		int object_index_highest_ = 1; //!< Highest object index used for the Normalized Object Index pass.
		int material_index_highest_ = 1; //!< Highest material index used for the Normalized Object Index pass.
		size_t material_id_default_ = 0;
		bool mipmap_interpolation_required_{false}; //!< Indicates if there are any textures that require mipmap interpolation
		ParamMap accelerator_param_map_;
		bool accelerator_param_map_modified_{true};
		std::unique_ptr<const Accelerator> accelerator_;
		std::unique_ptr<Background> background_;
		std::vector<std::unique_ptr<Instance>> instances_;
		Items<Object> objects_;
		Items<Light> lights_;
		Items<Material> materials_;
		Items<Texture> textures_;
		Items<VolumeRegion> volume_regions_;
		Items<Image> images_;
		Logger &logger_;
};

} //namespace yafaray

#endif // LIBYAFARAY_SCENE_H
