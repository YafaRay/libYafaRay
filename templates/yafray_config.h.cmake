#ifndef Y_CONFIG_H
#define Y_CONFIG_H
#include "yafray_constants.h"

#define MIN_RAYDIST 0.00005
#define YAF_SHADOW_BIAS 0.0005

#define Y_DEBUG(DebugLevel) if (DebugLevel <= env->Debug) std::cout << "DEBUG["<< DebugLevel << "]: "
#define Y_INFO std::cout << "INFO: "
#define Y_WARNING std::cout << "WARNING: "
#define Y_ERROR std::cout << "ERROR: "

__BEGIN_YAFRAY
typedef float CFLOAT;
typedef float GFLOAT;
typedef float PFLOAT;
__END_YAFRAY
#include "yafray_user_config.h"
#endif
