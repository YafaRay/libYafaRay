YafaRay uses CMake for the building process in Linux, Windows and MacOSX systems.

See directory "building" for example procedures to build YafaRay

Dependencies:
  * Optional, depending on options selected in CMake:
    * meganz/mingw-std-threads
    * \>=zlib-1.2.8
    * \>=libxml2-2.9.1
    * \>=opencv-3.1.0
    * \>=freetype-2.4.0
    * \>=libpng-1.2.56
    * \>=jpeg-6b
    * \>=tiff-4.0.3
    * \>=ilmbase-2.2.0
    * \>=openexr-2.2.0
    * \>=python-3.5
    * \>=ruby-2.2
    * \>=qt-5
    
Notes about dependencies:
 * ZLib is needed if LibXML2, libPNG or OpenEXR are used.
 * LibXML2 is used to allow import of XML files (usually by building YafaRay XML Loader)
 * OpenCV is used to do some image processing, most importantly texture mipmaps for Trilinear or EWA interpolations.
 * Python is used only if the Python bindings are built (for example for the Blender Exporter)
 * Ruby is used if the Ruby bindings are built (for example to create the Sketchup plugin)
 * Qt is optional, used for the Sketchup plugin for example
 
