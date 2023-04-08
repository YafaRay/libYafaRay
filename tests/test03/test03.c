/****************************************************************************
 *      This is part of the libYafaRay package
 *
 *      test03.c : material nodes test scene
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

	yafaray_setParamMapString(yi, "type", "yafaray");
	yafaray_createScene(yi);
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(yi, "color_space", "sRGB");
	yafaray_setParamMapString(yi, "filename", "tex.hdr");
	yafaray_setParamMapFloat(yi, "gamma", 1);
	yafaray_setParamMapString(yi, "image_optimization", "optimized");
	yafaray_createImage(yi, "Tex_image", NULL);
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapString(yi, "image_name", "Tex_image");
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
	yafaray_createTexture(yi, "Tex");
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapBool(yi, "adj_clamp", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapFloat(yi, "adj_contrast", 1);
	yafaray_setParamMapFloat(yi, "adj_hue", 0);
	yafaray_setParamMapFloat(yi, "adj_intensity", 1);
	yafaray_setParamMapFloat(yi, "adj_mult_factor_blue", 1);
	yafaray_setParamMapFloat(yi, "adj_mult_factor_green", 1);
	yafaray_setParamMapFloat(yi, "adj_mult_factor_red", 1);
	yafaray_setParamMapFloat(yi, "adj_saturation", 1);
	yafaray_setParamMapInt(yi, "depth", 0);
	yafaray_setParamMapBool(yi, "hard", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapString(yi, "noise_type", "blender");
	yafaray_setParamMapString(yi, "shape", "sin");
	yafaray_setParamMapFloat(yi, "size", 0.25);
	yafaray_setParamMapFloat(yi, "turbulence", 0);
	yafaray_setParamMapString(yi, "type", "wood");
	yafaray_setParamMapString(yi, "wood_type", "bands");
	yafaray_createTexture(yi, "Texture");
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapString(yi, "color_space", "sRGB");
	yafaray_setParamMapString(yi, "filename", "tex.tga");
	yafaray_setParamMapFloat(yi, "gamma", 1);
	yafaray_setParamMapString(yi, "image_optimization", "optimized");
	yafaray_createImage(yi, "Texture.001_image", NULL);
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapString(yi, "image_name", "Texture.001_image");
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
	yafaray_createTexture(yi, "Texture.001");
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapColor(yi, "color", 0.8, 0.8, 0.8, 1);
	yafaray_setParamMapString(yi, "type", "shinydiffusemat");
	yafaray_paramsEndList(yi);
	yafaray_createMaterial(yi, "defaultMat", NULL);
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapFloat(yi, "IOR", 1.52);
	yafaray_setParamMapColor(yi, "absorption", 1, 1, 1, 1);
	yafaray_setParamMapFloat(yi, "absorption_dist", 1);
	yafaray_setParamMapInt(yi, "additionaldepth", 0);
	yafaray_setParamMapFloat(yi, "dispersion_power", 0);
	yafaray_setParamMapBool(yi, "fake_shadows", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapColor(yi, "filter_color", 1, 1, 1, 1);
	yafaray_setParamMapBool(yi, "flat_material", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapInt(yi, "mat_pass_index", 0);
	yafaray_setParamMapColor(yi, "mirror_color", 1, 1, 1, 1);
	yafaray_setParamMapBool(yi, "receive_shadows", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapFloat(yi, "samplingfactor", 1);
	yafaray_setParamMapFloat(yi, "transmit_filter", 1);
	yafaray_setParamMapString(yi, "type", "glass");
	yafaray_setParamMapString(yi, "visibility", "normal");
	yafaray_setParamMapFloat(yi, "wireframe_amount", 0);
	yafaray_setParamMapColor(yi, "wireframe_color", 1, 1, 1, 1);
	yafaray_setParamMapFloat(yi, "wireframe_exponent", 0);
	yafaray_setParamMapFloat(yi, "wireframe_thickness", 0.01);
	yafaray_paramsEndList(yi);
	yafaray_createMaterial(yi, "Mat1", NULL);
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapFloat(yi, "IOR", 1.8);
	yafaray_setParamMapInt(yi, "additionaldepth", 0);
	yafaray_setParamMapColor(yi, "color", 0.8, 0.8, 0.8, 1);
	yafaray_setParamMapFloat(yi, "diffuse_reflect", 1);
	yafaray_setParamMapString(yi, "diffuse_shader", "diff_layer0");
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
	yafaray_setParamMapString(yi, "mapping", "plain");
	yafaray_setParamMapString(yi, "name", "map0");
	yafaray_setParamMapVector(yi, "offset", 0, 0, 0);
	yafaray_setParamMapInt(yi, "proj_x", 1);
	yafaray_setParamMapInt(yi, "proj_y", 2);
	yafaray_setParamMapInt(yi, "proj_z", 3);
	yafaray_setParamMapVector(yi, "scale", 1, 1, 1);
	yafaray_setParamMapString(yi, "texco", "orco");
	yafaray_setParamMapString(yi, "texture", "Tex");
	yafaray_setParamMapString(yi, "type", "texture_mapper");
	yafaray_paramsEndList(yi);
	yafaray_createMaterial(yi, "diffuse2", NULL);
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapFloat(yi, "IOR", 1.8);
	yafaray_setParamMapInt(yi, "additionaldepth", 0);
	yafaray_setParamMapColor(yi, "color", 0.8, 0.8, 0.8, 1);
	yafaray_setParamMapFloat(yi, "diffuse_reflect", 1);
	yafaray_setParamMapString(yi, "diffuse_shader", "diff_layer2");
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
	yafaray_setParamMapString(yi, "texture", "Tex");
	yafaray_setParamMapString(yi, "type", "texture_mapper");
	yafaray_addParamMapToList(param_map_list, param_map);
	yafaray_setParamMapString(yi, "blend_mode", "mix");
	yafaray_setParamMapFloat(yi, "colfac", 1);
	yafaray_setParamMapBool(yi, "color_input", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapColor(yi, "def_col", 1, 0, 1, 1);
	yafaray_setParamMapFloat(yi, "def_val", 1);
	yafaray_setParamMapBool(yi, "do_color", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapBool(yi, "do_scalar", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapString(yi, "element", "shader_node");
	yafaray_setParamMapString(yi, "input", "map1");
	yafaray_setParamMapString(yi, "name", "diff_layer1");
	yafaray_setParamMapBool(yi, "negative", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(yi, "noRGB", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(yi, "stencil", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapString(yi, "type", "layer");
	yafaray_setParamMapString(yi, "upper_layer", "diff_layer0");
	yafaray_setParamMapBool(yi, "use_alpha", YAFARAY_BOOL_FALSE);
	yafaray_addParamMapToList(param_map_list, param_map);
	yafaray_setParamMapString(yi, "element", "shader_node");
	yafaray_setParamMapString(yi, "mapping", "cube");
	yafaray_setParamMapString(yi, "name", "map1");
	yafaray_setParamMapVector(yi, "offset", 0, 0, 0);
	yafaray_setParamMapInt(yi, "proj_x", 1);
	yafaray_setParamMapInt(yi, "proj_y", 2);
	yafaray_setParamMapInt(yi, "proj_z", 3);
	yafaray_setParamMapVector(yi, "scale", 1, 1, 1);
	yafaray_setParamMapString(yi, "texco", "orco");
	yafaray_setParamMapString(yi, "texture", "Texture");
	yafaray_setParamMapString(yi, "type", "texture_mapper");
	yafaray_addParamMapToList(param_map_list, param_map);
	yafaray_setParamMapString(yi, "blend_mode", "mix");
	yafaray_setParamMapFloat(yi, "colfac", 1);
	yafaray_setParamMapBool(yi, "color_input", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapColor(yi, "def_col", 1, 0, 1, 1);
	yafaray_setParamMapFloat(yi, "def_val", 1);
	yafaray_setParamMapBool(yi, "do_color", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapBool(yi, "do_scalar", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapString(yi, "element", "shader_node");
	yafaray_setParamMapString(yi, "input", "map2");
	yafaray_setParamMapString(yi, "name", "diff_layer2");
	yafaray_setParamMapBool(yi, "negative", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(yi, "noRGB", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(yi, "stencil", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapString(yi, "type", "layer");
	yafaray_setParamMapString(yi, "upper_layer", "diff_layer1");
	yafaray_setParamMapBool(yi, "use_alpha", YAFARAY_BOOL_FALSE);
	yafaray_addParamMapToList(param_map_list, param_map);
	yafaray_setParamMapString(yi, "element", "shader_node");
	yafaray_setParamMapString(yi, "mapping", "cube");
	yafaray_setParamMapString(yi, "name", "map2");
	yafaray_setParamMapVector(yi, "offset", 0, 0, 0);
	yafaray_setParamMapInt(yi, "proj_x", 1);
	yafaray_setParamMapInt(yi, "proj_y", 2);
	yafaray_setParamMapInt(yi, "proj_z", 3);
	yafaray_setParamMapVector(yi, "scale", 1, 1, 1);
	yafaray_setParamMapString(yi, "texco", "orco");
	yafaray_setParamMapString(yi, "texture", "Texture.001");
	yafaray_setParamMapString(yi, "type", "texture_mapper");
	yafaray_paramsEndList(yi);
	yafaray_createMaterial(yi, "Mat2", NULL);
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapInt(yi, "additionaldepth", 0);
	yafaray_setParamMapFloat(yi, "blend_value", 0);
	yafaray_setParamMapBool(yi, "flat_material", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapString(yi, "material1", "Mat1");
	yafaray_setParamMapString(yi, "material2", "Mat2");
	yafaray_setParamMapBool(yi, "receive_shadows", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapFloat(yi, "samplingfactor", 1);
	yafaray_setParamMapString(yi, "type", "blend_mat");
	yafaray_setParamMapString(yi, "visibility", "normal");
	yafaray_setParamMapFloat(yi, "wireframe_amount", 0);
	yafaray_setParamMapColor(yi, "wireframe_color", 1, 1, 1, 1);
	yafaray_setParamMapFloat(yi, "wireframe_exponent", 0);
	yafaray_setParamMapFloat(yi, "wireframe_thickness", 0.01);
	yafaray_paramsEndList(yi);
	yafaray_createMaterial(yi, "blend", NULL);
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
	yafaray_createMaterial(yi, "diffuse", NULL);
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapBool(yi, "cast_shadows", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapColor(yi, "color", 1, 1, 1, 1);
	yafaray_setParamMapVector(yi, "from", -4.44722, 1.00545, 5.90386);
	yafaray_setParamMapBool(yi, "light_enabled", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapBool(yi, "photon_only", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapFloat(yi, "power", 50);
	yafaray_setParamMapString(yi, "type", "pointlight");
	yafaray_setParamMapBool(yi, "with_caustic", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapBool(yi, "with_diffuse", YAFARAY_BOOL_TRUE);
	yafaray_createLight(yi, "Lamp.001");
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapBool(yi, "cast_shadows", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapColor(yi, "color", 1, 1, 1, 1);
	yafaray_setParamMapVector(yi, "from", 4.07625, 1.00545, 5.90386);
	yafaray_setParamMapBool(yi, "light_enabled", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapBool(yi, "photon_only", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapFloat(yi, "power", 50);
	yafaray_setParamMapString(yi, "type", "pointlight");
	yafaray_setParamMapBool(yi, "with_caustic", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapBool(yi, "with_diffuse", YAFARAY_BOOL_TRUE);
	yafaray_createLight(yi, "Lamp");
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapBool(yi, "has_orco", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapBool(yi, "has_uv", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(yi, "is_base_object", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapInt(yi, "num_faces", 6);
	yafaray_setParamMapInt(yi, "num_vertices", 8);
	yafaray_setParamMapInt(yi, "object_index", 0);
	yafaray_setParamMapString(yi, "type", "mesh");
	yafaray_setParamMapString(yi, "visibility", "normal");
	yafaray_createObject(yi, "Cube.002", &object_id);
	yafaray_clearParamMap(param_map);

	yafaray_addVertexWithOrco(yi, object_id, 1, 4, 1.00136e-05, 1, 1, -1);
	yafaray_addVertexWithOrco(yi, object_id, 1, 2, 1.00136e-05, 1, -0.999999, -1);
	yafaray_addVertexWithOrco(yi, object_id, -1, 2, 1.00136e-05, -1, -0.999999, -1);
	yafaray_addVertexWithOrco(yi, object_id, -1, 4, 1.00136e-05, -0.999999, 1, -1);
	yafaray_addVertexWithOrco(yi, object_id, 1, 4, 2.00001, 1, 0.999999, 1);
	yafaray_addVertexWithOrco(yi, object_id, 0.999999, 2, 2.00001, 0.999999, -1, 1);
	yafaray_addVertexWithOrco(yi, object_id, -1, 2, 2.00001, -1, -0.999999, 1);
	yafaray_addVertexWithOrco(yi, object_id, -1, 4, 2.00001, -1, 1, 1);
	yafaray_getMaterialId(yi, "Mat1", &material_id);
	yafaray_addTriangle(yi, object_id, 0, 1, 2, material_id);
	yafaray_addTriangle(yi, object_id, 0, 2, 3, material_id);
	yafaray_addTriangle(yi, object_id, 4, 7, 6, material_id);
	yafaray_addTriangle(yi, object_id, 4, 6, 5, material_id);
	yafaray_addTriangle(yi, object_id, 0, 4, 5, material_id);
	yafaray_addTriangle(yi, object_id, 0, 5, 1, material_id);
	yafaray_addTriangle(yi, object_id, 1, 5, 6, material_id);
	yafaray_addTriangle(yi, object_id, 1, 6, 2, material_id);
	yafaray_addTriangle(yi, object_id, 2, 6, 7, material_id);
	yafaray_addTriangle(yi, object_id, 2, 7, 3, material_id);
	yafaray_addTriangle(yi, object_id, 4, 0, 3, material_id);
	yafaray_addTriangle(yi, object_id, 4, 3, 7, material_id);
	yafaray_initObject(yi, object_id, material_id);
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapBool(yi, "has_orco", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapBool(yi, "has_uv", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(yi, "is_base_object", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapInt(yi, "num_faces", 6);
	yafaray_setParamMapInt(yi, "num_vertices", 8);
	yafaray_setParamMapInt(yi, "object_index", 0);
	yafaray_setParamMapString(yi, "type", "mesh");
	yafaray_setParamMapString(yi, "visibility", "normal");
	yafaray_createObject(yi, "Cube.001", &object_id);
	yafaray_clearParamMap(param_map);

	yafaray_addVertexWithOrco(yi, object_id, 1, -2, 1.00136e-05, 1, 1, -1);
	yafaray_addVertexWithOrco(yi, object_id, 1, -4, 1.00136e-05, 1, -0.999999, -1);
	yafaray_addVertexWithOrco(yi, object_id, -1, -4, 1.00136e-05, -1, -0.999999, -1);
	yafaray_addVertexWithOrco(yi, object_id, -1, -2, 1.00136e-05, -0.999999, 1, -1);
	yafaray_addVertexWithOrco(yi, object_id, 1, -2, 2.00001, 1, 0.999999, 1);
	yafaray_addVertexWithOrco(yi, object_id, 0.999999, -4, 2.00001, 0.999999, -1, 1);
	yafaray_addVertexWithOrco(yi, object_id, -1, -4, 2.00001, -1, -0.999999, 1);
	yafaray_addVertexWithOrco(yi, object_id, -1, -2, 2.00001, -1, 1, 1);
	yafaray_getMaterialId(yi, "blend", &material_id);
	yafaray_addTriangle(yi, object_id, 0, 1, 2, material_id);
	yafaray_addTriangle(yi, object_id, 0, 2, 3, material_id);
	yafaray_addTriangle(yi, object_id, 4, 7, 6, material_id);
	yafaray_addTriangle(yi, object_id, 4, 6, 5, material_id);
	yafaray_addTriangle(yi, object_id, 0, 4, 5, material_id);
	yafaray_addTriangle(yi, object_id, 0, 5, 1, material_id);
	yafaray_addTriangle(yi, object_id, 1, 5, 6, material_id);
	yafaray_addTriangle(yi, object_id, 1, 6, 2, material_id);
	yafaray_addTriangle(yi, object_id, 2, 6, 7, material_id);
	yafaray_addTriangle(yi, object_id, 2, 7, 3, material_id);
	yafaray_addTriangle(yi, object_id, 4, 0, 3, material_id);
	yafaray_addTriangle(yi, object_id, 4, 3, 7, material_id);
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
	yafaray_getMaterialId(yi, "diffuse", &material_id);
	yafaray_addTriangle(yi, object_id, 0, 1, 3, material_id);
	yafaray_addTriangle(yi, object_id, 0, 3, 2, material_id);
	yafaray_initObject(yi, object_id, material_id);
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapBool(yi, "has_orco", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapBool(yi, "has_uv", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapBool(yi, "is_base_object", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapInt(yi, "num_faces", 6);
	yafaray_setParamMapInt(yi, "num_vertices", 8);
	yafaray_setParamMapInt(yi, "object_index", 0);
	yafaray_setParamMapString(yi, "type", "mesh");
	yafaray_setParamMapString(yi, "visibility", "normal");
	yafaray_createObject(yi, "Cube", &object_id);
	yafaray_clearParamMap(param_map);

	yafaray_addVertexWithOrco(yi, object_id, 1, 1, 1.00136e-05, 1, 1, -1);
	yafaray_addVertexWithOrco(yi, object_id, 1, -1, 1.00136e-05, 1, -0.999999, -1);
	yafaray_addVertexWithOrco(yi, object_id, -1, -1, 1.00136e-05, -1, -0.999999, -1);
	yafaray_addVertexWithOrco(yi, object_id, -1, 1, 1.00136e-05, -0.999999, 1, -1);
	yafaray_addVertexWithOrco(yi, object_id, 1, 0.999999, 2.00001, 1, 0.999999, 1);
	yafaray_addVertexWithOrco(yi, object_id, 0.999999, -1, 2.00001, 0.999999, -1, 1);
	yafaray_addVertexWithOrco(yi, object_id, -1, -1, 2.00001, -1, -0.999999, 1);
	yafaray_addVertexWithOrco(yi, object_id, -1, 1, 2.00001, -1, 1, 1);
	yafaray_getMaterialId(yi, "Mat2", &material_id);
	yafaray_addTriangle(yi, object_id, 0, 1, 2, material_id);
	yafaray_addTriangle(yi, object_id, 0, 2, 3, material_id);
	yafaray_addTriangle(yi, object_id, 4, 7, 6, material_id);
	yafaray_addTriangle(yi, object_id, 4, 6, 5, material_id);
	yafaray_addTriangle(yi, object_id, 0, 4, 5, material_id);
	yafaray_addTriangle(yi, object_id, 0, 5, 1, material_id);
	yafaray_addTriangle(yi, object_id, 1, 5, 6, material_id);
	yafaray_addTriangle(yi, object_id, 1, 6, 2, material_id);
	yafaray_addTriangle(yi, object_id, 2, 6, 7, material_id);
	yafaray_addTriangle(yi, object_id, 2, 7, 3, material_id);
	yafaray_addTriangle(yi, object_id, 4, 0, 3, material_id);
	yafaray_addTriangle(yi, object_id, 4, 3, 7, material_id);
	yafaray_initObject(yi, object_id, material_id);
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapFloat(yi, "aperture", 0);
	yafaray_setParamMapFloat(yi, "bokeh_rotation", 0);
	yafaray_setParamMapString(yi, "bokeh_type", "disk1");
	yafaray_setParamMapFloat(yi, "dof_distance", 0);
	yafaray_setParamMapFloat(yi, "focal", 1.09375);
	yafaray_setParamMapVector(yi, "from", 7.48113, -6.50764, 5.34367);
	yafaray_setParamMapInt(yi, "resx", 768);
	yafaray_setParamMapInt(yi, "resy", 432);
	yafaray_setParamMapVector(yi, "to", 6.82627, -5.89697, 4.89842);
	yafaray_setParamMapString(yi, "type", "perspective");
	yafaray_setParamMapVector(yi, "up", 7.16376, -6.19517, 6.23901);
	yafaray_defineCamera(yi, "Camera");
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapString(yi, "camera_name", "Camera");
	yafaray_createRenderView(yi, "");
	yafaray_clearParamMap(param_map);

	yafaray_setParamMapBool(yi, "cast_shadows", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapColor(yi, "color", 0.155663, 0.155663, 0.155663, 1);
	yafaray_setParamMapBool(yi, "ibl", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapInt(yi, "ibl_samples", 16);
	yafaray_setParamMapFloat(yi, "power", 1);
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
	yafaray_setParamMapInt(yi, "raydepth", 10);
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

	yafaray_setParamMapFloat(yi, "AA_clamp_indirect", 0);
	yafaray_setParamMapFloat(yi, "AA_clamp_samples", 0);
	yafaray_setParamMapString(yi, "AA_dark_detection_type", "linear");
	yafaray_setParamMapFloat(yi, "AA_dark_threshold_factor", 0);
	yafaray_setParamMapBool(yi, "AA_detect_color_noise", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapInt(yi, "AA_inc_samples", 1);
	yafaray_setParamMapFloat(yi, "AA_indirect_sample_multiplier_factor", 1);
	yafaray_setParamMapFloat(yi, "AA_light_sample_multiplier_factor", 1);
	yafaray_setParamMapInt(yi, "AA_minsamples", 1);
	yafaray_setParamMapInt(yi, "AA_passes", 1);
	yafaray_setParamMapFloat(yi, "AA_pixelwidth", 1);
	yafaray_setParamMapFloat(yi, "AA_resampled_floor", 0);
	yafaray_setParamMapFloat(yi, "AA_sample_multiplier_factor", 1);
	yafaray_setParamMapFloat(yi, "AA_threshold", 0.05);
	yafaray_setParamMapInt(yi, "AA_variance_edge_size", 10);
	yafaray_setParamMapInt(yi, "AA_variance_pixels", 0);
	yafaray_setParamMapBool(yi, "adv_auto_min_raydist_enabled", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapBool(yi, "adv_auto_shadow_bias_enabled", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapInt(yi, "adv_base_sampling_offset", 0);
	yafaray_setParamMapInt(yi, "adv_computer_node", 0);
	yafaray_setParamMapFloat(yi, "adv_min_raydist_value", 5e-05);
	yafaray_setParamMapFloat(yi, "adv_shadow_bias_value", 0.0005);
	yafaray_setParamMapBool(yi, "background_resampling", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapInt(yi, "film_autosave_interval_passes", 1);
	yafaray_setParamMapFloat(yi, "film_autosave_interval_seconds", 300);
	yafaray_setParamMapString(yi, "film_autosave_interval_type", "none");
	yafaray_setParamMapString(yi, "film_load_save_mode", "none");
	yafaray_setParamMapString(yi, "filter_type", "gauss");
	yafaray_setParamMapInt(yi, "height", 432);
	yafaray_setParamMapInt(yi, "images_autosave_interval_passes", 1);
	yafaray_setParamMapFloat(yi, "images_autosave_interval_seconds", 300);
	yafaray_setParamMapString(yi, "images_autosave_interval_type", "none");
	yafaray_setParamMapFloat(yi, "layer_faces_edge_smoothness", 0.5);
	yafaray_setParamMapInt(yi, "layer_faces_edge_thickness", 1);
	yafaray_setParamMapFloat(yi, "layer_faces_edge_threshold", 0.01);
	yafaray_setParamMapBool(yi, "layer_mask_invert", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapInt(yi, "layer_mask_mat_index", 0);
	yafaray_setParamMapInt(yi, "layer_mask_obj_index", 0);
	yafaray_setParamMapBool(yi, "layer_mask_only", YAFARAY_BOOL_FALSE);
	yafaray_setParamMapFloat(yi, "layer_object_edge_smoothness", 0.75);
	yafaray_setParamMapInt(yi, "layer_object_edge_thickness", 2);
	yafaray_setParamMapFloat(yi, "layer_object_edge_threshold", 0.3);
	yafaray_setParamMapColor(yi, "layer_toon_edge_color", 0, 0, 0, 1);
	yafaray_setParamMapFloat(yi, "layer_toon_post_smooth", 3);
	yafaray_setParamMapFloat(yi, "layer_toon_pre_smooth", 3);
	yafaray_setParamMapFloat(yi, "layer_toon_quantization", 0.1);
	yafaray_setParamMapString(yi, "scene_accelerator", "yafaray-kdtree-original");
	yafaray_setParamMapBool(yi, "show_sam_pix", YAFARAY_BOOL_TRUE);
	yafaray_setParamMapInt(yi, "threads", 1);
	yafaray_setParamMapInt(yi, "threads_photons", 1);
	yafaray_setParamMapInt(yi, "tile_size", 32);
	yafaray_setParamMapString(yi, "tiles_order", "centre");
	yafaray_setParamMapInt(yi, "width", 10);
	yafaray_setParamMapInt(yi, "height", 10);
	yafaray_setParamMapInt(yi, "xstart", 80);
	yafaray_setParamMapInt(yi, "ystart", 220);
	yafaray_preprocessSurfaceIntegrator(yi);
	yafaray_clearParamMap(param_map);

	/* Creating image output */
	yafaray_setParamMapString(yi, "image_path", "./test03-output1.tga");
	yafaray_setParamMapString(yi, "color_space", "sRGB");
	yafaray_setParamMapString(yi, "badge_position", "none");
	yafaray_createOutput(yi, "output1_tga");
	yafaray_clearParamMap(param_map);

	yafaray_render(yi, NULL, NULL, YAFARAY_DISPLAY_CONSOLE_NORMAL);

	yafaray_destroyInterface(yi);

	return 0;
}
