/****************************************************************************
 *      This is part of the libYafaRay package
 *
 *      test08.c : scene with shader nodes
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
	yafaray_Interface *yi = yafaray_createInterface(YAFARAY_INTERFACE_FOR_RENDERING, NULL, NULL, NULL, YAFARAY_DISPLAY_CONSOLE_NORMAL);
	yafaray_setConsoleLogColorsEnabled(yi, YAFARAY_BOOL_TRUE);
	yafaray_setConsoleVerbosityLevel(yi, YAFARAY_LOG_LEVEL_DEBUG);

	yafaray_createScene(yi);
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetString(yi, "color_space", "sRGB");
	yafaray_paramsSetString(yi, "filename", "tex.tga");
	yafaray_paramsSetFloat(yi, "gamma", 1);
	yafaray_paramsSetString(yi, "image_optimization", "optimized");
	yafaray_createImage(yi, "Texture_TGA_image", NULL);
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetString(yi, "image_name", "Texture_TGA_image");
	yafaray_paramsSetString(yi, "type", "image");
	yafaray_paramsSetBool(yi, "adj_clamp", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetFloat(yi, "adj_contrast", 1);
	yafaray_paramsSetFloat(yi, "adj_hue", 0);
	yafaray_paramsSetFloat(yi, "adj_intensity", 1);
	yafaray_paramsSetFloat(yi, "adj_mult_factor_blue", 1);
	yafaray_paramsSetFloat(yi, "adj_mult_factor_green", 1);
	yafaray_paramsSetFloat(yi, "adj_mult_factor_red", 1);
	yafaray_paramsSetFloat(yi, "adj_saturation", 1);
	yafaray_paramsSetBool(yi, "calc_alpha", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetString(yi, "clipping", "repeat");
	yafaray_paramsSetFloat(yi, "cropmax_x", 1);
	yafaray_paramsSetFloat(yi, "cropmax_y", 1);
	yafaray_paramsSetFloat(yi, "cropmin_x", 0);
	yafaray_paramsSetFloat(yi, "cropmin_y", 0);
	yafaray_paramsSetFloat(yi, "ewa_max_anisotropy", 8);
	yafaray_paramsSetString(yi, "interpolate", "bilinear");
	yafaray_paramsSetBool(yi, "mirror_x", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "mirror_y", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "normalmap", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "rot90", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetFloat(yi, "trilinear_level_bias", 0);
	yafaray_paramsSetBool(yi, "use_alpha", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetInt(yi, "xrepeat", 1);
	yafaray_paramsSetInt(yi, "yrepeat", 1);
	yafaray_createTexture(yi, "Texture_TGA");
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetString(yi, "color_space", "LinearRGB");
	yafaray_paramsSetString(yi, "filename", "tex.hdr");
	yafaray_paramsSetFloat(yi, "gamma", 1);
	yafaray_paramsSetString(yi, "image_optimization", "optimized");
	yafaray_createImage(yi, "Texture_HDR_image", NULL);
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetString(yi, "image_name", "Texture_HDR_image");
	yafaray_paramsSetString(yi, "type", "image");
	yafaray_paramsSetBool(yi, "adj_clamp", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetFloat(yi, "adj_contrast", 1);
	yafaray_paramsSetFloat(yi, "adj_hue", 0);
	yafaray_paramsSetFloat(yi, "adj_intensity", 1);
	yafaray_paramsSetFloat(yi, "adj_mult_factor_blue", 1);
	yafaray_paramsSetFloat(yi, "adj_mult_factor_green", 1);
	yafaray_paramsSetFloat(yi, "adj_mult_factor_red", 1);
	yafaray_paramsSetFloat(yi, "adj_saturation", 1);
	yafaray_paramsSetBool(yi, "calc_alpha", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetString(yi, "clipping", "repeat");
	yafaray_paramsSetFloat(yi, "cropmax_x", 1);
	yafaray_paramsSetFloat(yi, "cropmax_y", 1);
	yafaray_paramsSetFloat(yi, "cropmin_x", 0);
	yafaray_paramsSetFloat(yi, "cropmin_y", 0);
	yafaray_paramsSetFloat(yi, "ewa_max_anisotropy", 8);
	yafaray_paramsSetString(yi, "interpolate", "bilinear");
	yafaray_paramsSetBool(yi, "mirror_x", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "mirror_y", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "normalmap", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "rot90", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetFloat(yi, "trilinear_level_bias", 0);
	yafaray_paramsSetBool(yi, "use_alpha", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetInt(yi, "xrepeat", 1);
	yafaray_paramsSetInt(yi, "yrepeat", 1);
	yafaray_createTexture(yi, "Texture_HDR");
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetColor(yi, "color", 0.8, 0.8, 0.8, 1);
	yafaray_paramsSetString(yi, "type", "shinydiffusemat");
	yafaray_paramsEndList(yi);
	yafaray_createMaterial(yi, "defaultMat", NULL);
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetFloat(yi, "IOR", 1.8);
	yafaray_paramsSetInt(yi, "additionaldepth", 0);
	yafaray_paramsSetColor(yi, "color", 0.8, 0.8, 0.8, 1);
	yafaray_paramsSetFloat(yi, "diffuse_reflect", 0.5);
	yafaray_paramsSetString(yi, "diffuse_shader", "diff_layer0");
	yafaray_paramsSetFloat(yi, "emit", 0);
	yafaray_paramsSetBool(yi, "flat_material", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "fresnel_effect", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetInt(yi, "mat_pass_index", 2);
	yafaray_paramsSetColor(yi, "mirror_color", 1, 1, 1, 1);
	yafaray_paramsSetBool(yi, "receive_shadows", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetFloat(yi, "samplingfactor", 1);
	yafaray_paramsSetFloat(yi, "specular_reflect", 0);
	yafaray_paramsSetFloat(yi, "translucency", 0);
	yafaray_paramsSetFloat(yi, "transmit_filter", 1);
	yafaray_paramsSetFloat(yi, "transparency", 0);
	yafaray_paramsSetFloat(yi, "transparentbias_factor", 0);
	yafaray_paramsSetBool(yi, "transparentbias_multiply_raydepth", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetString(yi, "type", "shinydiffusemat");
	yafaray_paramsSetString(yi, "visibility", "normal");
	yafaray_paramsSetFloat(yi, "wireframe_amount", 0);
	yafaray_paramsSetColor(yi, "wireframe_color", 1, 1, 1, 1);
	yafaray_paramsSetFloat(yi, "wireframe_exponent", 0);
	yafaray_paramsSetFloat(yi, "wireframe_thickness", 0.01);
	yafaray_paramsPushList(yi);
		yafaray_paramsSetString(yi, "element", "shader_node");
		yafaray_paramsSetString(yi, "mapping", "cube");
		yafaray_paramsSetString(yi, "name", "node_tga");
		yafaray_paramsSetVector(yi, "offset", 0, 0, 0);
		yafaray_paramsSetInt(yi, "proj_x", 1);
		yafaray_paramsSetInt(yi, "proj_y", 2);
		yafaray_paramsSetInt(yi, "proj_z", 3);
		yafaray_paramsSetVector(yi, "scale", 1, 1, 1);
		yafaray_paramsSetString(yi, "texco", "orco");
		yafaray_paramsSetString(yi, "texture", "Texture_TGA");
		yafaray_paramsSetString(yi, "type", "texture_mapper");
	yafaray_paramsPushList(yi);
		yafaray_paramsSetString(yi, "element", "shader_node");
		yafaray_paramsSetString(yi, "mapping", "cube");
		yafaray_paramsSetString(yi, "name", "node_hdr");
		yafaray_paramsSetVector(yi, "offset", 0, 0, 0);
		yafaray_paramsSetInt(yi, "proj_x", 1);
		yafaray_paramsSetInt(yi, "proj_y", 2);
		yafaray_paramsSetInt(yi, "proj_z", 3);
		yafaray_paramsSetVector(yi, "scale", 1, 1, 1);
		yafaray_paramsSetString(yi, "texco", "orco");
		yafaray_paramsSetString(yi, "texture", "Texture_HDR");
		yafaray_paramsSetString(yi, "type", "texture_mapper");
	yafaray_paramsPushList(yi);
		yafaray_paramsSetString(yi, "element", "shader_node");
		yafaray_paramsSetString(yi, "name", "node_mix");
		yafaray_paramsSetString(yi, "input1", "node_tga");
		yafaray_paramsSetString(yi, "input2", "node_hdr");
		yafaray_paramsSetFloat(yi, "cfactor", 0.8);
		yafaray_paramsSetString(yi, "blend_mode", "add");
		yafaray_paramsSetString(yi, "type", "mix");
	yafaray_paramsPushList(yi);
		yafaray_paramsSetString(yi, "blend_mode", "mix");
		yafaray_paramsSetFloat(yi, "colfac", 1);
		yafaray_paramsSetBool(yi, "color_input", YAFARAY_BOOL_TRUE);
		yafaray_paramsSetColor(yi, "def_col", 1, 0, 1, 1);
		yafaray_paramsSetFloat(yi, "def_val", 1);
		yafaray_paramsSetBool(yi, "do_color", YAFARAY_BOOL_TRUE);
		yafaray_paramsSetBool(yi, "do_scalar", YAFARAY_BOOL_FALSE);
		yafaray_paramsSetString(yi, "element", "shader_node");
		yafaray_paramsSetString(yi, "input", "node_mix");
		yafaray_paramsSetString(yi, "name", "diff_layer0");
		yafaray_paramsSetBool(yi, "negative", YAFARAY_BOOL_FALSE);
		yafaray_paramsSetBool(yi, "noRGB", YAFARAY_BOOL_FALSE);
		yafaray_paramsSetBool(yi, "stencil", YAFARAY_BOOL_FALSE);
		yafaray_paramsSetString(yi, "type", "layer");
		yafaray_paramsSetColor(yi, "upper_color", 0.8, 0.8, 0.8, 1);
		yafaray_paramsSetFloat(yi, "upper_value", 0);
		yafaray_paramsSetBool(yi, "use_alpha", YAFARAY_BOOL_FALSE);
	yafaray_paramsEndList(yi);
	yafaray_createMaterial(yi, "Material_TGA", NULL);
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetFloat(yi, "IOR", 1.8);
	yafaray_paramsSetInt(yi, "additionaldepth", 0);
	yafaray_paramsSetColor(yi, "color", 0.8, 0.8, 0.8, 1);
	yafaray_paramsSetFloat(yi, "diffuse_reflect", 1);
	yafaray_paramsSetFloat(yi, "emit", 0);
	yafaray_paramsSetBool(yi, "flat_material", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "fresnel_effect", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetInt(yi, "mat_pass_index", 0);
	yafaray_paramsSetColor(yi, "mirror_color", 1, 1, 1, 1);
	yafaray_paramsSetBool(yi, "receive_shadows", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetFloat(yi, "samplingfactor", 1);
	yafaray_paramsSetFloat(yi, "specular_reflect", 0);
	yafaray_paramsSetFloat(yi, "translucency", 0);
	yafaray_paramsSetFloat(yi, "transmit_filter", 1);
	yafaray_paramsSetFloat(yi, "transparency", 0);
	yafaray_paramsSetFloat(yi, "transparentbias_factor", 0);
	yafaray_paramsSetBool(yi, "transparentbias_multiply_raydepth", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetString(yi, "type", "shinydiffusemat");
	yafaray_paramsSetString(yi, "visibility", "normal");
	yafaray_paramsSetFloat(yi, "wireframe_amount", 0);
	yafaray_paramsSetColor(yi, "wireframe_color", 1, 1, 1, 1);
	yafaray_paramsSetFloat(yi, "wireframe_exponent", 0);
	yafaray_paramsSetFloat(yi, "wireframe_thickness", 0.01);
	yafaray_paramsEndList(yi);
	yafaray_createMaterial(yi, "Material_Gray", NULL);
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetBool(yi, "cast_shadows", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetColor(yi, "color", 1, 1, 1, 1);
	yafaray_paramsSetVector(yi, "from", 5.27648, 4.88993, 8.89514);
	yafaray_paramsSetBool(yi, "light_enabled", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetBool(yi, "photon_only", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetFloat(yi, "power", 72);
	yafaray_paramsSetString(yi, "type", "pointlight");
	yafaray_paramsSetBool(yi, "with_caustic", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetBool(yi, "with_diffuse", YAFARAY_BOOL_TRUE);
	yafaray_createLight(yi, "Point");
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetBool(yi, "has_orco", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetBool(yi, "has_uv", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "is_base_object", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetInt(yi, "num_faces", 6);
	yafaray_paramsSetInt(yi, "num_vertices", 8);
	yafaray_paramsSetInt(yi, "object_index", 5);
	yafaray_paramsSetString(yi, "type", "mesh");
	yafaray_paramsSetString(yi, "visibility", "normal");
	yafaray_createObject(yi, "Cube_TGA", &object_id);
	yafaray_paramsClearAll(yi);

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
	yafaray_paramsClearAll(yi);


	yafaray_paramsSetBool(yi, "has_orco", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "has_uv", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "is_base_object", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetInt(yi, "num_faces", 1);
	yafaray_paramsSetInt(yi, "num_vertices", 4);
	yafaray_paramsSetInt(yi, "object_index", 0);
	yafaray_paramsSetString(yi, "type", "mesh");
	yafaray_paramsSetString(yi, "visibility", "normal");
	yafaray_createObject(yi, "Plane", &object_id);
	yafaray_paramsClearAll(yi);

	yafaray_addVertex(yi, object_id, -10, -10, 0);
	yafaray_addVertex(yi, object_id, 10, -10, 0);
	yafaray_addVertex(yi, object_id, -10, 10, 0);
	yafaray_addVertex(yi, object_id, 10, 10, 0);
	yafaray_getMaterialId(yi, "Material_Gray", &material_id);
	yafaray_addTriangle(yi, object_id, 0, 1, 3, material_id);
	yafaray_addTriangle(yi, object_id, 0, 3, 2, material_id);
	yafaray_initObject(yi, object_id, material_id);
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetFloat(yi, "aperture", 0);
	yafaray_paramsSetFloat(yi, "bokeh_rotation", 0);
	yafaray_paramsSetString(yi, "bokeh_type", "disk1");
	yafaray_paramsSetFloat(yi, "dof_distance", 0);
	yafaray_paramsSetFloat(yi, "focal", 1.09375);
	yafaray_paramsSetVector(yi, "from", 8.64791, -7.22615, 8.1295);
	yafaray_paramsSetInt(yi, "resx", 480);
	yafaray_paramsSetInt(yi, "resy", 270);
	yafaray_paramsSetVector(yi, "to", 8.03447, -6.65603, 7.58301);
	yafaray_paramsSetString(yi, "type", "perspective");
	yafaray_paramsSetVector(yi, "up", 8.25644, -6.8447, 8.9669);
	yafaray_createCamera(yi, "Camera");
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetString(yi, "camera_name", "Camera");
	yafaray_createRenderView(yi, "");
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetBool(yi, "cast_shadows", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetColor(yi, "color", 0.7, 0.7, 0.7, 1);
	yafaray_paramsSetBool(yi, "ibl", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetInt(yi, "ibl_samples", 3);
	yafaray_paramsSetFloat(yi, "power", 0.5);
	yafaray_paramsSetString(yi, "type", "constant");
	yafaray_paramsSetBool(yi, "with_caustic", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetBool(yi, "with_diffuse", YAFARAY_BOOL_TRUE);
	yafaray_defineBackground(yi);
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetColor(yi, "AO_color", 0.9, 0.9, 0.9, 1);
	yafaray_paramsSetFloat(yi, "AO_distance", 1);
	yafaray_paramsSetInt(yi, "AO_samples", 32);
	yafaray_paramsSetBool(yi, "bg_transp", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "bg_transp_refract", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "caustics", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "do_AO", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetString(yi, "photon_maps_processing", "generate-only");
	yafaray_paramsSetInt(yi, "raydepth", 2);
	yafaray_paramsSetInt(yi, "shadowDepth", 2);
	yafaray_paramsSetBool(yi, "transpShad", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetString(yi, "type", "directlighting");
	yafaray_defineSurfaceIntegrator(yi);
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetString(yi, "exported_image_name", "Combined");
	yafaray_paramsSetString(yi, "exported_image_type", "ColorAlpha");
	yafaray_paramsSetString(yi, "image_type", "ColorAlpha");
	yafaray_paramsSetString(yi, "type", "combined");
	yafaray_defineLayer(yi);
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetInt(yi, "AA_minsamples", 3);
	yafaray_paramsSetInt(yi, "AA_passes", 1);
	yafaray_paramsSetInt(yi, "AA_inc_samples", 1);
	yafaray_paramsSetString(yi, "filter_type", "gauss");
	yafaray_paramsSetFloat(yi, "AA_pixelwidth", 1.5);
	yafaray_paramsSetString(yi, "scene_accelerator", "yafaray-kdtree-original");
	yafaray_paramsSetInt(yi, "threads", -1);
	yafaray_paramsSetInt(yi, "threads_photons", -1);
	yafaray_paramsSetInt(yi, "width", 480);
	yafaray_paramsSetInt(yi, "height", 270);
	yafaray_paramsSetInt(yi, "xstart", 0);
	yafaray_paramsSetInt(yi, "ystart", 0);
	yafaray_setupRender(yi);
	yafaray_paramsClearAll(yi);

	/* Creating image output */
	yafaray_paramsSetString(yi, "image_path", "./test08-output1.tga");
	yafaray_paramsSetString(yi, "color_space", "sRGB");
	yafaray_paramsSetString(yi, "badge_position", "top");
	yafaray_createOutput(yi, "output1_tga");
	yafaray_paramsClearAll(yi);

	yafaray_render(yi, NULL, NULL, YAFARAY_DISPLAY_CONSOLE_NORMAL);

	yafaray_destroyInterface(yi);

	return 0;
}
