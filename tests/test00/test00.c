/****************************************************************************
 *      This is part of the libYafaRay package
 *
 *      test00.c : basic client example, including callbacks.
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "yafaray_c_api.h"
#include <signal.h>

#ifdef _WIN32
#include <windows.h>
#endif

struct ResultImage
{
    int width_;
    int height_;
    char *data_;
};

void notifyLayerCallback(const char *internal_layer_name, const char *exported_layer_name, int width, int height, int layer_exported_channels, void *callback_data);
void putPixelCallback(const char *layer_name, int x, int y, float r, float g, float b, float a, void *callback_data);
void flushAreaCallback(int area_id, int x_0, int y_0, int x_1, int y_1, void *callback_data);
void flushCallback(void *callback_data);
void highlightCallback(int area_id, int x_0, int y_0, int x_1, int y_1, void *callback_data);
void monitorCallback(int steps_total, int steps_done, const char *tag, void *callback_data);
void loggerCallback(yafaray_LogLevel log_level, size_t datetime, const char *time_of_day, const char *description, void *callback_data);

yafaray_Logger *logger = NULL;
yafaray_RenderControl *render_control = NULL;

#ifdef _WIN32
BOOL WINAPI ctrlCHandler_global(DWORD signal)
{
	yafaray_printWarning(logger, "CTRL+C pressed, cancelling.\n");
	if(render_control)
	{
		yafaray_cancelRendering(render_control);
		return TRUE;
	}
	else exit(1);
}
#else
void ctrlCHandler_global(int signal)
{
	yafaray_printWarning(logger, "CTRL+C pressed, cancelling.\n");
	if(render_control) yafaray_cancelRendering(render_control);
	else exit(1);
}
#endif

int main(void)
{
	yafaray_ParamMap *param_map = NULL;
	yafaray_ParamMapList *param_map_list = NULL;
	yafaray_Scene *scene = NULL;
	yafaray_SurfaceIntegrator *surface_integrator = NULL;
	yafaray_Film *film = NULL;
	yafaray_RenderMonitor *render_monitor = NULL;
	yafaray_SceneModifiedFlags scene_modified_flags = 0;
	FILE *fp = NULL;
	size_t material_id = 0;
	size_t object_id = 0;
	size_t image_id = 0;
	struct ResultImage result_image;
	size_t result_image_size_bytes = 0;
	int total_steps = 0;
	char *version_string = NULL;
	char *layers_string = NULL;
	int tex_width = 0;
	int tex_height = 0;
	int i = 0;
	int j = 0;

	printf("***** Test client 'test00' for libYafaRay *****\n");
	printf("Using libYafaRay version (%d.%d.%d)\n", yafaray_getVersionMajor(), yafaray_getVersionMinor(), yafaray_getVersionPatch());
	version_string = yafaray_getVersionString();
	printf("    libYafaRay version details: '%s'\n\n", version_string);
	yafaray_destroyCharString(version_string);

	/* handle CTRL+C events */
	#ifdef _WIN32
		SetConsoleCtrlHandler(ctrlCHandler_global, TRUE);
	#else
		signal(SIGINT, ctrlCHandler_global);
	#endif

	result_image.width_ = 400;
	result_image.height_ = 400;
	result_image_size_bytes = 3 * result_image.width_ * result_image.height_ * sizeof(char);
	result_image.data_ = malloc(result_image_size_bytes);
	printf("result_image: %p\n", (void *) &result_image);

	/* Basic libYafaRay C API usage example, rendering a cube with a TGA texture */

	/* YafaRay standard rendering interface */
	/*FIXME scene = yafaray_createInterface(YAFARAY_INTERFACE_FOR_RENDERING, "test00.xml", loggerCallback, &result_image, YAFARAY_DISPLAY_CONSOLE_NORMAL);*/
	logger = yafaray_createLogger(loggerCallback, &result_image, YAFARAY_DISPLAY_CONSOLE_NORMAL);
	yafaray_setConsoleLogColorsEnabled(logger, YAFARAY_BOOL_TRUE);
	yafaray_setConsoleVerbosityLevel(logger, YAFARAY_LOG_LEVEL_VERBOSE);

	/* Creating param map and param map list */
	param_map = yafaray_createParamMap();
	param_map_list = yafaray_createParamMapList();

	/* Creating scene */
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "scene_accelerator", "yafaray-kdtree-original");
	scene = yafaray_createScene(logger, "scene", param_map);

	/* Creating surface integrator */
	yafaray_clearParamMap(param_map);
	/*yafaray_setParamMapString(yi, "type", "directlighting");*/
	yafaray_setParamMapString(param_map, "type", "photonmapping");
	yafaray_setParamMapInt(param_map, "AA_passes", 5);
	yafaray_setParamMapInt(param_map, "AA_minsamples", 50);
	yafaray_setParamMapInt(param_map, "AA_inc_samples", 3);
	yafaray_setParamMapInt(param_map, "threads", -1);
	yafaray_setParamMapInt(param_map, "threads_photons", -1);
	surface_integrator = yafaray_createSurfaceIntegrator(logger, "surface integrator", param_map);

	/* Creating film */
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapInt(param_map, "width", result_image.width_);
	yafaray_setParamMapInt(param_map, "height", result_image.height_);
	yafaray_setParamMapString(param_map, "film_load_save_mode", "load-save");
	/*yafaray_setParamMapString(yi, "film_load_save_path", "???");*/
	yafaray_setParamMapInt(param_map, "threads", -1);
	film = yafaray_createFilm(logger, surface_integrator, "film", param_map);

	/* Creating film image outputs */
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "image_path", "./test00-output1.tga");
	yafaray_createOutput(film, "output1_tga", param_map);

	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "image_path", "./test00-output2.tga");
	yafaray_setParamMapString(param_map, "color_space", "Raw_Manual_Gamma");
	yafaray_setParamMapFloat(param_map, "gamma", 4.0);
	yafaray_setParamMapBool(param_map, "denoise_enabled", YAFARAY_BOOL_TRUE);
	yafaray_createOutput(film, "output2_tga", param_map);

	/* Setting up Film callbacks, must be done before yafaray_preprocessSurfaceIntegrator() */
	yafaray_setNotifyLayerCallback(film, notifyLayerCallback, (void *) &result_image);
	yafaray_setPutPixelCallback(film, putPixelCallback, (void *) &result_image);
	yafaray_setFlushAreaCallback(film, flushAreaCallback, (void *) &result_image);
	yafaray_setFlushCallback(film, flushCallback, (void *) &result_image);
	yafaray_setHighlightAreaCallback(film, highlightCallback, (void *) &result_image);

	/* Creating image from RAM or file */
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "type", "ColorAlpha"); /* Note: the specified type os overriden by the loaded image type */
	yafaray_setParamMapString(param_map, "image_optimization", "none"); /* Note: only "none" allows high dynamic range values > 1.f */
	yafaray_setParamMapString(param_map, "filename", "tex.tga");
	yafaray_createImage(scene, "Image01", &image_id, param_map);
	tex_width = yafaray_getImageWidth(scene, image_id);
	tex_height = yafaray_getImageHeight(scene, image_id);
	for(i = 0; i < tex_width / 2; ++i) /*For this test example we overwrite half of the image width with a "rainbow"*/
		for(j = 0; j < tex_height / 2; ++j) /*For this test example we overwrite half of the image height with a "rainbow"*/
			yafaray_setImageColor(scene, image_id, i, j, 0.01f * i, 0.01f * j, 0.01f * (i + j), 1.f);

	/* Creating texture from image */
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "type", "image");
	yafaray_setParamMapString(param_map, "image_name", "Image01");
	yafaray_createTexture(scene, "TextureTGA", param_map);

	/* Creating material */
	/* Shader tree definition */
	yafaray_clearParamMap(param_map);
	yafaray_clearParamMapList(param_map_list);
	yafaray_setParamMapString(param_map, "element", "shader_node");
	yafaray_setParamMapString(param_map, "name", "diff_layer0");
	yafaray_setParamMapString(param_map, "input", "map0");
	yafaray_setParamMapString(param_map, "type", "layer");
	yafaray_setParamMapString(param_map, "blend_mode", "mix");
	yafaray_setParamMapColor(param_map, "upper_color", 1.f, 1.f, 1.f, 1.f);
	yafaray_addParamMapToList(param_map_list, param_map);
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "element", "shader_node");
	yafaray_setParamMapString(param_map, "name", "map0");
	yafaray_setParamMapString(param_map, "type", "texture_mapper");
	yafaray_setParamMapString(param_map, "mapping", "cube");
	yafaray_setParamMapString(param_map, "texco", "orco");
	yafaray_setParamMapString(param_map, "texture", "TextureTGA");
	yafaray_addParamMapToList(param_map_list, param_map);
	/* Actual material creation */
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "type", "shinydiffusemat");
	yafaray_setParamMapColor(param_map, "color", 0.9f, 0.9f, 0.9f, 1.f);
	yafaray_setParamMapString(param_map, "diffuse_shader", "diff_layer0");
	yafaray_createMaterial(scene, &material_id, "MaterialTGA", param_map, param_map_list);

	/* Creating a geometric object */
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapBool(param_map, "has_orco", 1);
	yafaray_setParamMapString(param_map, "type", "mesh");
	yafaray_createObject(scene, &object_id, "Cube", param_map);
	/* Creating vertices for the object */
	yafaray_addVertexWithOrco(scene, object_id, -4.f, 1.5f, 0.f, -1.f, -1.f, -1);
	yafaray_addVertexWithOrco(scene, object_id, -4.f, 1.5f, 2.f, -1.f, -1.f, 1);
	yafaray_addVertexWithOrco(scene, object_id, -4.f, 3.5f, 0.f, -1.f, 1.f, -1);
	yafaray_addVertexWithOrco(scene, object_id, -4.f, 3.5f, 2.f, -1.f, 1.f, 1);
	yafaray_addVertexWithOrco(scene, object_id, -2.f, 1.5f, 0.f, 1.f, -1.f, -1);
	yafaray_addVertexWithOrco(scene, object_id, -2.f, 1.5f, 2.f, 1.f, -1.f, 1);
	yafaray_addVertexWithOrco(scene, object_id, -2.f, 3.5f, 0.f, 1.f, 1.f, -1);
	yafaray_addVertexWithOrco(scene, object_id, -2.f, 3.5f, 2.f, 1.f, 1.f, 1);

	/* Adding faces indicating the vertices indices used in each face and the material used for each face */
	yafaray_addTriangle(scene, object_id, 2, 0, 1, material_id);
	yafaray_addTriangle(scene, object_id, 2, 1, 3, material_id);
	yafaray_addTriangle(scene, object_id, 3, 7, 6, material_id);
	yafaray_addTriangle(scene, object_id, 3, 6, 2, material_id);
	yafaray_addTriangle(scene, object_id, 7, 5, 4, material_id);
	yafaray_addTriangle(scene, object_id, 7, 4, 6, material_id);
	yafaray_addTriangle(scene, object_id, 0, 4, 5, material_id);
	yafaray_addTriangle(scene, object_id, 0, 5, 1, material_id);
	yafaray_addTriangle(scene, object_id, 0, 2, 6, material_id);
	yafaray_addTriangle(scene, object_id, 0, 6, 4, material_id);
	yafaray_addTriangle(scene, object_id, 5, 7, 3, material_id);
	yafaray_addTriangle(scene, object_id, 5, 3, 1, material_id);

	/* Creating light/lamp */
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "type", "pointlight");
	yafaray_setParamMapColor(param_map, "color", 1.f, 1.f, 1.f, 1.f);
	yafaray_setParamMapVector(param_map, "from", 5.3f, -4.9f, 8.9f);
	yafaray_setParamMapFloat(param_map, "power", 150.f);
	yafaray_createLight(scene, "light_1", param_map);

	/* Creating scene background */
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "type", "constant");
	yafaray_setParamMapColor(param_map, "color", 1.f, 1.f, 1.f, 1.f);
	yafaray_defineBackground(scene, param_map);

	/* Creating camera */
	yafaray_clearParamMap(param_map);
	yafaray_setParamMapString(param_map, "type", "perspective");
	yafaray_setParamMapInt(param_map, "resx", result_image.width_);
	yafaray_setParamMapInt(param_map, "resy", result_image.height_);
	yafaray_setParamMapFloat(param_map, "focal", 1.1f);
	yafaray_setParamMapVector(param_map, "from", 8.6f, -7.2f, 8.1f);
	yafaray_setParamMapVector(param_map, "to", 8.0f, -6.7f, 7.6f);
	yafaray_setParamMapVector(param_map, "up", 8.3f, -6.8f, 9.f);
	yafaray_defineCamera(film, "cam_1", param_map);

	layers_string = yafaray_getLayersTable(film);
	printf("** Layers defined:\n%s\n", layers_string);
	yafaray_destroyCharString(layers_string);

	/* Rendering */
	render_monitor = yafaray_createRenderMonitor(monitorCallback, &total_steps, YAFARAY_DISPLAY_CONSOLE_NORMAL);
	render_control = yafaray_createRenderControl();
	scene_modified_flags = yafaray_checkAndClearSceneModifiedFlags(scene);
	yafaray_preprocessScene(scene, render_control, scene_modified_flags);
	yafaray_preprocessSurfaceIntegrator(render_control, render_monitor, surface_integrator, scene);
	yafaray_render(render_control, render_monitor, surface_integrator, film, YAFARAY_RENDER_NORMAL);
	printf("END: total_steps = %d\n", total_steps);
	yafaray_destroyRenderControl(render_control);
	yafaray_destroyRenderMonitor(render_monitor);

	/* Destruction/deallocation */
	yafaray_destroySurfaceIntegrator(surface_integrator);
	yafaray_destroyScene(scene);
	yafaray_destroyFilm(film);
	yafaray_destroyLogger(logger);
	yafaray_destroyParamMapList(param_map_list);
	yafaray_destroyParamMap(param_map);

	/* Saving rendered image */
	fp = fopen("test.ppm", "wb");
	fprintf(fp, "P6 %d %d %d ", result_image.width_, result_image.height_, 255);
	fwrite(result_image.data_, result_image_size_bytes, 1, fp);
	fclose(fp);
	free(result_image.data_);
	return 0;
}

