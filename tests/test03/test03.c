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
#define NULL 0

int main()
{
	yafaray_Interface_t *yi = yafaray_createInterface(YAFARAY_INTERFACE_FOR_RENDERING, NULL, NULL, NULL, YAFARAY_DISPLAY_CONSOLE_NORMAL);
	yafaray_setConsoleLogColorsEnabled(yi, YAFARAY_BOOL_TRUE);
	yafaray_setConsoleVerbosityLevel(yi, YAFARAY_LOG_LEVEL_DEBUG);

	yafaray_paramsSetString(yi, "type", "yafaray");
	yafaray_createScene(yi);
	yafaray_paramsClearAll(yi);
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
	yafaray_paramsSetString(yi, "color_space", "sRGB");
	yafaray_paramsSetFloat(yi, "cropmax_x", 1);
	yafaray_paramsSetFloat(yi, "cropmax_y", 1);
	yafaray_paramsSetFloat(yi, "cropmin_x", 0);
	yafaray_paramsSetFloat(yi, "cropmin_y", 0);
	yafaray_paramsSetFloat(yi, "ewa_max_anisotropy", 8);
	yafaray_paramsSetString(yi, "filename", "pixels_texture.tga");
	yafaray_paramsSetFloat(yi, "gamma", 1);
	yafaray_paramsSetString(yi, "image_optimization", "optimized");
	yafaray_paramsSetBool(yi, "img_grayscale", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetString(yi, "interpolate", "bilinear");
	yafaray_paramsSetBool(yi, "mirror_x", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "mirror_y", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "normalmap", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "rot90", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetFloat(yi, "trilinear_level_bias", 0);
	yafaray_paramsSetString(yi, "type", "image");
	yafaray_paramsSetBool(yi, "use_alpha", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetInt(yi, "xrepeat", 1);
	yafaray_paramsSetInt(yi, "yrepeat", 1);
	yafaray_createImage(yi, "Tex_image");
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetString(yi, "image_name", "Tex_image");
	yafaray_paramsSetString(yi, "type", "image");
	yafaray_createTexture(yi, "Tex");
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetBool(yi, "adj_clamp", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetFloat(yi, "adj_contrast", 1);
	yafaray_paramsSetFloat(yi, "adj_hue", 0);
	yafaray_paramsSetFloat(yi, "adj_intensity", 1);
	yafaray_paramsSetFloat(yi, "adj_mult_factor_blue", 1);
	yafaray_paramsSetFloat(yi, "adj_mult_factor_green", 1);
	yafaray_paramsSetFloat(yi, "adj_mult_factor_red", 1);
	yafaray_paramsSetFloat(yi, "adj_saturation", 1);
	yafaray_paramsSetInt(yi, "depth", 0);
	yafaray_paramsSetFloat(yi, "ewa_max_anisotropy", 8);
	yafaray_paramsSetBool(yi, "hard", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetBool(yi, "img_grayscale", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetString(yi, "noise_type", "blender");
	yafaray_paramsSetString(yi, "shape", "sin");
	yafaray_paramsSetFloat(yi, "size", 0.25);
	yafaray_paramsSetFloat(yi, "trilinear_level_bias", 0);
	yafaray_paramsSetFloat(yi, "turbulence", 0);
	yafaray_paramsSetString(yi, "type", "wood");
	yafaray_paramsSetString(yi, "wood_type", "bands");
	yafaray_createTexture(yi, "Texture");
	yafaray_paramsClearAll(yi);

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
	yafaray_paramsSetString(yi, "color_space", "sRGB");
	yafaray_paramsSetFloat(yi, "cropmax_x", 1);
	yafaray_paramsSetFloat(yi, "cropmax_y", 1);
	yafaray_paramsSetFloat(yi, "cropmin_x", 0);
	yafaray_paramsSetFloat(yi, "cropmin_y", 0);
	yafaray_paramsSetFloat(yi, "ewa_max_anisotropy", 8);
	yafaray_paramsSetString(yi, "filename", "tex.tga");
	yafaray_paramsSetFloat(yi, "gamma", 1);
	yafaray_paramsSetString(yi, "image_optimization", "optimized");
	yafaray_paramsSetBool(yi, "img_grayscale", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetString(yi, "interpolate", "bilinear");
	yafaray_paramsSetBool(yi, "mirror_x", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "mirror_y", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "normalmap", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "rot90", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetFloat(yi, "trilinear_level_bias", 0);
	yafaray_paramsSetString(yi, "type", "image");
	yafaray_paramsSetBool(yi, "use_alpha", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetInt(yi, "xrepeat", 1);
	yafaray_paramsSetInt(yi, "yrepeat", 1);
	yafaray_createImage(yi, "Texture.001_image");
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetString(yi, "image_name", "Texture.001_image");
	yafaray_paramsSetString(yi, "type", "image");
	yafaray_createTexture(yi, "Texture.001");
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetColor(yi, "color", 0.8, 0.8, 0.8, 1);
	yafaray_paramsSetString(yi, "type", "shinydiffusemat");
	yafaray_paramsEndList(yi);
	yafaray_createMaterial(yi, "defaultMat");
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetFloat(yi, "IOR", 1.52);
	yafaray_paramsSetColor(yi, "absorption", 1, 1, 1, 1);
	yafaray_paramsSetFloat(yi, "absorption_dist", 1);
	yafaray_paramsSetInt(yi, "additionaldepth", 0);
	yafaray_paramsSetFloat(yi, "dispersion_power", 0);
	yafaray_paramsSetBool(yi, "fake_shadows", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetColor(yi, "filter_color", 1, 1, 1, 1);
	yafaray_paramsSetBool(yi, "flat_material", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetInt(yi, "mat_pass_index", 0);
	yafaray_paramsSetColor(yi, "mirror_color", 1, 1, 1, 1);
	yafaray_paramsSetBool(yi, "receive_shadows", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetFloat(yi, "samplingfactor", 1);
	yafaray_paramsSetFloat(yi, "transmit_filter", 1);
	yafaray_paramsSetString(yi, "type", "glass");
	yafaray_paramsSetString(yi, "visibility", "normal");
	yafaray_paramsSetFloat(yi, "wireframe_amount", 0);
	yafaray_paramsSetColor(yi, "wireframe_color", 1, 1, 1, 1);
	yafaray_paramsSetFloat(yi, "wireframe_exponent", 0);
	yafaray_paramsSetFloat(yi, "wireframe_thickness", 0.01);
	yafaray_paramsEndList(yi);
	yafaray_createMaterial(yi, "Mat1");
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetFloat(yi, "IOR", 1.8);
	yafaray_paramsSetInt(yi, "additionaldepth", 0);
	yafaray_paramsSetColor(yi, "color", 0.8, 0.8, 0.8, 1);
	yafaray_paramsSetFloat(yi, "diffuse_reflect", 1);
	yafaray_paramsSetString(yi, "diffuse_shader", "diff_layer0");
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
	yafaray_paramsPushList(yi);
	yafaray_paramsSetString(yi, "blend_mode", "mix");
	yafaray_paramsSetFloat(yi, "colfac", 1);
	yafaray_paramsSetBool(yi, "color_input", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetColor(yi, "def_col", 1, 0, 1, 1);
	yafaray_paramsSetFloat(yi, "def_val", 1);
	yafaray_paramsSetBool(yi, "do_color", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetBool(yi, "do_scalar", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetString(yi, "element", "shader_node");
	yafaray_paramsSetString(yi, "input", "map0");
	yafaray_paramsSetString(yi, "name", "diff_layer0");
	yafaray_paramsSetBool(yi, "negative", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "noRGB", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "stencil", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetString(yi, "type", "layer");
	yafaray_paramsSetColor(yi, "upper_color", 0.8, 0.8, 0.8, 1);
	yafaray_paramsSetFloat(yi, "upper_value", 0);
	yafaray_paramsSetBool(yi, "use_alpha", YAFARAY_BOOL_FALSE);
	yafaray_paramsPushList(yi);
	yafaray_paramsSetString(yi, "element", "shader_node");
	yafaray_paramsSetString(yi, "mapping", "plain");
	yafaray_paramsSetString(yi, "name", "map0");
	yafaray_paramsSetVector(yi, "offset", 0, 0, 0);
	yafaray_paramsSetInt(yi, "proj_x", 1);
	yafaray_paramsSetInt(yi, "proj_y", 2);
	yafaray_paramsSetInt(yi, "proj_z", 3);
	yafaray_paramsSetVector(yi, "scale", 1, 1, 1);
	yafaray_paramsSetString(yi, "texco", "orco");
	yafaray_paramsSetString(yi, "texture", "Tex");
	yafaray_paramsSetString(yi, "type", "texture_mapper");
	yafaray_paramsEndList(yi);
	yafaray_createMaterial(yi, "diffuse2");
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetFloat(yi, "IOR", 1.8);
	yafaray_paramsSetInt(yi, "additionaldepth", 0);
	yafaray_paramsSetColor(yi, "color", 0.8, 0.8, 0.8, 1);
	yafaray_paramsSetFloat(yi, "diffuse_reflect", 1);
	yafaray_paramsSetString(yi, "diffuse_shader", "diff_layer2");
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
	yafaray_paramsPushList(yi);
	yafaray_paramsSetString(yi, "blend_mode", "mix");
	yafaray_paramsSetFloat(yi, "colfac", 1);
	yafaray_paramsSetBool(yi, "color_input", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetColor(yi, "def_col", 1, 0, 1, 1);
	yafaray_paramsSetFloat(yi, "def_val", 1);
	yafaray_paramsSetBool(yi, "do_color", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetBool(yi, "do_scalar", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetString(yi, "element", "shader_node");
	yafaray_paramsSetString(yi, "input", "map0");
	yafaray_paramsSetString(yi, "name", "diff_layer0");
	yafaray_paramsSetBool(yi, "negative", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "noRGB", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "stencil", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetString(yi, "type", "layer");
	yafaray_paramsSetColor(yi, "upper_color", 0.8, 0.8, 0.8, 1);
	yafaray_paramsSetFloat(yi, "upper_value", 0);
	yafaray_paramsSetBool(yi, "use_alpha", YAFARAY_BOOL_FALSE);
	yafaray_paramsPushList(yi);
	yafaray_paramsSetString(yi, "element", "shader_node");
	yafaray_paramsSetString(yi, "mapping", "cube");
	yafaray_paramsSetString(yi, "name", "map0");
	yafaray_paramsSetVector(yi, "offset", 0, 0, 0);
	yafaray_paramsSetInt(yi, "proj_x", 1);
	yafaray_paramsSetInt(yi, "proj_y", 2);
	yafaray_paramsSetInt(yi, "proj_z", 3);
	yafaray_paramsSetVector(yi, "scale", 1, 1, 1);
	yafaray_paramsSetString(yi, "texco", "orco");
	yafaray_paramsSetString(yi, "texture", "Tex");
	yafaray_paramsSetString(yi, "type", "texture_mapper");
	yafaray_paramsPushList(yi);
	yafaray_paramsSetString(yi, "blend_mode", "mix");
	yafaray_paramsSetFloat(yi, "colfac", 1);
	yafaray_paramsSetBool(yi, "color_input", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetColor(yi, "def_col", 1, 0, 1, 1);
	yafaray_paramsSetFloat(yi, "def_val", 1);
	yafaray_paramsSetBool(yi, "do_color", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetBool(yi, "do_scalar", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetString(yi, "element", "shader_node");
	yafaray_paramsSetString(yi, "input", "map1");
	yafaray_paramsSetString(yi, "name", "diff_layer1");
	yafaray_paramsSetBool(yi, "negative", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "noRGB", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "stencil", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetString(yi, "type", "layer");
	yafaray_paramsSetString(yi, "upper_layer", "diff_layer0");
	yafaray_paramsSetBool(yi, "use_alpha", YAFARAY_BOOL_FALSE);
	yafaray_paramsPushList(yi);
	yafaray_paramsSetString(yi, "element", "shader_node");
	yafaray_paramsSetString(yi, "mapping", "cube");
	yafaray_paramsSetString(yi, "name", "map1");
	yafaray_paramsSetVector(yi, "offset", 0, 0, 0);
	yafaray_paramsSetInt(yi, "proj_x", 1);
	yafaray_paramsSetInt(yi, "proj_y", 2);
	yafaray_paramsSetInt(yi, "proj_z", 3);
	yafaray_paramsSetVector(yi, "scale", 1, 1, 1);
	yafaray_paramsSetString(yi, "texco", "orco");
	yafaray_paramsSetString(yi, "texture", "Texture");
	yafaray_paramsSetString(yi, "type", "texture_mapper");
	yafaray_paramsPushList(yi);
	yafaray_paramsSetString(yi, "blend_mode", "mix");
	yafaray_paramsSetFloat(yi, "colfac", 1);
	yafaray_paramsSetBool(yi, "color_input", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetColor(yi, "def_col", 1, 0, 1, 1);
	yafaray_paramsSetFloat(yi, "def_val", 1);
	yafaray_paramsSetBool(yi, "do_color", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetBool(yi, "do_scalar", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetString(yi, "element", "shader_node");
	yafaray_paramsSetString(yi, "input", "map2");
	yafaray_paramsSetString(yi, "name", "diff_layer2");
	yafaray_paramsSetBool(yi, "negative", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "noRGB", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "stencil", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetString(yi, "type", "layer");
	yafaray_paramsSetString(yi, "upper_layer", "diff_layer1");
	yafaray_paramsSetBool(yi, "use_alpha", YAFARAY_BOOL_FALSE);
	yafaray_paramsPushList(yi);
	yafaray_paramsSetString(yi, "element", "shader_node");
	yafaray_paramsSetString(yi, "mapping", "cube");
	yafaray_paramsSetString(yi, "name", "map2");
	yafaray_paramsSetVector(yi, "offset", 0, 0, 0);
	yafaray_paramsSetInt(yi, "proj_x", 1);
	yafaray_paramsSetInt(yi, "proj_y", 2);
	yafaray_paramsSetInt(yi, "proj_z", 3);
	yafaray_paramsSetVector(yi, "scale", 1, 1, 1);
	yafaray_paramsSetString(yi, "texco", "orco");
	yafaray_paramsSetString(yi, "texture", "Texture.001");
	yafaray_paramsSetString(yi, "type", "texture_mapper");
	yafaray_paramsEndList(yi);
	yafaray_createMaterial(yi, "Mat2");
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetInt(yi, "additionaldepth", 0);
	yafaray_paramsSetFloat(yi, "blend_value", 0);
	yafaray_paramsSetBool(yi, "flat_material", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetString(yi, "material1", "Mat1");
	yafaray_paramsSetString(yi, "material2", "Mat2");
	yafaray_paramsSetBool(yi, "receive_shadows", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetFloat(yi, "samplingfactor", 1);
	yafaray_paramsSetString(yi, "type", "blend_mat");
	yafaray_paramsSetString(yi, "visibility", "normal");
	yafaray_paramsSetFloat(yi, "wireframe_amount", 0);
	yafaray_paramsSetColor(yi, "wireframe_color", 1, 1, 1, 1);
	yafaray_paramsSetFloat(yi, "wireframe_exponent", 0);
	yafaray_paramsSetFloat(yi, "wireframe_thickness", 0.01);
	yafaray_paramsEndList(yi);
	yafaray_createMaterial(yi, "blend");
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
	yafaray_createMaterial(yi, "diffuse");
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetBool(yi, "cast_shadows", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetColor(yi, "color", 1, 1, 1, 1);
	yafaray_paramsSetVector(yi, "from", -4.44722, 1.00545, 5.90386);
	yafaray_paramsSetBool(yi, "light_enabled", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetBool(yi, "photon_only", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetFloat(yi, "power", 50);
	yafaray_paramsSetString(yi, "type", "pointlight");
	yafaray_paramsSetBool(yi, "with_caustic", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetBool(yi, "with_diffuse", YAFARAY_BOOL_TRUE);
	yafaray_createLight(yi, "Lamp.001");
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetBool(yi, "cast_shadows", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetColor(yi, "color", 1, 1, 1, 1);
	yafaray_paramsSetVector(yi, "from", 4.07625, 1.00545, 5.90386);
	yafaray_paramsSetBool(yi, "light_enabled", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetBool(yi, "photon_only", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetFloat(yi, "power", 50);
	yafaray_paramsSetString(yi, "type", "pointlight");
	yafaray_paramsSetBool(yi, "with_caustic", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetBool(yi, "with_diffuse", YAFARAY_BOOL_TRUE);
	yafaray_createLight(yi, "Lamp");
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetBool(yi, "has_orco", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetBool(yi, "has_uv", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "is_base_object", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetInt(yi, "num_faces", 6);
	yafaray_paramsSetInt(yi, "num_vertices", 8);
	yafaray_paramsSetInt(yi, "object_index", 0);
	yafaray_paramsSetString(yi, "type", "mesh");
	yafaray_paramsSetString(yi, "visibility", "visible");
	yafaray_createObject(yi, "Cube.002");
	yafaray_paramsClearAll(yi);

	yafaray_addVertexWithOrco(yi, 1, 4, 1.00136e-05, 1, 1, -1);
	yafaray_addVertexWithOrco(yi, 1, 2, 1.00136e-05, 1, -0.999999, -1);
	yafaray_addVertexWithOrco(yi, -1, 2, 1.00136e-05, -1, -0.999999, -1);
	yafaray_addVertexWithOrco(yi, -1, 4, 1.00136e-05, -0.999999, 1, -1);
	yafaray_addVertexWithOrco(yi, 1, 4, 2.00001, 1, 0.999999, 1);
	yafaray_addVertexWithOrco(yi, 0.999999, 2, 2.00001, 0.999999, -1, 1);
	yafaray_addVertexWithOrco(yi, -1, 2, 2.00001, -1, -0.999999, 1);
	yafaray_addVertexWithOrco(yi, -1, 4, 2.00001, -1, 1, 1);
	yafaray_setCurrentMaterial(yi, "Mat1");
	yafaray_addTriangle(yi, 0, 1, 2);
	yafaray_addTriangle(yi, 0, 2, 3);
	yafaray_addTriangle(yi, 4, 7, 6);
	yafaray_addTriangle(yi, 4, 6, 5);
	yafaray_addTriangle(yi, 0, 4, 5);
	yafaray_addTriangle(yi, 0, 5, 1);
	yafaray_addTriangle(yi, 1, 5, 6);
	yafaray_addTriangle(yi, 1, 6, 2);
	yafaray_addTriangle(yi, 2, 6, 7);
	yafaray_addTriangle(yi, 2, 7, 3);
	yafaray_addTriangle(yi, 4, 0, 3);
	yafaray_addTriangle(yi, 4, 3, 7);
	yafaray_endObject(yi);
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetBool(yi, "has_orco", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetBool(yi, "has_uv", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "is_base_object", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetInt(yi, "num_faces", 6);
	yafaray_paramsSetInt(yi, "num_vertices", 8);
	yafaray_paramsSetInt(yi, "object_index", 0);
	yafaray_paramsSetString(yi, "type", "mesh");
	yafaray_paramsSetString(yi, "visibility", "visible");
	yafaray_createObject(yi, "Cube.001");
	yafaray_paramsClearAll(yi);

	yafaray_addVertexWithOrco(yi, 1, -2, 1.00136e-05, 1, 1, -1);
	yafaray_addVertexWithOrco(yi, 1, -4, 1.00136e-05, 1, -0.999999, -1);
	yafaray_addVertexWithOrco(yi, -1, -4, 1.00136e-05, -1, -0.999999, -1);
	yafaray_addVertexWithOrco(yi, -1, -2, 1.00136e-05, -0.999999, 1, -1);
	yafaray_addVertexWithOrco(yi, 1, -2, 2.00001, 1, 0.999999, 1);
	yafaray_addVertexWithOrco(yi, 0.999999, -4, 2.00001, 0.999999, -1, 1);
	yafaray_addVertexWithOrco(yi, -1, -4, 2.00001, -1, -0.999999, 1);
	yafaray_addVertexWithOrco(yi, -1, -2, 2.00001, -1, 1, 1);
	yafaray_setCurrentMaterial(yi, "blend");
	yafaray_addTriangle(yi, 0, 1, 2);
	yafaray_addTriangle(yi, 0, 2, 3);
	yafaray_addTriangle(yi, 4, 7, 6);
	yafaray_addTriangle(yi, 4, 6, 5);
	yafaray_addTriangle(yi, 0, 4, 5);
	yafaray_addTriangle(yi, 0, 5, 1);
	yafaray_addTriangle(yi, 1, 5, 6);
	yafaray_addTriangle(yi, 1, 6, 2);
	yafaray_addTriangle(yi, 2, 6, 7);
	yafaray_addTriangle(yi, 2, 7, 3);
	yafaray_addTriangle(yi, 4, 0, 3);
	yafaray_addTriangle(yi, 4, 3, 7);
	yafaray_endObject(yi);
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetBool(yi, "has_orco", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "has_uv", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "is_base_object", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetInt(yi, "num_faces", 1);
	yafaray_paramsSetInt(yi, "num_vertices", 4);
	yafaray_paramsSetInt(yi, "object_index", 0);
	yafaray_paramsSetString(yi, "type", "mesh");
	yafaray_paramsSetString(yi, "visibility", "visible");
	yafaray_createObject(yi, "Plane");
	yafaray_paramsClearAll(yi);

	yafaray_addVertex(yi, -10, -10, 0);
	yafaray_addVertex(yi, 10, -10, 0);
	yafaray_addVertex(yi, -10, 10, 0);
	yafaray_addVertex(yi, 10, 10, 0);
	yafaray_setCurrentMaterial(yi, "diffuse");
	yafaray_addTriangle(yi, 0, 1, 3);
	yafaray_addTriangle(yi, 0, 3, 2);
	yafaray_endObject(yi);
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetBool(yi, "has_orco", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetBool(yi, "has_uv", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "is_base_object", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetInt(yi, "num_faces", 6);
	yafaray_paramsSetInt(yi, "num_vertices", 8);
	yafaray_paramsSetInt(yi, "object_index", 0);
	yafaray_paramsSetString(yi, "type", "mesh");
	yafaray_paramsSetString(yi, "visibility", "visible");
	yafaray_createObject(yi, "Cube");
	yafaray_paramsClearAll(yi);

	yafaray_addVertexWithOrco(yi, 1, 1, 1.00136e-05, 1, 1, -1);
	yafaray_addVertexWithOrco(yi, 1, -1, 1.00136e-05, 1, -0.999999, -1);
	yafaray_addVertexWithOrco(yi, -1, -1, 1.00136e-05, -1, -0.999999, -1);
	yafaray_addVertexWithOrco(yi, -1, 1, 1.00136e-05, -0.999999, 1, -1);
	yafaray_addVertexWithOrco(yi, 1, 0.999999, 2.00001, 1, 0.999999, 1);
	yafaray_addVertexWithOrco(yi, 0.999999, -1, 2.00001, 0.999999, -1, 1);
	yafaray_addVertexWithOrco(yi, -1, -1, 2.00001, -1, -0.999999, 1);
	yafaray_addVertexWithOrco(yi, -1, 1, 2.00001, -1, 1, 1);
	yafaray_setCurrentMaterial(yi, "Mat2");
	yafaray_addTriangle(yi, 0, 1, 2);
	yafaray_addTriangle(yi, 0, 2, 3);
	yafaray_addTriangle(yi, 4, 7, 6);
	yafaray_addTriangle(yi, 4, 6, 5);
	yafaray_addTriangle(yi, 0, 4, 5);
	yafaray_addTriangle(yi, 0, 5, 1);
	yafaray_addTriangle(yi, 1, 5, 6);
	yafaray_addTriangle(yi, 1, 6, 2);
	yafaray_addTriangle(yi, 2, 6, 7);
	yafaray_addTriangle(yi, 2, 7, 3);
	yafaray_addTriangle(yi, 4, 0, 3);
	yafaray_addTriangle(yi, 4, 3, 7);
	yafaray_endObject(yi);
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetFloat(yi, "aperture", 0);
	yafaray_paramsSetFloat(yi, "bokeh_rotation", 0);
	yafaray_paramsSetString(yi, "bokeh_type", "disk1");
	yafaray_paramsSetFloat(yi, "dof_distance", 0);
	yafaray_paramsSetFloat(yi, "focal", 1.09375);
	yafaray_paramsSetVector(yi, "from", 7.48113, -6.50764, 5.34367);
	yafaray_paramsSetInt(yi, "resx", 768);
	yafaray_paramsSetInt(yi, "resy", 432);
	yafaray_paramsSetVector(yi, "to", 6.82627, -5.89697, 4.89842);
	yafaray_paramsSetString(yi, "type", "perspective");
	yafaray_paramsSetVector(yi, "up", 7.16376, -6.19517, 6.23901);
	yafaray_createCamera(yi, "Camera");
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetString(yi, "camera_name", "Camera");
	yafaray_createRenderView(yi, "");
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetBool(yi, "cast_shadows", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetBool(yi, "cast_shadows_sun", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetColor(yi, "color", 0.155663, 0.155663, 0.155663, 1);
	yafaray_paramsSetBool(yi, "ibl", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetInt(yi, "ibl_samples", 16);
	yafaray_paramsSetFloat(yi, "power", 1);
	yafaray_paramsSetString(yi, "type", "constant");
	yafaray_paramsSetBool(yi, "with_caustic", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetBool(yi, "with_diffuse", YAFARAY_BOOL_TRUE);
	yafaray_createBackground(yi, "world_background");
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetColor(yi, "AO_color", 0.9, 0.9, 0.9, 1);
	yafaray_paramsSetFloat(yi, "AO_distance", 1);
	yafaray_paramsSetInt(yi, "AO_samples", 32);
	yafaray_paramsSetBool(yi, "bg_transp", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "bg_transp_refract", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "caustics", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetBool(yi, "do_AO", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetString(yi, "photon_maps_processing", "generate-only");
	yafaray_paramsSetInt(yi, "raydepth", 10);
	yafaray_paramsSetInt(yi, "shadowDepth", 2);
	yafaray_paramsSetBool(yi, "transpShad", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetString(yi, "type", "directlighting");
	yafaray_createIntegrator(yi, "default");
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetString(yi, "exported_image_name", "Combined");
	yafaray_paramsSetString(yi, "exported_image_type", "ColorAlpha");
	yafaray_paramsSetString(yi, "image_type", "ColorAlpha");
	yafaray_paramsSetString(yi, "type", "combined");
	yafaray_defineLayer(yi);
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetFloat(yi, "AA_clamp_indirect", 0);
	yafaray_paramsSetFloat(yi, "AA_clamp_samples", 0);
	yafaray_paramsSetString(yi, "AA_dark_detection_type", "linear");
	yafaray_paramsSetFloat(yi, "AA_dark_threshold_factor", 0);
	yafaray_paramsSetBool(yi, "AA_detect_color_noise", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetInt(yi, "AA_inc_samples", 1);
	yafaray_paramsSetFloat(yi, "AA_indirect_sample_multiplier_factor", 1);
	yafaray_paramsSetFloat(yi, "AA_light_sample_multiplier_factor", 1);
	yafaray_paramsSetInt(yi, "AA_minsamples", 1);
	yafaray_paramsSetInt(yi, "AA_passes", 1);
	yafaray_paramsSetFloat(yi, "AA_pixelwidth", 1);
	yafaray_paramsSetFloat(yi, "AA_resampled_floor", 0);
	yafaray_paramsSetFloat(yi, "AA_sample_multiplier_factor", 1);
	yafaray_paramsSetFloat(yi, "AA_threshold", 0.05);
	yafaray_paramsSetInt(yi, "AA_variance_edge_size", 10);
	yafaray_paramsSetInt(yi, "AA_variance_pixels", 0);
	yafaray_paramsSetBool(yi, "adv_auto_min_raydist_enabled", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetBool(yi, "adv_auto_shadow_bias_enabled", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetInt(yi, "adv_base_sampling_offset", 0);
	yafaray_paramsSetInt(yi, "adv_computer_node", 0);
	yafaray_paramsSetFloat(yi, "adv_min_raydist_value", 5e-05);
	yafaray_paramsSetFloat(yi, "adv_shadow_bias_value", 0.0005);
	yafaray_paramsSetString(yi, "background_name", "world_background");
	yafaray_paramsSetBool(yi, "background_resampling", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetInt(yi, "film_autosave_interval_passes", 1);
	yafaray_paramsSetFloat(yi, "film_autosave_interval_seconds", 300);
	yafaray_paramsSetString(yi, "film_autosave_interval_type", "none");
	yafaray_paramsSetString(yi, "film_load_save_mode", "none");
	yafaray_paramsSetString(yi, "filter_type", "gauss");
	yafaray_paramsSetInt(yi, "height", 432);
	yafaray_paramsSetInt(yi, "images_autosave_interval_passes", 1);
	yafaray_paramsSetFloat(yi, "images_autosave_interval_seconds", 300);
	yafaray_paramsSetString(yi, "images_autosave_interval_type", "none");
	yafaray_paramsSetString(yi, "integrator_name", "default");
	yafaray_paramsSetFloat(yi, "layer_faces_edge_smoothness", 0.5);
	yafaray_paramsSetInt(yi, "layer_faces_edge_thickness", 1);
	yafaray_paramsSetFloat(yi, "layer_faces_edge_threshold", 0.01);
	yafaray_paramsSetBool(yi, "layer_mask_invert", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetInt(yi, "layer_mask_mat_index", 0);
	yafaray_paramsSetInt(yi, "layer_mask_obj_index", 0);
	yafaray_paramsSetBool(yi, "layer_mask_only", YAFARAY_BOOL_FALSE);
	yafaray_paramsSetFloat(yi, "layer_object_edge_smoothness", 0.75);
	yafaray_paramsSetInt(yi, "layer_object_edge_thickness", 2);
	yafaray_paramsSetFloat(yi, "layer_object_edge_threshold", 0.3);
	yafaray_paramsSetColor(yi, "layer_toon_edge_color", 0, 0, 0, 1);
	yafaray_paramsSetFloat(yi, "layer_toon_post_smooth", 3);
	yafaray_paramsSetFloat(yi, "layer_toon_pre_smooth", 3);
	yafaray_paramsSetFloat(yi, "layer_toon_quantization", 0.1);
	yafaray_paramsSetString(yi, "scene_accelerator", "yafaray-kdtree-original");
	yafaray_paramsSetBool(yi, "show_sam_pix", YAFARAY_BOOL_TRUE);
	yafaray_paramsSetInt(yi, "threads", 1);
	yafaray_paramsSetInt(yi, "threads_photons", 1);
	yafaray_paramsSetInt(yi, "tile_size", 32);
	yafaray_paramsSetString(yi, "tiles_order", "centre");
	yafaray_paramsSetInt(yi, "width", 10);
	yafaray_paramsSetInt(yi, "height", 10);
	yafaray_paramsSetInt(yi, "xstart", 80);
	yafaray_paramsSetInt(yi, "ystart", 220);
	yafaray_setupRender(yi);
	yafaray_paramsClearAll(yi);

	/* Creating image output */
	yafaray_paramsSetString(yi, "image_path", "./test03-output1.tga");
	yafaray_paramsSetString(yi, "color_space", "sRGB");
	yafaray_paramsSetString(yi, "badge_position", "none");
	yafaray_createOutput(yi, "output1_tga");
	yafaray_paramsClearAll(yi);

	yafaray_render(yi, NULL, NULL, YAFARAY_DISPLAY_CONSOLE_NORMAL);

	yafaray_destroyInterface(yi);

	return 0;
}
