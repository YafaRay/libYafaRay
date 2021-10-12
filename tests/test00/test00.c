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

void notifyViewCallback(const char *view_name, void *callback_data);
void notifyLayerCallback(const char *internal_layer_name, const char *exported_layer_name, int width, int height, int layer_exported_channels, void *callback_data);
void putPixelCallback(const char *view_name, const char *layer_name, int x, int y, float r, float g, float b, float a, void *callback_data);
void flushAreaCallback(const char *view_name, int area_id, int x_0, int y_0, int x_1, int y_1, void *callback_data);
void flushCallback(const char *view_name, void *callback_data);
void highlightCallback(const char *view_name, int area_id, int x_0, int y_0, int x_1, int y_1, void *callback_data);
void monitorCallback(int steps_total, int steps_done, const char *tag, void *callback_data);
void loggerCallback(yafaray_LogLevel_t log_level, long datetime, const char *time_of_day, const char *description, void *callback_data);

yafaray_Interface_t *yi = NULL;

#ifdef _WIN32
BOOL WINAPI ctrlCHandler_global(DWORD signal)
{
	yafaray_printWarning(yi, "CTRL+C pressed, cancelling.\n");
	if(yi)
	{
		yafaray_cancelRendering(yi);
		return TRUE;
	}
	else exit(1);
}
#else
void ctrlCHandler_global(int signal)
{
	yafaray_printWarning(yi, "CTRL+C pressed, cancelling.\n");
	if(yi) yafaray_cancelRendering(yi);
	else exit(1);
}
#endif

