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

#ifndef YAFARAY_UV_H
#define YAFARAY_UV_H

#include "common/yafaray_common.h"
#include <type_traits>

namespace yafaray {

template <typename T>
struct Uv
{
	T u_;
	T v_;
};

template <typename T>
inline Uv<T> operator * (float f, const Uv<T> &uv)
{
	return {f * uv.u_, f * uv.v_};
}

template <typename T>
inline Uv<T> operator * (const Uv<T> &uv, float f)
{
	return f * uv;
}

template <typename T>
inline Uv<T> operator + (const Uv<T> &uv_a, const Uv<T> &uv_b)
{
	return {uv_a.u_ + uv_b.u_, uv_a.v_ + uv_b.v_};
}

template <typename T>
inline Uv<T> operator - (const Uv<T> &uv_a, const Uv<T> &uv_b)
{
	return {uv_a.u_ - uv_b.u_, uv_a.v_ - uv_b.v_};
}

} //namespace yafaray

#endif //YAFARAY_UV_H
