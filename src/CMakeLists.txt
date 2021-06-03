if(WIN32)
	list(APPEND YAF_DEFINITIONS "WIN32")
	list(APPEND YAF_DEFINITIONS "_USE_MATH_DEFINES")
	list(APPEND YAF_DEFINITIONS "_hypot=hypot")
endif()

message_boolean("Using mathematical" FAST_MATH "approximations (faster)" "precise values (slower)")
if(FAST_MATH)
	list(APPEND YAF_DEFINITIONS "FAST_MATH")
endif()

message_boolean("Using trigonometric" FAST_TRIG "approximations (faster)" "precise values (slower)")
if(FAST_TRIG)
	list(APPEND YAF_DEFINITIONS "FAST_TRIG")
endif()

if(WITH_MINGW_STD_THREADS AND WIN32 AND MINGW)
	list(APPEND YAF_DEPS_INCLUDE_DIRS ${MINGW_STD_THREADS_INCLUDE_DIR})
	list(APPEND YAF_DEFINITIONS "HAVE_MINGW_STD_THREADS")
endif()

message_boolean("Using OpenCV" WITH_OpenCV "yes" "no")
if(WITH_OpenCV)
	FIND_PACKAGE(OpenCV COMPONENTS core imgproc photo REQUIRED)
	list(APPEND YAF_DEFINITIONS "HAVE_OPENCV")
	list(APPEND YAF_DEPS opencv_core opencv_imgproc opencv_photo)
endif()

if(WITH_OpenEXR OR WITH_PNG)
	find_package(ZLIB)
	if(NOT ZLIB_FOUND)
		message(FATAL_ERROR "ZLib not found: if XML Loader, OpenEXR or PNG options are enabled, ZLib is required.")
	endif()
endif()

message_boolean("Using FreeType" WITH_Freetype "yes" "no")
if(WITH_Freetype)
	find_package(Freetype REQUIRED)
	list(APPEND YAF_DEFINITIONS "HAVE_FREETYPE")
	list(APPEND YAF_DEPS Freetype::Freetype)
endif()

message_boolean("Using OpenEXR" WITH_OpenEXR "yes" "no")
if(WITH_OpenEXR)
	find_package(Imath CONFIG)
	if(NOT TARGET Imath::Imath)
		message("-- Imath not found (not OpenEXR v3.x), looking for IlmBase and OpenEXR v2.4/2.5")
		find_package(IlmBase CONFIG)
		if (NOT TARGET IlmBase::IlmBase)
			message("-- IlmBase not found (not OpenEXR v2.4/2.5), looking for old OpenEXR v2.2")
		endif()
	endif()
	find_package(OpenEXR REQUIRED)
	list(APPEND YAF_DEFINITIONS "HAVE_OPENEXR")
	if(TARGET OpenEXR::OpenEXR)
		list(APPEND YAF_DEPS OpenEXR::OpenEXR)
	elseif(TARGET OpenEXR::IlmImf)
		list(APPEND YAF_DEPS OpenEXR::IlmImf)
	else()
		list(APPEND YAF_DEPS_INCLUDE_DIRS ${OPENEXR_INCLUDE_DIRS})
		list(APPEND YAF_DEPS ${OPENEXR_LIBRARIES})
	endif()
endif()

message_boolean("Using JPEG" WITH_JPEG "yes" "no")
if(WITH_JPEG)
	find_package(JPEG REQUIRED)
	list(APPEND YAF_DEFINITIONS "HAVE_JPEG")
	list(APPEND YAF_DEPS JPEG::JPEG)
endif()

message_boolean("Using PNG" WITH_PNG "yes" "no")
if(WITH_PNG)
	find_package(PNG REQUIRED)
	list(APPEND YAF_DEFINITIONS "HAVE_PNG")
	list(APPEND YAF_DEPS PNG::PNG)
endif()

message_boolean("Using TIFF" WITH_TIFF "yes" "no")
if(WITH_TIFF)
	find_package(TIFF REQUIRED)
	list(APPEND YAF_DEFINITIONS "HAVE_TIFF")
	list(APPEND YAF_DEPS TIFF::TIFF)
endif()

add_library(libyafaray4)
set_target_properties(libyafaray4 PROPERTIES PREFIX "" VERSION 4.0.0 SOVERSION 4)
set_target_properties(libyafaray4 PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
set_target_properties(libyafaray4 PROPERTIES C_VISIBILITY_PRESET hidden CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN hidden)
target_compile_definitions(libyafaray4 PRIVATE ${YAF_DEFINITIONS} YAFARAY_BUILD_COMPILER=std::string\(\"FIXME\"\) YAFARAY_BUILD_PLATFORM=std::string\(\"FIXME\"\) YAFARAY_BUILD_OS=std::string\(\"FIXME\"\) YAFARAY_BUILD_ARCHITECTURE=std::string\(\"FIXME\"\) YAFARAY_BUILD_VERSION=std::string\(\"FIXME\"\) min_raydist_global=0.0005 shadow_bias_global=0.005)
target_include_directories(libyafaray4 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include ${CMAKE_BINARY_DIR}/src)
target_include_directories(libyafaray4 INTERFACE $<INSTALL_INTERFACE:include>)
target_include_directories(libyafaray4 SYSTEM BEFORE PRIVATE ${YAF_DEPS_INCLUDE_DIRS})
target_link_libraries(libyafaray4 PRIVATE ${YAF_DEPS})
find_package(Threads REQUIRED)
target_link_libraries(libyafaray4 PRIVATE Threads::Threads)

target_sources(libyafaray4
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/yafaray_c_api.cc
)

add_subdirectory(accelerator)
add_subdirectory(background)
add_subdirectory(color)
add_subdirectory(common)
add_subdirectory(camera)
add_subdirectory(geometry)
add_subdirectory(image)
add_subdirectory(format)
add_subdirectory(integrator)
add_subdirectory(interface)
add_subdirectory(light)
add_subdirectory(material)
add_subdirectory(math)
add_subdirectory(output)
add_subdirectory(photon)
add_subdirectory(render)
add_subdirectory(sampler)
add_subdirectory(scene)
add_subdirectory(shader)
add_subdirectory(texture)
add_subdirectory(volume)

# Shared target used for generating export header # with the name mytools_export.h, which will be suitable # for both the shared and static targets
include(GenerateExportHeader)
generate_export_header(libyafaray4 BASE_NAME yafaray_c_api)

install(TARGETS libyafaray4
		EXPORT LibYafaRay
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		)

install(FILES
		../include/yafaray_conf.h
		../include/yafaray_c_api.h
		${CMAKE_BINARY_DIR}/src/yafaray_c_api_export.h
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
		)

install(EXPORT LibYafaRay
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LibYafaRay
		NAMESPACE LibYafaRay::
		FILE LibYafaRayConfig.cmake
		)