int main()
{
	struct ResultImage result_image;
	size_t result_image_size_bytes;
	int total_steps = 0;
	char *version_string = NULL;
	char *layers_string = NULL;
	char *views_string = NULL;

	printf("***** Test client 'test00' for libYafaRay *****\n");
	printf("Using libYafaRay version (%d.%d.%d)\n", yafaray_getVersionMajor(), yafaray_getVersionMinor(), yafaray_getVersionPatch());
	version_string = yafaray_getVersionString();
	printf("    libYafaRay version details: '%s'\n\n", version_string);
	yafaray_deallocateCharPointer(version_string);

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
	yi = yafaray_createInterface(YAFARAY_INTERFACE_FOR_RENDERING, "test00.xml", loggerCallback, &result_image, YAFARAY_DISPLAY_CONSOLE_NORMAL);
	yafaray_setConsoleLogColorsEnabled(yi, YAFARAY_BOOL_TRUE);
	yafaray_setConsoleVerbosityLevel(yi, YAFARAY_LOG_LEVEL_DEBUG);

	/* Creating scene */
	yafaray_createScene(yi);
	yafaray_paramsClearAll(yi);
	{
		/* Creating image from RAM or file */
		const int tex_width = 200;
		const int tex_height = 200;
		yafaray_Image_t *image = NULL;
		int i, j;

		yafaray_paramsSetString(yi, "type", "ColorAlpha");
		yafaray_paramsSetString(yi, "image_optimization", "none"); /* Note: only "none" allows more HDR values > 1.f */
		yafaray_paramsSetInt(yi, "tex_width", tex_width);
		yafaray_paramsSetInt(yi, "tex_height", tex_height);
		yafaray_paramsSetString(yi, "filename", "tex.tga");
		image = yafaray_createImage(yi, "Image01");
		yafaray_paramsClearAll(yi);

		for(i = 0; i < tex_width; ++i)
			for(j = 0; j < tex_height; ++j)
				yafaray_setImageColor(image, i, j, 0.01f * i, 0.01f * j, 0.01f * (i + j), 1.f);
	}

	/* Creating texture from image */
	yafaray_paramsSetString(yi, "type", "image");
	yafaray_paramsSetString(yi, "image_name", "Image01");
	yafaray_createTexture(yi, "TextureTGA");
	yafaray_paramsClearAll(yi);

	/* Creating material */
	/* General material parameters */
	yafaray_paramsSetString(yi, "type", "shinydiffusemat");
	yafaray_paramsSetColor(yi, "color", 0.9f, 0.9f, 0.9f, 1.f);
	/* Shader tree definition */
	yafaray_paramsPushList(yi);
	yafaray_paramsSetString(yi, "element", "shader_node");
	yafaray_paramsSetString(yi, "name", "diff_layer0");
	yafaray_paramsSetString(yi, "input", "map0");
	yafaray_paramsSetString(yi, "type", "layer");
	yafaray_paramsSetString(yi, "blend_mode", "mix");
	yafaray_paramsSetColor(yi, "upper_color", 1.f,1.f,1.f,1.f);
	yafaray_paramsPushList(yi);
	yafaray_paramsSetString(yi, "element", "shader_node");
	yafaray_paramsSetString(yi, "name", "map0");
	yafaray_paramsSetString(yi, "type", "texture_mapper");
	yafaray_paramsSetString(yi, "mapping", "cube");
	yafaray_paramsSetString(yi, "texco", "orco");
	yafaray_paramsSetString(yi, "texture", "TextureTGA");
	yafaray_paramsEndList(yi);
	/* Actual material creation */
	yafaray_paramsSetString(yi, "diffuse_shader", "diff_layer0");
	yafaray_createMaterial(yi, "MaterialTGA");
	yafaray_paramsClearAll(yi);

	/* Creating geometric objects in the scene */
	yafaray_startGeometry(yi);

	/* Creating a geometric object */
	yafaray_paramsSetBool(yi, "has_orco", 1);
	yafaray_paramsSetString(yi, "type", "mesh");
	yafaray_createObject(yi, "Cube");
	yafaray_paramsClearAll(yi);
	/* Creating vertices for the object */
	yafaray_addVertexWithOrco(yi, -4.f, 1.5f, 0.f, -1.f, -1.f, -1);
	yafaray_addVertexWithOrco(yi, -4.f, 1.5f, 2.f, -1.f, -1.f, 1);
	yafaray_addVertexWithOrco(yi, -4.f, 3.5f, 0.f, -1.f, 1.f, -1);
	yafaray_addVertexWithOrco(yi, -4.f, 3.5f, 2.f, -1.f, 1.f, 1);
	yafaray_addVertexWithOrco(yi, -2.f, 1.5f, 0.f, 1.f, -1.f, -1);
	yafaray_addVertexWithOrco(yi, -2.f, 1.5f, 2.f, 1.f, -1.f, 1);
	yafaray_addVertexWithOrco(yi, -2.f, 3.5f, 0.f, 1.f, 1.f, -1);
	yafaray_addVertexWithOrco(yi, -2.f, 3.5f, 2.f, 1.f, 1.f, 1);
	/* Setting up material for the faces (each face or group of faces can have different materials assigned) */
	yafaray_setCurrentMaterial(yi, "MaterialTGA");
	/* Adding faces indicating the vertices indices used in each face */
	yafaray_addTriangle(yi, 2, 0, 1);
	yafaray_addTriangle(yi, 2, 1, 3);
	yafaray_addTriangle(yi, 3, 7, 6);
	yafaray_addTriangle(yi, 3, 6, 2);
	yafaray_addTriangle(yi, 7, 5, 4);
	yafaray_addTriangle(yi, 7, 4, 6);
	yafaray_addTriangle(yi, 0, 4, 5);
	yafaray_addTriangle(yi, 0, 5, 1);
	yafaray_addTriangle(yi, 0, 2, 6);
	yafaray_addTriangle(yi, 0, 6, 4);
	yafaray_addTriangle(yi, 5, 7, 3);
	yafaray_addTriangle(yi, 5, 3, 1);

	/* Ending definition of geometric objects */
	yafaray_endGeometry(yi);

	/* Creating light/lamp */
	yafaray_paramsSetString(yi, "type", "pointlight");
	yafaray_paramsSetColor(yi, "color", 1.f, 1.f, 1.f, 1.f);
	yafaray_paramsSetVector(yi, "from", 5.3f, -4.9f, 8.9f);
	yafaray_paramsSetFloat(yi, "power", 150.f);
	yafaray_createLight(yi, "light_1");
	yafaray_paramsClearAll(yi);

	/* Creating scene background */
	yafaray_paramsSetString(yi, "type", "constant");
	yafaray_paramsSetColor(yi, "color", 1.f, 1.f, 1.f, 1.f);
	yafaray_createBackground(yi, "world_background");
	yafaray_paramsClearAll(yi);

	/* Creating camera */
	yafaray_paramsSetString(yi, "type", "perspective");
	yafaray_paramsSetInt(yi, "resx", result_image.width_);
	yafaray_paramsSetInt(yi, "resy", result_image.height_);
	yafaray_paramsSetFloat(yi, "focal", 1.1f);
	yafaray_paramsSetVector(yi, "from", 8.6f, -7.2f, 8.1f);
	yafaray_paramsSetVector(yi, "to", 8.0f, -6.7f, 7.6f);
	yafaray_paramsSetVector(yi, "up", 8.3f, -6.8f, 9.f);
	yafaray_createCamera(yi, "cam_1");
	yafaray_paramsClearAll(yi);

	/* Creating scene view */
	yafaray_paramsSetString(yi, "camera_name", "cam_1");
	yafaray_createRenderView(yi, "view_1");
	yafaray_paramsClearAll(yi);

	/* Creating image outputs */
	yafaray_paramsSetString(yi, "image_path", "./test00-output1.tga");
	yafaray_createOutput(yi, "output1_tga");
	yafaray_paramsClearAll(yi);

	yafaray_paramsSetString(yi, "image_path", "./test00-output2.tga");
	yafaray_paramsSetString(yi, "color_space", "Raw_Manual_Gamma");
	yafaray_paramsSetFloat(yi, "gamma", 4.0);
	yafaray_paramsSetBool(yi, "denoise_enabled", YAFARAY_BOOL_TRUE);
	yafaray_createOutput(yi, "output2_tga");
	yafaray_paramsClearAll(yi);

	/* Creating surface integrator */
	/*yafaray_paramsSetString(yi, "type", "directlighting");*/
	yafaray_paramsSetString(yi, "type", "photonmapping");
	yafaray_createIntegrator(yi, "surfintegr");
	yafaray_paramsClearAll(yi);

	/* Creating volume integrator */
	yafaray_paramsSetString(yi, "type", "none");
	yafaray_createIntegrator(yi, "volintegr");
	yafaray_paramsClearAll(yi);

	/* Setting up Film callbacks, must be done before yafaray_setupRender() */
	yafaray_setRenderNotifyViewCallback(yi, notifyViewCallback, (void *) &result_image);
	yafaray_setRenderNotifyLayerCallback(yi, notifyLayerCallback, (void *) &result_image);
	yafaray_setRenderPutPixelCallback(yi, putPixelCallback, (void *) &result_image);
	yafaray_setRenderFlushAreaCallback(yi, flushAreaCallback, (void *) &result_image);
	yafaray_setRenderFlushCallback(yi, flushCallback, (void *) &result_image);
	yafaray_setRenderHighlightAreaCallback(yi, highlightCallback, (void *) &result_image);

	/* Setting up render parameters */
	yafaray_paramsSetString(yi, "integrator_name", "surfintegr");
	yafaray_paramsSetString(yi, "volintegrator_name", "volintegr");
	yafaray_paramsSetString(yi, "scene_accelerator", "yafaray-kdtree-original");
	yafaray_paramsSetString(yi, "background_name", "world_background");
	yafaray_paramsSetInt(yi, "width", result_image.width_);
	yafaray_paramsSetInt(yi, "height", result_image.height_);
	yafaray_paramsSetString(yi, "film_load_save_mode", "load-save");
	/*yafaray_paramsSetString(yi, "film_load_save_path", "???");*/
	yafaray_paramsSetInt(yi, "AA_minsamples",  50);
	yafaray_paramsSetInt(yi, "AA_passes",  2);
	yafaray_paramsSetInt(yi, "threads", -1);
	yafaray_paramsSetInt(yi, "threads_photons", -1);
	yafaray_setupRender(yi);
	yafaray_paramsClearAll(yi);

	layers_string = yafaray_getLayersTable(yi);
	views_string = yafaray_getViewsTable(yi);
	printf("** Layers defined:\n%s\n", layers_string);
	printf("** Views defined:\n%s\n", views_string);
	yafaray_deallocateCharPointer(layers_string);
	yafaray_deallocateCharPointer(views_string);

	/* Rendering */
	yafaray_render(yi, monitorCallback, &total_steps, YAFARAY_DISPLAY_CONSOLE_NORMAL);
	printf("END: total_steps = %d\n", total_steps);

	/* Destroying YafaRay interface. Scene and all objects inside are automatically destroyed */
	yafaray_destroyInterface(yi);
	{
		FILE *fp = fopen("test.ppm", "wb");
		fprintf(fp, "P6 %d %d %d ", result_image.width_, result_image.height_, 255);
		fwrite(result_image.data_, result_image_size_bytes, 1, fp);
		fclose(fp);
	}
	free(result_image.data_);
	return 0;
}

