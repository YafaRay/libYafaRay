libYafaRay uses CMake for the building process in Linux, Windows and MacOSX systems.

Dependencies:
  * Required
    * C++11 compliant compiler
    * \>=cmake-3.14

  * Optional, depending on options selected in CMake:
    * \>=zlib-1.2.8
    * \>=opencv-3.1.0
    * \>=freetype-2.4.0
    * \>=libpng-1.2.56
    * \>=jpeg-6b
    * \>=tiff-4.0.3
    * \>=ilmbase-2.2.0
    * \>=openexr-2.2.0

Notes about dependencies:
 * ZLib is needed if libPNG or OpenEXR are used.
 * OpenCV is used to do some image processing, most importantly LDR denoise, edge/toon layers and texture mipmaps for Trilinear or EWA interpolations.