float forceRange01(float value)
{
	if(value > 1.f) return 1.f;
	else if(value < 0.f) return 0.f;
	else return value;
}

void notifyLayerCallback(const char *internal_layer_name, const char *exported_layer_name, int width, int height, int layer_exported_channels, void *callback_data)
{
	printf("**** notifyLayerCallback internal_layer_name='%s', exported_layer_name='%s', width=%d, height=%d, layer_exported_channels=%d, callback_data=%p\n", internal_layer_name, exported_layer_name, width, height, layer_exported_channels, callback_data);
}

void putPixelCallback(const char *layer_name, int x, int y, float r, float g, float b, float a, void *callback_data)
{
	if(x % 100 == 0 && y % 100 == 0) printf("**** putPixelCallback layer_name='%s', x=%d, y=%d, rgba={%f,%f,%f,%f}, callback_data=%p\n", layer_name, x, y, r, g, b, a, callback_data);
	if(strcmp(layer_name, "combined") == 0)
	{
		struct ResultImage *result_image = (struct ResultImage *) callback_data;
		const int width = result_image->width_;
		const size_t idx = 3 * (y * width + x);
		*(result_image->data_ + idx + 0) = (char) (forceRange01(r) * 255.f);
		*(result_image->data_ + idx + 1) = (char) (forceRange01(g) * 255.f);
		*(result_image->data_ + idx + 2) = (char) (forceRange01(b) * 255.f);
	}
}