float forceRange01(float value)
{
	if(value > 1.f) return 1.f;
	else if(value < 0.f) return 0.f;
	else return value;
}

void notifyViewCallback(const char *view_name, void *callback_data)
{
	printf("**** notifyViewCallback view_name='%s', callback_data=%p\n", view_name, callback_data);
}

void notifyLayerCallback(const char *internal_layer_name, const char *exported_layer_name, int width, int height, int layer_exported_channels, void *callback_data)
{
	printf("**** notifyLayerCallback internal_layer_name='%s', exported_layer_name='%s', width=%d, height=%d, layer_exported_channels=%d, callback_data=%p\n", internal_layer_name, exported_layer_name, width, height, layer_exported_channels, callback_data);
}

void putPixelCallback(const char *view_name, const char *layer_name, int x, int y, float r, float g, float b, float a, void *callback_data)
{
	if(x % 100 == 0 && y % 100 == 0) printf("**** putPixelCallback view_name='%s', layer_name='%s', x=%d, y=%d, rgba={%f,%f,%f,%f}, callback_data=%p\n", view_name, layer_name, x, y, r, g, b, a, callback_data);
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

void flushAreaCallback(const char *view_name, int area_id, int x_0, int y_0, int x_1, int y_1, void *callback_data)
{
	printf("**** flushAreaCallback view_name='%s', area_id=%d, x_0=%d, y_0=%d, x_1=%d, y_1=%d, callback_data=%p\n", view_name, area_id, x_0, y_0, x_1, y_1, callback_data);
}

void flushCallback(const char *view_name, void *callback_data)
{
	printf("**** flushCallback view_name='%s', callback_data=%p\n", view_name, callback_data);
}

void highlightCallback(const char *view_name, int area_id, int x_0, int y_0, int x_1, int y_1, void *callback_data)
{
	printf("**** highlightCallback view_name='%s', area_id=%d, x_0=%d, y_0=%d, x_1=%d, y_1=%d, callback_data=%p\n", view_name, area_id, x_0, y_0, x_1, y_1, callback_data);
}

void monitorCallback(int steps_total, int steps_done, const char *tag, void *callback_data)
{
	*((int *) callback_data) = steps_total;
	printf("**** monitorCallback steps_total=%d, steps_done=%d, tag='%s', callback_data=%p\n", steps_total, steps_done, tag, callback_data);
}

void loggerCallback(yafaray_LogLevel_t log_level, long datetime, const char *time_of_day, const char *description, void *callback_data)
{
	printf("**** loggerCallback log_level=%d, datetime=%ld, time_of_day='%s', description='%s', callback_data=%p\n", log_level, datetime, time_of_day, description, callback_data);
}
