/****************************************************************************
 *      This is part of the libYafaRay package
 *
 *      test06.c : scene with quads
 *      Should work even with a "barebones" libYafaRay built without
 *      any dependencies
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

#include "yafaray_c_api.h"

int main()
{
	size_t material_id = 0;
	size_t object_id = 0;
	yafaray_Scene *yi = yafaray_createInterface(YAFARAY_INTERFACE_FOR_RENDERING, NULL, NULL, NULL, YAFARAY_DISPLAY_CONSOLE_NORMAL);
	yafaray_setConsoleLogColorsEnabled(yi, YAFARAY_BOOL_TRUE);
	yafaray_setConsoleVerbosityLevel(yi, YAFARAY_LOG_LEVEL_VERBOSE);

	yafaray_createScene(yi);
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapString(yi, "color_space", "sRGB");
	yafaray_setParamMapString(yi, "filename", "tex.tga");
	yafaray_setParamMapFloat(yi, "gamma", 1);
	yafaray_setParamMapString(yi, "image_optimization", "optimized");
	yafaray_createImage(yi, "Texture_TGA_image", NULL);
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapString(yi, "image_name", "Texture_TGA_image");
	yafaray_setParamMapString(yi, "type", "image");
	yafaray_setParamMapBool(yi, "adj_clamp", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapFloat(yi, "adj_contrast", 1);
	yafaray_setParamMapFloat(yi, "adj_hue", 0);
	yafaray_setParamMapFloat(yi, "adj_intensity", 1);
	yafaray_setParamMapFloat(yi, "adj_mult_factor_blue", 1);
	yafaray_setParamMapFloat(yi, "adj_mult_factor_green", 1);
	yafaray_setParamMapFloat(yi, "adj_mult_factor_red", 1);
	yafaray_setParamMapFloat(yi, "adj_saturation", 1);
	yafaray_setParamMapBool(yi, "calc_alpha", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapString(yi, "clipping", "repeat");
	yafaray_setParamMapFloat(yi, "cropmax_x", 1);
	yafaray_setParamMapFloat(yi, "cropmax_y", 1);
	yafaray_setParamMapFloat(yi, "cropmin_x", 0);
	yafaray_setParamMapFloat(yi, "cropmin_y", 0);
	yafaray_setParamMapFloat(yi, "ewa_max_anisotropy", 8);
	yafaray_setParamMapString(yi, "interpolate", "bilinear");
	yafaray_setParamMapBool(yi, "mirror_x", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(yi, "mirror_y", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(yi, "normalmap", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(yi, "rot90", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapFloat(yi, "trilinear_level_bias", 0);
	yafaray_setParamMapBool(yi, "use_alpha", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapInt(yi, "xrepeat", 1);
	yafaray_setParamMapInt(yi, "yrepeat", 1);
	yafaray_createTexture(yi, "Texture_TGA");
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapString(yi, "color_space", "LinearRGB");
	yafaray_setParamMapString(yi, "filename", "tex.hdr");
	yafaray_setParamMapFloat(yi, "gamma", 1);
	yafaray_setParamMapString(yi, "image_optimization", "optimized");
	yafaray_createImage(yi, "Texture_HDR_image", NULL);
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapString(yi, "image_name", "Texture_HDR_image");
	yafaray_setParamMapString(yi, "type", "image");
	yafaray_setParamMapBool(yi, "adj_clamp", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapFloat(yi, "adj_contrast", 1);
	yafaray_setParamMapFloat(yi, "adj_hue", 0);
	yafaray_setParamMapFloat(yi, "adj_intensity", 1);
	yafaray_setParamMapFloat(yi, "adj_mult_factor_blue", 1);
	yafaray_setParamMapFloat(yi, "adj_mult_factor_green", 1);
	yafaray_setParamMapFloat(yi, "adj_mult_factor_red", 1);
	yafaray_setParamMapFloat(yi, "adj_saturation", 1);
	yafaray_setParamMapBool(yi, "calc_alpha", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapString(yi, "clipping", "repeat");
	yafaray_setParamMapFloat(yi, "cropmax_x", 1);
	yafaray_setParamMapFloat(yi, "cropmax_y", 1);
	yafaray_setParamMapFloat(yi, "cropmin_x", 0);
	yafaray_setParamMapFloat(yi, "cropmin_y", 0);
	yafaray_setParamMapFloat(yi, "ewa_max_anisotropy", 8);
	yafaray_setParamMapString(yi, "interpolate", "bilinear");
	yafaray_setParamMapBool(yi, "mirror_x", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(yi, "mirror_y", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(yi, "normalmap", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(yi, "rot90", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapFloat(yi, "trilinear_level_bias", 0);
	yafaray_setParamMapBool(yi, "use_alpha", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapInt(yi, "xrepeat", 1);
	yafaray_setParamMapInt(yi, "yrepeat", 1);
	yafaray_createTexture(yi, "Texture_HDR");
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapColor(yi, "color", 0.8, 0.8, 0.8, 1);
	yafaray_setParamMapString(yi, "type", "shinydiffusemat");
	yafaray_paramsEndList(yi);
	yafaray_createMaterial(yi, "defaultMat", NULL);
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapFloat(yi, "IOR", 1.8);
	yafaray_setParamMapInt(yi, "additionaldepth", 0);
	yafaray_setParamMapColor(yi, "color", 0.8, 0.8, 0.8, 1);
	yafaray_setParamMapFloat(yi, "diffuse_reflect", 0.5);
	yafaray_setParamMapString(yi, "diffuse_shader", "diff_layer0");
	yafaray_setParamMapFloat(yi, "emit", 0);
	yafaray_setParamMapBool(yi, "flat_material", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(yi, "fresnel_effect", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapInt(yi, "mat_pass_index", 2);
	yafaray_setParamMapColor(yi, "mirror_color", 1, 1, 1, 1);
	yafaray_setParamMapBool(yi, "receive_shadows", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapFloat(yi, "samplingfactor", 1);
	yafaray_setParamMapFloat(yi, "specular_reflect", 0);
	yafaray_setParamMapFloat(yi, "translucency", 0);
	yafaray_setParamMapFloat(yi, "transmit_filter", 1);
	yafaray_setParamMapFloat(yi, "transparency", 0);
	yafaray_setParamMapFloat(yi, "transparentbias_factor", 0);
	yafaray_setParamMapBool(yi, "transparentbias_multiply_raydepth", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapString(yi, "type", "shinydiffusemat");
	yafaray_setParamMapString(yi, "visibility", "normal");
	yafaray_setParamMapFloat(yi, "wireframe_amount", 0);
	yafaray_setParamMapColor(yi, "wireframe_color", 1, 1, 1, 1);
	yafaray_setParamMapFloat(yi, "wireframe_exponent", 0);
	yafaray_setParamMapFloat(yi, "wireframe_thickness", 0.01);
	yafaray_addParamMapToList(param_map_list, param_map);
		yafaray_setParamMapString(yi, "blend_mode", "mix");
		yafaray_setParamMapFloat(yi, "colfac", 1);
		yafaray_setParamMapBool(yi, "color_input", YAFARAY_BOOL_TRUE);
		yafaray_setParamMapColor(yi, "def_col", 1, 0, 1, 1);
		yafaray_setParamMapFloat(yi, "def_val", 1);
		yafaray_setParamMapBool(yi, "do_color", YAFARAY_BOOL_TRUE);
		yafaray_setParamMapBool(yi, "do_scalar", YAFARAY_BOOL_FALSE);
		yafaray_setParamMapString(yi, "element", "shader_node");
		yafaray_setParamMapString(yi, "input", "map0");
		yafaray_setParamMapString(yi, "name", "diff_layer0");
		yafaray_setParamMapBool(yi, "negative", YAFARAY_BOOL_FALSE);
		yafaray_setParamMapBool(yi, "noRGB", YAFARAY_BOOL_FALSE);
		yafaray_setParamMapBool(yi, "stencil", YAFARAY_BOOL_FALSE);
		yafaray_setParamMapString(yi, "type", "layer");
		yafaray_setParamMapColor(yi, "upper_color", 0.8, 0.8, 0.8, 1);
		yafaray_setParamMapFloat(yi, "upper_value", 0);
		yafaray_setParamMapBool(yi, "use_alpha", YAFARAY_BOOL_FALSE);
	yafaray_addParamMapToList(param_map_list, param_map);
		yafaray_setParamMapString(yi, "element", "shader_node");
		yafaray_setParamMapString(yi, "mapping", "cube");
		yafaray_setParamMapString(yi, "name", "map0");
		yafaray_setParamMapVector(yi, "offset", 0, 0, 0);
		yafaray_setParamMapInt(yi, "proj_x", 1);
		yafaray_setParamMapInt(yi, "proj_y", 2);
		yafaray_setParamMapInt(yi, "proj_z", 3);
		yafaray_setParamMapVector(yi, "scale", 1, 1, 1);
		yafaray_setParamMapString(yi, "texco", "orco");
		yafaray_setParamMapString(yi, "texture", "Texture_TGA");
		yafaray_setParamMapString(yi, "type", "texture_mapper");
	yafaray_paramsEndList(yi);
	yafaray_createMaterial(yi, "Material_TGA", NULL);
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapFloat(yi, "IOR", 1.8);
	yafaray_setParamMapInt(yi, "additionaldepth", 0);
	yafaray_setParamMapColor(yi, "color", 0.8, 0.8, 0.8, 1);
	yafaray_setParamMapFloat(yi, "diffuse_reflect", 1);
	yafaray_setParamMapFloat(yi, "emit", 0);
	yafaray_setParamMapBool(yi, "flat_material", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(yi, "fresnel_effect", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapInt(yi, "mat_pass_index", 0);
	yafaray_setParamMapColor(yi, "mirror_color", 1, 1, 1, 1);
	yafaray_setParamMapBool(yi, "receive_shadows", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapFloat(yi, "samplingfactor", 1);
	yafaray_setParamMapFloat(yi, "specular_reflect", 0);
	yafaray_setParamMapFloat(yi, "translucency", 0);
	yafaray_setParamMapFloat(yi, "transmit_filter", 1);
	yafaray_setParamMapFloat(yi, "transparency", 0);
	yafaray_setParamMapFloat(yi, "transparentbias_factor", 0);
	yafaray_setParamMapBool(yi, "transparentbias_multiply_raydepth", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapString(yi, "type", "shinydiffusemat");
	yafaray_setParamMapString(yi, "visibility", "normal");
	yafaray_setParamMapFloat(yi, "wireframe_amount", 0);
	yafaray_setParamMapColor(yi, "wireframe_color", 1, 1, 1, 1);
	yafaray_setParamMapFloat(yi, "wireframe_exponent", 0);
	yafaray_setParamMapFloat(yi, "wireframe_thickness", 0.01);
	yafaray_paramsEndList(yi);
	yafaray_createMaterial(yi, "Material_Gray", NULL);
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapBool(yi, "cast_shadows", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapColor(yi, "color", 1, 1, 1, 1);
	yafaray_setParamMapVector(yi, "from", 5.27648, 4.88993, 8.89514);
	yafaray_setParamMapBool(yi, "light_enabled", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapBool(yi, "photon_only", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapFloat(yi, "power", 72);
	yafaray_setParamMapString(yi, "type", "pointlight");
	yafaray_setParamMapBool(yi, "with_caustic", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapBool(yi, "with_diffuse", YAFARAY_BOOL_TRUE);
	yafaray_createLight(yi, "Point");
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapBool(yi, "has_orco", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapBool(yi, "has_uv", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(yi, "is_base_object", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapInt(yi, "num_faces", 6);
	yafaray_setParamMapInt(yi, "num_vertices", 8);
	yafaray_setParamMapInt(yi, "object_index", 5);
	yafaray_setParamMapString(yi, "type", "mesh");
	yafaray_setParamMapString(yi, "visibility", "normal");
	yafaray_createObject(yi, "Cube_TGA", &object_id);
	yafaray_clearParamMap(param_map);

	yafaray_addVertexWithOrco(yi, object_id, 0.364422, -2.81854, 1.00136e-05, -1, -1, -1);
	yafaray_addVertexWithOrco(yi, object_id, 0.364422, -2.81854, 2.00001, -1, -1, 1);
	yafaray_addVertexWithOrco(yi, object_id, 0.364422, -0.81854, 1.00136e-05, -1, 1, -1);
	yafaray_addVertexWithOrco(yi, object_id, 0.364422, -0.81854, 2.00001, -1, 1, 1);
	yafaray_addVertexWithOrco(yi, object_id, 2.36442, -2.81854, 1.00136e-05, 1, -1, -1);
	yafaray_addVertexWithOrco(yi, object_id, 2.36442, -2.81854, 2.00001, 1, -1, 1);
	yafaray_addVertexWithOrco(yi, object_id, 2.36442, -0.81854, 1.00136e-05, 1, 1, -1);
	yafaray_addVertexWithOrco(yi, object_id, 2.36442, -0.81854, 2.00001, 1, 1, 1);

	yafaray_getMaterialId(yi, "Material_TGA", &material_id);
	yafaray_addQuad(yi, object_id, 2, 0, 1, 3, material_id);
	yafaray_addQuad(yi, object_id, 3, 7, 6, 2, material_id);
	yafaray_addQuad(yi, object_id, 7, 5, 4, 6, material_id);
	yafaray_addQuad(yi, object_id, 0, 4, 5, 1, material_id);
	yafaray_addQuad(yi, object_id, 0, 2, 6, 4, material_id);
	yafaray_addTriangle(yi, object_id, 5, 7, 3, material_id);
	yafaray_addTriangle(yi, object_id, 5, 3, 1, material_id);
	yafaray_initObject(yi, object_id, material_id);
	yafaray_clearParamMap(param_map);


	yafaray_setParamMapBool(yi, "has_orco", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(yi, "has_uv", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(yi, "is_base_object", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapInt(yi, "num_faces", 1);
	yafaray_setParamMapInt(yi, "num_vertices", 4);
	yafaray_setParamMapInt(yi, "object_index", 0);
	yafaray_setParamMapString(yi, "type", "mesh");
	yafaray_setParamMapString(yi, "visibility", "normal");
	yafaray_createObject(yi, "Plane", &object_id);
	yafaray_clearParamMap(param_map);

	yafaray_addVertex(yi, object_id, -10, -10, 0);
	yafaray_addVertex(yi, object_id, 10, -10, 0);
	yafaray_addVertex(yi, object_id, -10, 10, 0);
	yafaray_addVertex(yi, object_id, 10, 10, 0);
	yafaray_getMaterialId(yi, "Material_Gray", &material_id);
	yafaray_addTriangle(yi, object_id, 0, 1, 3, material_id);
	yafaray_addTriangle(yi, object_id, 0, 3, 2, material_id);
	yafaray_initObject(yi, object_id, material_id);
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapFloat(yi, "aperture", 0);
	yafaray_setParamMapFloat(yi, "bokeh_rotation", 0);
	yafaray_setParamMapString(yi, "bokeh_type", "disk1");
	yafaray_setParamMapFloat(yi, "dof_distance", 0);
	yafaray_setParamMapFloat(yi, "focal", 1.09375);
	yafaray_setParamMapVector(yi, "from", 8.64791, -7.22615, 8.1295);
	yafaray_setParamMapInt(yi, "resx", 480);
	yafaray_setParamMapInt(yi, "resy", 270);
	yafaray_setParamMapVector(yi, "to", 8.03447, -6.65603, 7.58301);
	yafaray_setParamMapString(yi, "type", "perspective");
	yafaray_setParamMapVector(yi, "up", 8.25644, -6.8447, 8.9669);
	yafaray_defineCamera(yi, "Camera");
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapString(yi, "camera_name", "Camera");
	yafaray_createRenderView(yi, "");
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapBool(yi, "cast_shadows", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapColor(yi, "color", 0.7, 0.7, 0.7, 1);
	yafaray_setParamMapBool(yi, "ibl", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapInt(yi, "ibl_samples", 3);
	yafaray_setParamMapFloat(yi, "power", 0.5);
	yafaray_setParamMapString(yi, "type", "constant");
	yafaray_setParamMapBool(yi, "with_caustic", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapBool(yi, "with_diffuse", YAFARAY_BOOL_TRUE);
	yafaray_defineBackground(yi);
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapColor(yi, "AO_color", 0.9, 0.9, 0.9, 1);
	yafaray_setParamMapFloat(yi, "AO_distance", 1);
	yafaray_setParamMapInt(yi, "AO_samples", 32);
	yafaray_setParamMapBool(yi, "bg_transp", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(yi, "bg_transp_refract", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(yi, "caustics", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(yi, "do_AO", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapString(yi, "photon_maps_processing", "generate-only");
	yafaray_setParamMapInt(yi, "raydepth", 2);
	yafaray_setParamMapInt(yi, "shadowDepth", 2);
	yafaray_setParamMapBool(yi, "transpShad", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapString(yi, "type", "directlighting");
	yafaray_defineSurfaceIntegrator(yi);
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapString(yi, "exported_image_name", "Combined");
	yafaray_setParamMapString(yi, "exported_image_type", "ColorAlpha");
	yafaray_setParamMapString(yi, "image_type", "ColorAlpha");
	yafaray_setParamMapString(yi, "type", "combined");
	yafaray_defineLayer(yi);
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapInt(yi, "AA_minsamples", 3);
	yafaray_setParamMapInt(yi, "AA_passes", 1);
	yafaray_setParamMapInt(yi, "AA_inc_samples", 1);
	yafaray_setParamMapString(yi, "filter_type", "gauss");
	yafaray_setParamMapFloat(yi, "AA_pixelwidth", 1.5);
	yafaray_setParamMapString(yi, "scene_accelerator", "yafaray-kdtree-original");
	yafaray_setParamMapInt(yi, "threads", -1);
	yafaray_setParamMapInt(yi, "threads_photons", -1);
	yafaray_setParamMapInt(yi, "width", 480);
	yafaray_setParamMapInt(yi, "height", 270);
	yafaray_setParamMapInt(yi, "xstart", 0);
	yafaray_setParamMapInt(yi, "ystart", 0);
	yafaray_setupRender(yi);
	yafaray_clearParamMap(param_map);

	/* Creating image output */
	yafaray_setParamMapString(yi, "image_path", "./test06-output1.tga");
	yafaray_setParamMapString(yi, "color_space", "sRGB");
	yafaray_setParamMapString(yi, "badge_position", "top");
	yafaray_createOutput(yi, "output1_tga");
	yafaray_clearParamMap(param_map);

	yafaray_render(yi, NULL, NULL, YAFARAY_DISPLAY_CONSOLE_NORMAL);

	yafaray_destroyInterface(yi);

	return 0;
}
