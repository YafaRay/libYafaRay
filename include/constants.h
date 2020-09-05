#pragma once
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

#ifndef YAFARAY_CONSTANTS_H
#define YAFARAY_CONSTANTS_H

#define BEGIN_YAFARAY namespace yafaray4 {
#define END_YAFARAY }

// define symbol export and import attributes
#ifdef _WIN32
#define YF_EXPORT __declspec(dllexport)
#define YF_IMPORT __declspec(dllimport)
#else // _WIN32
#define YF_EXPORT __attribute__ ((visibility("default")))
#define YF_IMPORT
#endif // _WIN32

// automatic macros that switch between import and export, depending on compiler environment
#ifdef BUILDING_LIBYAFARAY
#define LIBYAFARAY_EXPORT YF_EXPORT
#else // BUILDING_LIBYAFARAY
#define LIBYAFARAY_EXPORT YF_IMPORT
#endif // BUILDING_LIBYAFARAY

#endif // YAFARAY_CONSTANTS_H
