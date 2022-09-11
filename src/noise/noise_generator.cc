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

#include "noise/noise_generator.h"
#include "noise/generator/noise_blender.h"
#include "noise/generator/noise_cell.h"
#include "noise/generator/noise_perlin_improved.h"
#include "noise/generator/noise_perlin_standard.h"
#include "noise/generator/noise_voronoi.h"
#include "math/interpolation.h"
#include <array>

namespace yafaray {

std::unique_ptr<NoiseGenerator> NoiseGenerator::newNoise(const NoiseType &noise_type)
{
	switch(noise_type.value())
	{
		case NoiseType::Blender: return std::unique_ptr<NoiseGenerator>(new BlenderNoiseGenerator());
		case NoiseType::PerlinStandard: return std::unique_ptr<NoiseGenerator>(new StdPerlinNoiseGenerator());
		case NoiseType::VoronoiF1:
		case NoiseType::VoronoiF2:
		case NoiseType::VoronoiF3:
		case NoiseType::VoronoiF4:
		case NoiseType::VoronoiF2F1:
		case NoiseType::VoronoiCrackle: return std::unique_ptr<NoiseGenerator>(new VoronoiNoiseGenerator{noise_type, {VoronoiNoiseGenerator::DMetricType::DistReal}, 2.5f});
		case NoiseType::Cell: return std::unique_ptr<NoiseGenerator>(new CellNoiseGenerator());
		case NoiseType::PerlinImproved:
		default: return std::unique_ptr<NoiseGenerator>(new NewPerlinNoiseGenerator());
	}
}

Point3f NoiseGenerator::hashPnt(const Point3i &point)
{
	const int index = 3 * hash_[(hash_[(hash_[point[Axis::Z] & 255] + point[Axis::Y]) & 255] + point[Axis::X]) & 255];
	if(index < 0 || index > (static_cast<int>(hashpntf_.size()) - 3)) return {};
	else return {{hashpntf_[index], hashpntf_[index + 1], hashpntf_[index + 2]}};
}

// color cell noise, used by voronoi shader block
Rgba NoiseGenerator::cellNoiseColor(const Point3f &pt)
{
	const Point3i point_i {{
								   static_cast<int>(std::floor(pt[Axis::X])),
								   static_cast<int>(std::floor(pt[Axis::Y])),
								   static_cast<int>(std::floor(pt[Axis::Z]))
						   }};
	const Vec3f cl{hashPnt(point_i)};
	return {cl[0], cl[1], cl[2], 1.f};
}

// turbulence function used by basic blocks
float NoiseGenerator::turbulence(const NoiseGenerator *ngen, const Point3f &pt, int oct, float size, bool hard)
{
	float amp = 1.f, sum = 0.f;
	Point3f tp{ngen->offset(pt) * size};	// only blendernoise adds offset
	for(int i = 0; i <= oct; i++, amp *= 0.5f, tp *= 2.f)
	{
		float val = (*ngen)(tp);
		if(hard) val = std::abs(2.f * val - 1.f);
		sum += amp * val;
	}
	return sum * ((float)(1 << oct) / (float)((1 << (oct + 1)) - 1));
}

constexpr std::array<float, 768> NoiseGenerator::hashpntf_
{
	0.536902f, 0.020915f, 0.501445f, 0.216316f, 0.517036f, 0.822466f, 0.965315f,
	0.377313f, 0.678764f, 0.744545f, 0.097731f, 0.396357f, 0.247202f, 0.520897f,
	0.613396f, 0.542124f, 0.146813f, 0.255489f, 0.810868f, 0.638641f, 0.980742f,
	0.292316f, 0.357948f, 0.114382f, 0.861377f, 0.629634f, 0.722530f, 0.714103f,
	0.048549f, 0.075668f, 0.564920f, 0.162026f, 0.054466f, 0.411738f, 0.156897f,
	0.887657f, 0.599368f, 0.074249f, 0.170277f, 0.225799f, 0.393154f, 0.301348f,
	0.057434f, 0.293849f, 0.442745f, 0.150002f, 0.398732f, 0.184582f, 0.915200f,
	0.630984f, 0.974040f, 0.117228f, 0.795520f, 0.763238f, 0.158982f, 0.616211f,
	0.250825f, 0.906539f, 0.316874f, 0.676205f, 0.234720f, 0.667673f, 0.792225f,
	0.273671f, 0.119363f, 0.199131f, 0.856716f, 0.828554f, 0.900718f, 0.705960f,
	0.635923f, 0.989433f, 0.027261f, 0.283507f, 0.113426f, 0.388115f, 0.900176f,
	0.637741f, 0.438802f, 0.715490f, 0.043692f, 0.202640f, 0.378325f, 0.450325f,
	0.471832f, 0.147803f, 0.906899f, 0.524178f, 0.784981f, 0.051483f, 0.893369f,
	0.596895f, 0.275635f, 0.391483f, 0.844673f, 0.103061f, 0.257322f, 0.708390f,
	0.504091f, 0.199517f, 0.660339f, 0.376071f, 0.038880f, 0.531293f, 0.216116f,
	0.138672f, 0.907737f, 0.807994f, 0.659582f, 0.915264f, 0.449075f, 0.627128f,
	0.480173f, 0.380942f, 0.018843f, 0.211808f, 0.569701f, 0.082294f, 0.689488f,
	0.573060f, 0.593859f, 0.216080f, 0.373159f, 0.108117f, 0.595539f, 0.021768f,
	0.380297f, 0.948125f, 0.377833f, 0.319699f, 0.315249f, 0.972805f, 0.792270f,
	0.445396f, 0.845323f, 0.372186f, 0.096147f, 0.689405f, 0.423958f, 0.055675f,
	0.117940f, 0.328456f, 0.605808f, 0.631768f, 0.372170f, 0.213723f, 0.032700f,
	0.447257f, 0.440661f, 0.728488f, 0.299853f, 0.148599f, 0.649212f, 0.498381f,
	0.049921f, 0.496112f, 0.607142f, 0.562595f, 0.990246f, 0.739659f, 0.108633f,
	0.978156f, 0.209814f, 0.258436f, 0.876021f, 0.309260f, 0.600673f, 0.713597f,
	0.576967f, 0.641402f, 0.853930f, 0.029173f, 0.418111f, 0.581593f, 0.008394f,
	0.589904f, 0.661574f, 0.979326f, 0.275724f, 0.111109f, 0.440472f, 0.120839f,
	0.521602f, 0.648308f, 0.284575f, 0.204501f, 0.153286f, 0.822444f, 0.300786f,
	0.303906f, 0.364717f, 0.209038f, 0.916831f, 0.900245f, 0.600685f, 0.890002f,
	0.581660f, 0.431154f, 0.705569f, 0.551250f, 0.417075f, 0.403749f, 0.696652f,
	0.292652f, 0.911372f, 0.690922f, 0.323718f, 0.036773f, 0.258976f, 0.274265f,
	0.225076f, 0.628965f, 0.351644f, 0.065158f, 0.080340f, 0.467271f, 0.130643f,
	0.385914f, 0.919315f, 0.253821f, 0.966163f, 0.017439f, 0.392610f, 0.478792f,
	0.978185f, 0.072691f, 0.982009f, 0.097987f, 0.731533f, 0.401233f, 0.107570f,
	0.349587f, 0.479122f, 0.700598f, 0.481751f, 0.788429f, 0.706864f, 0.120086f,
	0.562691f, 0.981797f, 0.001223f, 0.192120f, 0.451543f, 0.173092f, 0.108960f,
	0.549594f, 0.587892f, 0.657534f, 0.396365f, 0.125153f, 0.666420f, 0.385823f,
	0.890916f, 0.436729f, 0.128114f, 0.369598f, 0.759096f, 0.044677f, 0.904752f,
	0.088052f, 0.621148f, 0.005047f, 0.452331f, 0.162032f, 0.494238f, 0.523349f,
	0.741829f, 0.698450f, 0.452316f, 0.563487f, 0.819776f, 0.492160f, 0.004210f,
	0.647158f, 0.551475f, 0.362995f, 0.177937f, 0.814722f, 0.727729f, 0.867126f,
	0.997157f, 0.108149f, 0.085726f, 0.796024f, 0.665075f, 0.362462f, 0.323124f,
	0.043718f, 0.042357f, 0.315030f, 0.328954f, 0.870845f, 0.683186f, 0.467922f,
	0.514894f, 0.809971f, 0.631979f, 0.176571f, 0.366320f, 0.850621f, 0.505555f,
	0.749551f, 0.750830f, 0.401714f, 0.481216f, 0.438393f, 0.508832f, 0.867971f,
	0.654581f, 0.058204f, 0.566454f, 0.084124f, 0.548539f, 0.902690f, 0.779571f,
	0.562058f, 0.048082f, 0.863109f, 0.079290f, 0.713559f, 0.783496f, 0.265266f,
	0.672089f, 0.786939f, 0.143048f, 0.086196f, 0.876129f, 0.408708f, 0.229312f,
	0.629995f, 0.206665f, 0.207308f, 0.710079f, 0.341704f, 0.264921f, 0.028748f,
	0.629222f, 0.470173f, 0.726228f, 0.125243f, 0.328249f, 0.794187f, 0.741340f,
	0.489895f, 0.189396f, 0.724654f, 0.092841f, 0.039809f, 0.860126f, 0.247701f,
	0.655331f, 0.964121f, 0.672536f, 0.044522f, 0.690567f, 0.837238f, 0.631520f,
	0.953734f, 0.352484f, 0.289026f, 0.034152f, 0.852575f, 0.098454f, 0.795529f,
	0.452181f, 0.826159f, 0.186993f, 0.820725f, 0.440328f, 0.922137f, 0.704592f,
	0.915437f, 0.738183f, 0.733461f, 0.193798f, 0.929213f, 0.161390f, 0.318547f,
	0.888751f, 0.430968f, 0.740837f, 0.193544f, 0.872253f, 0.563074f, 0.274598f,
	0.347805f, 0.666176f, 0.449831f, 0.800991f, 0.588727f, 0.052296f, 0.714761f,
	0.420620f, 0.570325f, 0.057550f, 0.210888f, 0.407312f, 0.662848f, 0.924382f,
	0.895958f, 0.775198f, 0.688605f, 0.025721f, 0.301913f, 0.791408f, 0.500602f,
	0.831984f, 0.828509f, 0.642093f, 0.494174f, 0.525880f, 0.446365f, 0.440063f,
	0.763114f, 0.630358f, 0.223943f, 0.333806f, 0.906033f, 0.498306f, 0.241278f,
	0.427640f, 0.772683f, 0.198082f, 0.225379f, 0.503894f, 0.436599f, 0.016503f,
	0.803725f, 0.189878f, 0.291095f, 0.499114f, 0.151573f, 0.079031f, 0.904618f,
	0.708535f, 0.273900f, 0.067419f, 0.317124f, 0.936499f, 0.716511f, 0.543845f,
	0.939909f, 0.826574f, 0.715090f, 0.154864f, 0.750150f, 0.845808f, 0.648108f,
	0.556564f, 0.644757f, 0.140873f, 0.799167f, 0.632989f, 0.444245f, 0.471978f,
	0.435910f, 0.359793f, 0.216241f, 0.007633f, 0.337236f, 0.857863f, 0.380247f,
	0.092517f, 0.799973f, 0.919000f, 0.296798f, 0.096989f, 0.854831f, 0.165369f,
	0.568475f, 0.216855f, 0.020457f, 0.835511f, 0.538039f, 0.999742f, 0.620226f,
	0.244053f, 0.060399f, 0.323007f, 0.294874f, 0.988899f, 0.384919f, 0.735655f,
	0.773428f, 0.549776f, 0.292882f, 0.660611f, 0.593507f, 0.621118f, 0.175269f,
	0.682119f, 0.794493f, 0.868197f, 0.632150f, 0.807823f, 0.509656f, 0.482035f,
	0.001780f, 0.259126f, 0.358002f, 0.280263f, 0.192985f, 0.290367f, 0.208111f,
	0.917633f, 0.114422f, 0.925491f, 0.981110f, 0.255570f, 0.974862f, 0.016629f,
	0.552599f, 0.575741f, 0.612978f, 0.615965f, 0.803615f, 0.772334f, 0.089745f,
	0.838812f, 0.634542f, 0.113709f, 0.755832f, 0.577589f, 0.667489f, 0.529834f,
	0.325660f, 0.817597f, 0.316557f, 0.335093f, 0.737363f, 0.260951f, 0.737073f,
	0.049540f, 0.735541f, 0.988891f, 0.299116f, 0.147695f, 0.417271f, 0.940811f,
	0.524160f, 0.857968f, 0.176403f, 0.244835f, 0.485759f, 0.033353f, 0.280319f,
	0.750688f, 0.755809f, 0.924208f, 0.095956f, 0.962504f, 0.275584f, 0.173715f,
	0.942716f, 0.706721f, 0.078464f, 0.576716f, 0.804667f, 0.559249f, 0.900611f,
	0.646904f, 0.432111f, 0.927885f, 0.383277f, 0.269973f, 0.114244f, 0.574867f,
	0.150703f, 0.241855f, 0.272871f, 0.199950f, 0.079719f, 0.868566f, 0.962833f,
	0.789122f, 0.320025f, 0.905554f, 0.234876f, 0.991356f, 0.061913f, 0.732911f,
	0.785960f, 0.874074f, 0.069035f, 0.658632f, 0.309901f, 0.023676f, 0.791603f,
	0.764661f, 0.661278f, 0.319583f, 0.829650f, 0.117091f, 0.903124f, 0.982098f,
	0.161631f, 0.193576f, 0.670428f, 0.857390f, 0.003760f, 0.572578f, 0.222162f,
	0.114551f, 0.420118f, 0.530404f, 0.470682f, 0.525527f, 0.764281f, 0.040596f,
	0.443275f, 0.501124f, 0.816161f, 0.417467f, 0.332172f, 0.447565f, 0.614591f,
	0.559246f, 0.805295f, 0.226342f, 0.155065f, 0.714630f, 0.160925f, 0.760001f,
	0.453456f, 0.093869f, 0.406092f, 0.264801f, 0.720370f, 0.743388f, 0.373269f,
	0.403098f, 0.911923f, 0.897249f, 0.147038f, 0.753037f, 0.516093f, 0.739257f,
	0.175018f, 0.045768f, 0.735857f, 0.801330f, 0.927708f, 0.240977f, 0.591870f,
	0.921831f, 0.540733f, 0.149100f, 0.423152f, 0.806876f, 0.397081f, 0.061100f,
	0.811630f, 0.044899f, 0.460915f, 0.961202f, 0.822098f, 0.971524f, 0.867608f,
	0.773604f, 0.226616f, 0.686286f, 0.926972f, 0.411613f, 0.267873f, 0.081937f,
	0.226124f, 0.295664f, 0.374594f, 0.533240f, 0.237876f, 0.669629f, 0.599083f,
	0.513081f, 0.878719f, 0.201577f, 0.721296f, 0.495038f, 0.079760f, 0.965959f,
	0.233090f, 0.052496f, 0.714748f, 0.887844f, 0.308724f, 0.972885f, 0.723337f,
	0.453089f, 0.914474f, 0.704063f, 0.823198f, 0.834769f, 0.906561f, 0.919600f,
	0.100601f, 0.307564f, 0.901977f, 0.468879f, 0.265376f, 0.885188f, 0.683875f,
	0.868623f, 0.081032f, 0.466835f, 0.199087f, 0.663437f, 0.812241f, 0.311337f,
	0.821361f, 0.356628f, 0.898054f, 0.160781f, 0.222539f, 0.714889f, 0.490287f,
	0.984915f, 0.951755f, 0.964097f, 0.641795f, 0.815472f, 0.852732f, 0.862074f,
	0.051108f, 0.440139f, 0.323207f, 0.517171f, 0.562984f, 0.115295f, 0.743103f,
	0.977914f, 0.337596f, 0.440694f, 0.535879f, 0.959427f, 0.351427f, 0.704361f,
	0.010826f, 0.131162f, 0.577080f, 0.349572f, 0.774892f, 0.425796f, 0.072697f,
	0.500001f, 0.267322f, 0.909654f, 0.206176f, 0.223987f, 0.937698f, 0.323423f,
	0.117501f, 0.490308f, 0.474372f, 0.689943f, 0.168671f, 0.719417f, 0.188928f,
	0.330464f, 0.265273f, 0.446271f, 0.171933f, 0.176133f, 0.474616f, 0.140182f,
	0.114246f, 0.905043f, 0.713870f, 0.555261f, 0.951333
};

const std::array<unsigned char, 512> NoiseGenerator::hash_
{
	0xA2, 0xA0, 0x19, 0x3B, 0xF8, 0xEB, 0xAA, 0xEE, 0xF3, 0x1C, 0x67, 0x28, 0x1D, 0xED, 0x0, 0xDE, 0x95, 0x2E, 0xDC, 0x3F, 0x3A, 0x82, 0x35, 0x4D, 0x6C, 0xBA, 0x36, 0xD0, 0xF6, 0xC, 0x79, 0x32, 0xD1, 0x59, 0xF4, 0x8, 0x8B, 0x63, 0x89, 0x2F, 0xB8, 0xB4, 0x97, 0x83, 0xF2, 0x8F, 0x18, 0xC7, 0x51, 0x14, 0x65, 0x87, 0x48, 0x20, 0x42, 0xA8, 0x80, 0xB5, 0x40, 0x13, 0xB2, 0x22, 0x7E, 0x57,
	0xBC, 0x7F, 0x6B, 0x9D, 0x86, 0x4C, 0xC8, 0xDB, 0x7C, 0xD5, 0x25, 0x4E, 0x5A, 0x55, 0x74, 0x50, 0xCD, 0xB3, 0x7A, 0xBB, 0xC3, 0xCB, 0xB6, 0xE2, 0xE4, 0xEC, 0xFD, 0x98, 0xB, 0x96, 0xD3, 0x9E, 0x5C, 0xA1, 0x64, 0xF1, 0x81, 0x61, 0xE1, 0xC4, 0x24, 0x72, 0x49, 0x8C, 0x90, 0x4B, 0x84, 0x34, 0x38, 0xAB, 0x78, 0xCA, 0x1F, 0x1, 0xD7, 0x93, 0x11, 0xC1, 0x58, 0xA9, 0x31, 0xF9, 0x44, 0x6D,
	0xBF, 0x33, 0x9C, 0x5F, 0x9, 0x94, 0xA3, 0x85, 0x6, 0xC6, 0x9A, 0x1E, 0x7B, 0x46, 0x15, 0x30, 0x27, 0x2B, 0x1B, 0x71, 0x3C, 0x5B, 0xD6, 0x6F, 0x62, 0xAC, 0x4F, 0xC2, 0xC0, 0xE, 0xB1, 0x23, 0xA7, 0xDF, 0x47, 0xB0, 0x77, 0x69, 0x5, 0xE9, 0xE6, 0xE7, 0x76, 0x73, 0xF, 0xFE, 0x6E, 0x9B, 0x56, 0xEF, 0x12, 0xA5, 0x37, 0xFC, 0xAE, 0xD9, 0x3, 0x8E, 0xDD, 0x10, 0xB9, 0xCE, 0xC9, 0x8D,
	0xDA, 0x2A, 0xBD, 0x68, 0x17, 0x9F, 0xBE, 0xD4, 0xA, 0xCC, 0xD2, 0xE8, 0x43, 0x3D, 0x70, 0xB7, 0x2, 0x7D, 0x99, 0xD8, 0xD, 0x60, 0x8A, 0x4, 0x2C, 0x3E, 0x92, 0xE5, 0xAF, 0x53, 0x7, 0xE0, 0x29, 0xA6, 0xC5, 0xE3, 0xF5, 0xF7, 0x4A, 0x41, 0x26, 0x6A, 0x16, 0x5E, 0x52, 0x2D, 0x21, 0xAD, 0xF0, 0x91, 0xFF, 0xEA, 0x54, 0xFA, 0x66, 0x1A, 0x45, 0x39, 0xCF, 0x75, 0xA4, 0x88, 0xFB, 0x5D,
	0xA2, 0xA0, 0x19, 0x3B, 0xF8, 0xEB, 0xAA, 0xEE, 0xF3, 0x1C, 0x67, 0x28, 0x1D, 0xED, 0x0, 0xDE, 0x95, 0x2E, 0xDC, 0x3F, 0x3A, 0x82, 0x35, 0x4D, 0x6C, 0xBA, 0x36, 0xD0, 0xF6, 0xC, 0x79, 0x32, 0xD1, 0x59, 0xF4, 0x8, 0x8B, 0x63, 0x89, 0x2F, 0xB8, 0xB4, 0x97, 0x83, 0xF2, 0x8F, 0x18, 0xC7, 0x51, 0x14, 0x65, 0x87, 0x48, 0x20, 0x42, 0xA8, 0x80, 0xB5, 0x40, 0x13, 0xB2, 0x22, 0x7E, 0x57,
	0xBC, 0x7F, 0x6B, 0x9D, 0x86, 0x4C, 0xC8, 0xDB, 0x7C, 0xD5, 0x25, 0x4E, 0x5A, 0x55, 0x74, 0x50, 0xCD, 0xB3, 0x7A, 0xBB, 0xC3, 0xCB, 0xB6, 0xE2, 0xE4, 0xEC, 0xFD, 0x98, 0xB, 0x96, 0xD3, 0x9E, 0x5C, 0xA1, 0x64, 0xF1, 0x81, 0x61, 0xE1, 0xC4, 0x24, 0x72, 0x49, 0x8C, 0x90, 0x4B, 0x84, 0x34, 0x38, 0xAB, 0x78, 0xCA, 0x1F, 0x1, 0xD7, 0x93, 0x11, 0xC1, 0x58, 0xA9, 0x31, 0xF9, 0x44, 0x6D,
	0xBF, 0x33, 0x9C, 0x5F, 0x9, 0x94, 0xA3, 0x85, 0x6, 0xC6, 0x9A, 0x1E, 0x7B, 0x46, 0x15, 0x30, 0x27, 0x2B, 0x1B, 0x71, 0x3C, 0x5B, 0xD6, 0x6F, 0x62, 0xAC, 0x4F, 0xC2, 0xC0, 0xE, 0xB1, 0x23, 0xA7, 0xDF, 0x47, 0xB0, 0x77, 0x69, 0x5, 0xE9, 0xE6, 0xE7, 0x76, 0x73, 0xF, 0xFE, 0x6E, 0x9B, 0x56, 0xEF, 0x12, 0xA5, 0x37, 0xFC, 0xAE, 0xD9, 0x3, 0x8E, 0xDD, 0x10, 0xB9, 0xCE, 0xC9, 0x8D,
	0xDA, 0x2A, 0xBD, 0x68, 0x17, 0x9F, 0xBE, 0xD4, 0xA, 0xCC, 0xD2, 0xE8, 0x43, 0x3D, 0x70, 0xB7, 0x2, 0x7D, 0x99, 0xD8, 0xD, 0x60, 0x8A, 0x4, 0x2C, 0x3E, 0x92, 0xE5, 0xAF, 0x53, 0x7, 0xE0, 0x29, 0xA6, 0xC5, 0xE3, 0xF5, 0xF7, 0x4A, 0x41, 0x26, 0x6A, 0x16, 0x5E, 0x52, 0x2D, 0x21, 0xAD, 0xF0, 0x91, 0xFF, 0xEA, 0x54, 0xFA, 0x66, 0x1A, 0x45, 0x39, 0xCF, 0x75, 0xA4, 0x88, 0xFB, 0x5D,
};

} //namespace yafaray