void flushAreaCallback(int area_id, int x_0, int y_0, int x_1, int y_1, void *callback_data)
{
	printf("**** flushAreaCallback area_id=%d, x_0=%d, y_0=%d, x_1=%d, y_1=%d, callback_data=%p\n", area_id, x_0, y_0, x_1, y_1, callback_data);
}

void flushCallback(void *callback_data)
{
	printf("**** flushCallback callback_data=%p\n", callback_data);
}

void highlightCallback(int area_id, int x_0, int y_0, int x_1, int y_1, void *callback_data)
{
	printf("**** highlightCallback area_id=%d, x_0=%d, y_0=%d, x_1=%d, y_1=%d, callback_data=%p\n", area_id, x_0, y_0, x_1, y_1, callback_data);
}

void monitorCallback(int steps_total, int steps_done, const char *tag, void *callback_data)
{
	*((int *) callback_data) = steps_total;
	printf("**** monitorCallback steps_total=%d, steps_done=%d, tag='%s', callback_data=%p\n", steps_total, steps_done, tag, callback_data);
}

void loggerCallback(yafaray_LogLevel log_level, size_t datetime, const char *time_of_day, const char *description, void *callback_data)
{
	printf("**** loggerCallback log_level=%d, datetime=%ld, time_of_day='%s', description='%s', callback_data=%p\n", log_level, datetime, time_of_day, description, callback_data);
}
