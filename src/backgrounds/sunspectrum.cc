#include<yafray_config.h>

#include <yafraycore/spectrum.h>
#include <algorithm>
#include <vector>

__BEGIN_YAFRAY


// k_o Spectrum table from pg 127, MI.
static const float k_oWavelengths[64] = {
	300, 305, 310, 315, 320,
	325, 330, 335, 340, 345,
	350, 355,

	445, 450, 455, 460, 465,
	470, 475, 480, 485, 490,
	495,

	500, 505, 510, 515, 520,
	525, 530, 535, 540, 545,
	550, 555, 560, 565, 570,
	575, 580, 585, 590, 595,

	600, 605, 610, 620, 630,
	640, 650, 660, 670, 680,
	690,

	700, 710, 720, 730, 740,
	750, 760, 770, 780, 790,
};




static const float k_oAmplitudes[64] = {
	10.0,  4.8,  2.7,   1.35,  .8,
	.380,  .160,  .075,  .04,  .019,
	.007,  .0,

	.003,  .003,  .004,  .006,  .008,
	.009,  .012,  .014,	.017,  .021,
	.025,

	.03,  .035,  .04,  .045,  .048,
	.057,  .063, .07,  .075,  .08,
	.085,  .095, .103, .110,  .12,
	.122,  .12,  .118,  .115,  .12,
	
	.125,  .130,  .12,  .105,  .09,
	.079,	.067, .057,  .048,  .036,
	.028,

	.023,  .018,  .014,  .011, .010,
	.009,	.007,  .004,  .0,  .0
};


// k_g Spectrum table from pg 130, MI.
static const float k_gWavelengths[4] = {
  759,  760,  770,  771
};

static const float k_gAmplitudes[4] = {
  0,  3.0,  0.210,  0
};

// k_wa Spectrum table from pg 130, MI.
static const float k_waWavelengths[13] = {
  689,  690,  700,  710,  720,  730,  740,
  750,  760,  770,  780,  790,  800
};

static const float k_waAmplitudes[13] = {
  0,
  0.160e-1,
  0.240e-1,
  0.125e-1,
  0.100e+1,
  0.870,
  0.610e-1,
  0.100e-2,
  0.100e-4,
  0.100e-4,
  0.600e-3,
  0.175e-1,
  0.360e-1
};


// 380-750 by 10nm
static const float solAmplitudes[38] = {
    165.5, 162.3, 211.2, 258.8, 258.2,
    242.3, 267.6, 296.6, 305.4, 300.6,
    306.6, 288.3, 287.1, 278.2, 271.0,
    272.3, 263.6, 255.0, 250.6, 253.1,
    253.5, 251.3, 246.3, 241.7, 236.8,
    232.1, 228.2, 223.4, 219.7, 215.3,
    211.0, 207.3, 202.4, 198.7, 194.3,
    190.7, 186.3, 182.6
};


struct  irregularSpectrum_t
{
	irregularSpectrum_t(const float *amps, const float *wl, int n)
	{
		for(int i=0; i<n; ++i){ wavelen.push_back(wl[i]); amplitude.push_back(amps[i]); }
	}
	float sample(float wl);
	std::vector<float> wavelen;
	std::vector<float> amplitude;
};

float irregularSpectrum_t::sample(float wl)
{
	std::vector<float>::const_iterator i;
	i = lower_bound(wavelen.begin(), wavelen.end(), wl);
	if(i == wavelen.begin() || i == wavelen.end()) return 0.f;
	int index = (i-wavelen.begin()) - 1;
	float delta = (wl - wavelen[index]) / (wavelen[index+1] - wavelen[index]);
	return (1.f - delta) * amplitude[index] + delta * amplitude[index+1];
}

color_t ComputeAttenuatedSunlight(float theta, int turbidity)
{
    irregularSpectrum_t k_oCurve(k_oAmplitudes, k_oWavelengths, 64);
    irregularSpectrum_t k_gCurve(k_gAmplitudes, k_gWavelengths, 4);
    irregularSpectrum_t k_waCurve(k_waAmplitudes, k_waWavelengths, 13);
    //RiRegularSpectralCurve   solCurve(solAmplitudes, 380, 750, 38);  // every 10 nm  IN WRONG UNITS
				                                     // Need a factor of 100 (done below)
    float data[38];  // (750 - 380) / 10  + 1

    float beta = 0.04608365822050 * turbidity - 0.04586025928522;
    float tauR, tauA, tauO, tauG, tauWA;
	const float alpha = 1.3, lOzone = .35, w = 2.0;
	
	color_t sun_xyz(0.f);
    float m = 1.0/(cos(theta) + 0.000940 * pow(1.6386f - theta,-1.253f));  // Relative Optical Mass

	int i;
	float lambda;
	for(i = 0, lambda = 380; i < 38; i++, lambda+=10)
	{
		float uL = lambda * 0.001f;
				// Rayleigh Scattering
				// Results agree with the graph (pg 115, MI) */
		tauR = fExp( -m * 0.008735 * pow(uL, -4.08f));
				// Aerosal (water + dust) attenuation
				// beta - amount of aerosols present 
				// alpha - ratio of small to large particle sizes. (0:4,usually 1.3)
				// Results agree with the graph (pg 121, MI) 
		tauA = fExp(-m * beta * pow(uL, -alpha));  // lambda should be in um
				// Attenuation due to ozone absorption  
				// lOzone - amount of ozone in cm(NTP) 
				// Results agree with the graph (pg 128, MI) 
		tauO = fExp(-m * k_oCurve.sample(lambda) * lOzone);
				// Attenuation due to mixed gases absorption  
				// Results agree with the graph (pg 131, MI)
		tauG = fExp(-1.41 * k_gCurve.sample(lambda) * m / pow(1 + 118.93 * k_gCurve.sample(lambda) * m, 0.45));
				// Attenuation due to water vapor absorbtion  
				// w - precipitable water vapor in centimeters (standard = 2) 
				// Results agree with the graph (pg 132, MI)
		tauWA = fExp(-0.2385 * k_waCurve.sample(lambda) * w * m /
				fPow(1 + 20.07 * k_waCurve.sample(lambda) * w * m, 0.45));
		
		data[i] = 100 * solAmplitudes[i] * tauR * tauA * tauO * tauG * tauWA; // 100 comes from solCurve being
																		 // in wrong units.
		sun_xyz += wl2XYZ(lambda) * data[i];
	}
	sun_xyz *= 0.02631578947368421053f;
	color_t sun_col;
	sun_col.set(( 3.240479 * sun_xyz.R - 1.537150 * sun_xyz.G - 0.498535 * sun_xyz.B),
				(-0.969256 * sun_xyz.R + 1.875992 * sun_xyz.G + 0.041556 * sun_xyz.B),
				( 0.055648 * sun_xyz.R - 0.204043 * sun_xyz.G + 1.057311 * sun_xyz.B));
	return sun_col;
}

__END_YAFRAY
