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

#include "sampler/halton.h"
#include "geometry/vector.h"
#include <vector>
#include <array>

BEGIN_YAFARAY

namespace faure
{

//Start of Faure tables
static const std::vector<int> base_3
		{0, 1, 2};

static const std::vector<int> base_5
		{0, 3, 2, 1, 4};

static const std::vector<int> base_7
		{0, 2, 5, 3, 1, 4, 6};

static const std::vector<int> base_11
		{0, 7, 4, 2, 9, 5, 1, 8, 6, 3, 10};

static const std::vector<int> base_13
		{0, 4, 9, 2, 7, 11, 6, 1, 5, 10, 3, 8, 12};

static const std::vector<int> base_17
		{0, 9, 4, 13, 2, 11, 6, 15, 8, 1, 10, 5, 14, 3, 12, 7, 16};

static const std::vector<int> base_19
		{0, 11, 4, 15, 8, 2, 13, 6, 17, 9, 1, 12, 5, 16, 10, 3, 14, 7, 18};

static const std::vector<int> base_23
		{0, 15, 8, 4, 19, 10, 2, 17, 13, 6, 21, 11, 1, 16, 9, 5, 20, 12, 3, 18, 14, 7, 22};

static const std::vector<int> base_29
		{0, 8, 21, 12, 4, 17, 25, 2, 10, 23, 15, 6, 19, 27, 14, 1, 9, 22, 13, 5, 18,
		 26, 3, 11, 24, 16, 7, 20, 28};

static const std::vector<int> base_31
		{0, 8, 23, 12, 4, 19, 27, 14, 2, 10, 25, 17, 6, 21, 29, 15, 1, 9, 24, 13, 5,
		 20, 28, 16, 3, 11, 26, 18, 7, 22, 30};

static const std::vector<int> base_37
		{0, 21, 8, 29, 16, 4, 25, 12, 33, 2, 23, 10, 31, 19, 6, 27, 14, 35, 18, 1, 22,
		 9, 30, 17, 5, 26, 13, 34, 3, 24, 11, 32, 20, 7, 28, 15, 36};

static const std::vector<int> base_41
		{0, 25, 16, 8, 33, 4, 29, 21, 12, 37, 2, 27, 18, 10, 35, 6, 31, 23, 14, 39, 20,
		 1, 26, 17, 9, 34, 5, 30, 22, 13, 38, 3, 28, 19, 11, 36, 7, 32, 24, 15, 40};

static const std::vector<int> base_43
		{0, 27, 16, 8, 35, 4, 31, 23, 12, 39, 20, 2, 29, 18, 10, 37, 6, 33, 25, 14, 41,
		 21, 1, 28, 17, 9, 36, 5, 32, 24, 13, 40, 22, 3, 30, 19, 11, 38, 7, 34, 26,
		 15, 42};

static const std::vector<int> base_47
		{0, 31, 16, 8, 39, 20, 4, 35, 27, 12, 43, 22, 2, 33, 18, 10, 41, 25, 6, 37, 29,
		 14, 45, 23, 1, 32, 17, 9, 40, 21, 5, 36, 28, 13, 44, 24, 3, 34, 19, 11, 42,
		 26, 7, 38, 30, 15, 46};

static const std::vector<int> base_53
		{0, 16, 37, 8, 29, 45, 24, 4, 20, 41, 12, 33, 49, 2, 18, 39, 10, 31, 47, 27, 6,
		 22, 43, 14, 35, 51, 26, 1, 17, 38, 9, 30, 46, 25, 5, 21, 42, 13, 34, 50, 3,
		 19, 40, 11, 32, 48, 28, 7, 23, 44, 15, 36, 52};

static const std::vector<int> base_59
		{0, 16, 43, 24, 8, 35, 51, 4, 20, 47, 31, 12, 39, 55, 28, 2, 18, 45, 26, 10, 37,
		 53, 6, 22, 49, 33, 14, 41, 57, 29, 1, 17, 44, 25, 9, 36, 52, 5, 21, 48, 32,
		 13, 40, 56, 30, 3, 19, 46, 27, 11, 38, 54, 7, 23, 50, 34, 15, 42, 58};

static const std::vector<int> base_61
		{0, 16, 45, 24, 8, 37, 53, 28, 4, 20, 49, 33, 12, 41, 57, 2, 18, 47, 26, 10, 39,
		 55, 31, 6, 22, 51, 35, 14, 43, 59, 30, 1, 17, 46, 25, 9, 38, 54, 29, 5, 21,
		 50, 34, 13, 42, 58, 3, 19, 48, 27, 11, 40, 56, 32, 7, 23, 52, 36, 15, 44, 60};

static const std::vector<int> base_67
		{0, 35, 16, 51, 8, 43, 24, 59, 4, 39, 20, 55, 12, 47, 28, 63, 32, 2, 37, 18, 53,
		 10, 45, 26, 61, 6, 41, 22, 57, 14, 49, 30, 65, 33, 1, 36, 17, 52, 9, 44, 25,
		 60, 5, 40, 21, 56, 13, 48, 29, 64, 34, 3, 38, 19, 54, 11, 46, 27, 62, 7, 42,
		 23, 58, 15, 50, 31, 66};

static const std::vector<int> base_71
		{0, 39, 16, 55, 8, 47, 24, 63, 32, 4, 43, 20, 59, 12, 51, 28, 67, 34, 2, 41, 18,
		 57, 10, 49, 26, 65, 37, 6, 45, 22, 61, 14, 53, 30, 69, 35, 1, 40, 17, 56, 9,
		 48, 25, 64, 33, 5, 44, 21, 60, 13, 52, 29, 68, 36, 3, 42, 19, 58, 11, 50, 27,
		 66, 38, 7, 46, 23, 62, 15, 54, 31, 70};

static const std::vector<int> base_73
		{0, 41, 16, 57, 32, 8, 49, 24, 65, 4, 45, 20, 61, 37, 12, 53, 28, 69, 2, 43, 18,
		 59, 34, 10, 51, 26, 67, 6, 47, 22, 63, 39, 14, 55, 30, 71, 36, 1, 42, 17, 58,
		 33, 9, 50, 25, 66, 5, 46, 21, 62, 38, 13, 54, 29, 70, 3, 44, 19, 60, 35, 11,
		 52, 27, 68, 7, 48, 23, 64, 40, 15, 56, 31, 72};

static const std::vector<int> base_79
		{0, 47, 16, 63, 32, 8, 55, 24, 71, 36, 4, 51, 20, 67, 43, 12, 59, 28, 75, 38, 2,
		 49, 18, 65, 34, 10, 57, 26, 73, 41, 6, 53, 22, 69, 45, 14, 61, 30, 77, 39, 1,
		 48, 17, 64, 33, 9, 56, 25, 72, 37, 5, 52, 21, 68, 44, 13, 60, 29, 76, 40, 3,
		 50, 19, 66, 35, 11, 58, 27, 74, 42, 7, 54, 23, 70, 46, 15, 62, 31, 78};

static const std::vector<int> base_83
		{0, 51, 32, 16, 67, 8, 59, 43, 24, 75, 4, 55, 36, 20, 71, 12, 63, 47, 28, 79, 40,
		 2, 53, 34, 18, 69, 10, 61, 45, 26, 77, 6, 57, 38, 22, 73, 14, 65, 49, 30, 81,
		 41, 1, 52, 33, 17, 68, 9, 60, 44, 25, 76, 5, 56, 37, 21, 72, 13, 64, 48, 29,
		 80, 42, 3, 54, 35, 19, 70, 11, 62, 46, 27, 78, 7, 58, 39, 23, 74, 15, 66, 50,
		 31, 82};

static const std::vector<int> base_89
		{0, 57, 32, 16, 73, 40, 8, 65, 49, 24, 81, 4, 61, 36, 20, 77, 45, 12, 69, 53, 28,
		 85, 2, 59, 34, 18, 75, 42, 10, 67, 51, 26, 83, 6, 63, 38, 22, 79, 47, 14, 71,
		 55, 30, 87, 44, 1, 58, 33, 17, 74, 41, 9, 66, 50, 25, 82, 5, 62, 37, 21, 78,
		 46, 13, 70, 54, 29, 86, 3, 60, 35, 19, 76, 43, 11, 68, 52, 27, 84, 7, 64, 39,
		 23, 80, 48, 15, 72, 56, 31, 88};

static const std::vector<int> base_97
		{0, 32, 65, 16, 49, 81, 8, 40, 73, 24, 57, 89, 4, 36, 69, 20, 53, 85, 12, 44, 77,
		 28, 61, 93, 2, 34, 67, 18, 51, 83, 10, 42, 75, 26, 59, 91, 6, 38, 71, 22, 55,
		 87, 14, 46, 79, 30, 63, 95, 48, 1, 33, 66, 17, 50, 82, 9, 41, 74, 25, 58, 90,
		 5, 37, 70, 21, 54, 86, 13, 45, 78, 29, 62, 94, 3, 35, 68, 19, 52, 84, 11, 43,
		 76, 27, 60, 92, 7, 39, 72, 23, 56, 88, 15, 47, 80, 31, 64, 96};

static const std::vector<int> base_101
		{0, 32, 69, 16, 53, 85, 8, 40, 77, 24, 61, 93, 48, 4, 36, 73, 20, 57, 89, 12, 44,
		 81, 28, 65, 97, 2, 34, 71, 18, 55, 87, 10, 42, 79, 26, 63, 95, 51, 6, 38, 75,
		 22, 59, 91, 14, 46, 83, 30, 67, 99, 50, 1, 33, 70, 17, 54, 86, 9, 41, 78, 25,
		 62, 94, 49, 5, 37, 74, 21, 58, 90, 13, 45, 82, 29, 66, 98, 3, 35, 72, 19, 56,
		 88, 11, 43, 80, 27, 64, 96, 52, 7, 39, 76, 23, 60, 92, 15, 47, 84, 31, 68, 100};

static const std::vector<int> base_103
		{0, 32, 71, 16, 55, 87, 8, 40, 79, 24, 63, 95, 48, 4, 36, 75, 20, 59, 91, 12, 44,
		 83, 28, 67, 99, 50, 2, 34, 73, 18, 57, 89, 10, 42, 81, 26, 65, 97, 53, 6, 38,
		 77, 22, 61, 93, 14, 46, 85, 30, 69, 101, 51, 1, 33, 72, 17, 56, 88, 9, 41, 80,
		 25, 64, 96, 49, 5, 37, 76, 21, 60, 92, 13, 45, 84, 29, 68, 100, 52, 3, 35, 74,
		 19, 58, 90, 11, 43, 82, 27, 66, 98, 54, 7, 39, 78, 23, 62, 94, 15, 47, 86, 31,
		 70, 102};

static const std::vector<int> base_107
		{0, 32, 75, 16, 59, 91, 48, 8, 40, 83, 24, 67, 99, 4, 36, 79, 20, 63, 95, 55, 12,
		 44, 87, 28, 71, 103, 52, 2, 34, 77, 18, 61, 93, 50, 10, 42, 85, 26, 69, 101, 6,
		 38, 81, 22, 65, 97, 57, 14, 46, 89, 30, 73, 105, 53, 1, 33, 76, 17, 60, 92, 49,
		 9, 41, 84, 25, 68, 100, 5, 37, 80, 21, 64, 96, 56, 13, 45, 88, 29, 72, 104, 54,
		 3, 35, 78, 19, 62, 94, 51, 11, 43, 86, 27, 70, 102, 7, 39, 82, 23, 66, 98, 58,
		 15, 47, 90, 31, 74, 106};

static const std::vector<int> base_109
		{0, 32, 77, 16, 61, 93, 48, 8, 40, 85, 24, 69, 101, 52, 4, 36, 81, 20, 65, 97, 57,
		 12, 44, 89, 28, 73, 105, 2, 34, 79, 18, 63, 95, 50, 10, 42, 87, 26, 71, 103, 55,
		 6, 38, 83, 22, 67, 99, 59, 14, 46, 91, 30, 75, 107, 54, 1, 33, 78, 17, 62, 94,
		 49, 9, 41, 86, 25, 70, 102, 53, 5, 37, 82, 21, 66, 98, 58, 13, 45, 90, 29, 74,
		 106, 3, 35, 80, 19, 64, 96, 51, 11, 43, 88, 27, 72, 104, 56, 7, 39, 84, 23, 68,
		 100, 60, 15, 47, 92, 31, 76, 108};

static const std::vector<int> base_113
		{0, 32, 81, 48, 16, 65, 97, 8, 40, 89, 57, 24, 73, 105, 4, 36, 85, 52, 20, 69, 101,
		 12, 44, 93, 61, 28, 77, 109, 2, 34, 83, 50, 18, 67, 99, 10, 42, 91, 59, 26, 75,
		 107, 6, 38, 87, 54, 22, 71, 103, 14, 46, 95, 63, 30, 79, 111, 56, 1, 33, 82, 49,
		 17, 66, 98, 9, 41, 90, 58, 25, 74, 106, 5, 37, 86, 53, 21, 70, 102, 13, 45, 94,
		 62, 29, 78, 110, 3, 35, 84, 51, 19, 68, 100, 11, 43, 92, 60, 27, 76, 108, 7, 39,
		 88, 55, 23, 72, 104, 15, 47, 96, 64, 31, 80, 112};

static const std::vector<int> base_127
		{0, 32, 95, 48, 16, 79, 111, 56, 8, 40, 103, 71, 24, 87, 119, 60, 4, 36, 99, 52, 20,
		 83, 115, 67, 12, 44, 107, 75, 28, 91, 123, 62, 2, 34, 97, 50, 18, 81, 113, 58, 10,
		 42, 105, 73, 26, 89, 121, 65, 6, 38, 101, 54, 22, 85, 117, 69, 14, 46, 109, 77, 30,
		 93, 125, 63, 1, 33, 96, 49, 17, 80, 112, 57, 9, 41, 104, 72, 25, 88, 120, 61, 5,
		 37, 100, 53, 21, 84, 116, 68, 13, 45, 108, 76, 29, 92, 124, 64, 3, 35, 98, 51, 19,
		 82, 114, 59, 11, 43, 106, 74, 27, 90, 122, 66, 7, 39, 102, 55, 23, 86, 118, 70, 15,
		 47, 110, 78, 31, 94, 126};

static const std::vector<int> base_131
		{0, 67, 32, 99, 16, 83, 48, 115, 8, 75, 40, 107, 24, 91, 56, 123, 4, 71, 36, 103, 20,
		 87, 52, 119, 12, 79, 44, 111, 28, 95, 60, 127, 64, 2, 69, 34, 101, 18, 85, 50, 117,
		 10, 77, 42, 109, 26, 93, 58, 125, 6, 73, 38, 105, 22, 89, 54, 121, 14, 81, 46, 113,
		 30, 97, 62, 129, 65, 1, 68, 33, 100, 17, 84, 49, 116, 9, 76, 41, 108, 25, 92, 57,
		 124, 5, 72, 37, 104, 21, 88, 53, 120, 13, 80, 45, 112, 29, 96, 61, 128, 66, 3, 70,
		 35, 102, 19, 86, 51, 118, 11, 78, 43, 110, 27, 94, 59, 126, 7, 74, 39, 106, 23, 90,
		 55, 122, 15, 82, 47, 114, 31, 98, 63, 130};

static const std::vector<int> base_137
		{0, 73, 32, 105, 16, 89, 48, 121, 64, 8, 81, 40, 113, 24, 97, 56, 129, 4, 77, 36, 109,
		 20, 93, 52, 125, 69, 12, 85, 44, 117, 28, 101, 60, 133, 2, 75, 34, 107, 18, 91, 50,
		 123, 66, 10, 83, 42, 115, 26, 99, 58, 131, 6, 79, 38, 111, 22, 95, 54, 127, 71, 14,
		 87, 46, 119, 30, 103, 62, 135, 68, 1, 74, 33, 106, 17, 90, 49, 122, 65, 9, 82, 41,
		 114, 25, 98, 57, 130, 5, 78, 37, 110, 21, 94, 53, 126, 70, 13, 86, 45, 118, 29, 102,
		 61, 134, 3, 76, 35, 108, 19, 92, 51, 124, 67, 11, 84, 43, 116, 27, 100, 59, 132, 7,
		 80, 39, 112, 23, 96, 55, 128, 72, 15, 88, 47, 120, 31, 104, 63, 136};

static const std::vector<int> base_139
		{0, 75, 32, 107, 16, 91, 48, 123, 64, 8, 83, 40, 115, 24, 99, 56, 131, 4, 79, 36, 111,
		 20, 95, 52, 127, 71, 12, 87, 44, 119, 28, 103, 60, 135, 68, 2, 77, 34, 109, 18, 93,
		 50, 125, 66, 10, 85, 42, 117, 26, 101, 58, 133, 6, 81, 38, 113, 22, 97, 54, 129, 73,
		 14, 89, 46, 121, 30, 105, 62, 137, 69, 1, 76, 33, 108, 17, 92, 49, 124, 65, 9, 84,
		 41, 116, 25, 100, 57, 132, 5, 80, 37, 112, 21, 96, 53, 128, 72, 13, 88, 45, 120, 29,
		 104, 61, 136, 70, 3, 78, 35, 110, 19, 94, 51, 126, 67, 11, 86, 43, 118, 27, 102, 59,
		 134, 7, 82, 39, 114, 23, 98, 55, 130, 74, 15, 90, 47, 122, 31, 106, 63, 138};

static const std::vector<int> base_149
		{0, 85, 32, 117, 64, 16, 101, 48, 133, 8, 93, 40, 125, 77, 24, 109, 56, 141, 72, 4, 89,
		 36, 121, 68, 20, 105, 52, 137, 12, 97, 44, 129, 81, 28, 113, 60, 145, 2, 87, 34, 119,
		 66, 18, 103, 50, 135, 10, 95, 42, 127, 79, 26, 111, 58, 143, 75, 6, 91, 38, 123, 70,
		 22, 107, 54, 139, 14, 99, 46, 131, 83, 30, 115, 62, 147, 74, 1, 86, 33, 118, 65, 17,
		 102, 49, 134, 9, 94, 41, 126, 78, 25, 110, 57, 142, 73, 5, 90, 37, 122, 69, 21, 106,
		 53, 138, 13, 98, 45, 130, 82, 29, 114, 61, 146, 3, 88, 35, 120, 67, 19, 104, 51, 136,
		 11, 96, 43, 128, 80, 27, 112, 59, 144, 76, 7, 92, 39, 124, 71, 23, 108, 55, 140, 15,
		 100, 47, 132, 84, 31, 116, 63, 148};

static const std::vector<int> base_151
		{0, 87, 32, 119, 64, 16, 103, 48, 135, 8, 95, 40, 127, 79, 24, 111, 56, 143, 72, 4, 91,
		 36, 123, 68, 20, 107, 52, 139, 12, 99, 44, 131, 83, 28, 115, 60, 147, 74, 2, 89, 34,
		 121, 66, 18, 105, 50, 137, 10, 97, 42, 129, 81, 26, 113, 58, 145, 77, 6, 93, 38, 125,
		 70, 22, 109, 54, 141, 14, 101, 46, 133, 85, 30, 117, 62, 149, 75, 1, 88, 33, 120, 65,
		 17, 104, 49, 136, 9, 96, 41, 128, 80, 25, 112, 57, 144, 73, 5, 92, 37, 124, 69, 21,
		 108, 53, 140, 13, 100, 45, 132, 84, 29, 116, 61, 148, 76, 3, 90, 35, 122, 67, 19, 106,
		 51, 138, 11, 98, 43, 130, 82, 27, 114, 59, 146, 78, 7, 94, 39, 126, 71, 23, 110, 55,
		 142, 15, 102, 47, 134, 86, 31, 118, 63, 150};

static const std::vector<int> base_157
		{0, 93, 32, 125, 64, 16, 109, 48, 141, 72, 8, 101, 40, 133, 85, 24, 117, 56, 149, 76, 4,
		 97, 36, 129, 68, 20, 113, 52, 145, 81, 12, 105, 44, 137, 89, 28, 121, 60, 153, 2, 95,
		 34, 127, 66, 18, 111, 50, 143, 74, 10, 103, 42, 135, 87, 26, 119, 58, 151, 79, 6, 99,
		 38, 131, 70, 22, 115, 54, 147, 83, 14, 107, 46, 139, 91, 30, 123, 62, 155, 78, 1, 94,
		 33, 126, 65, 17, 110, 49, 142, 73, 9, 102, 41, 134, 86, 25, 118, 57, 150, 77, 5, 98,
		 37, 130, 69, 21, 114, 53, 146, 82, 13, 106, 45, 138, 90, 29, 122, 61, 154, 3, 96, 35,
		 128, 67, 19, 112, 51, 144, 75, 11, 104, 43, 136, 88, 27, 120, 59, 152, 80, 7, 100, 39,
		 132, 71, 23, 116, 55, 148, 84, 15, 108, 47, 140, 92, 31, 124, 63, 156};

static const std::vector<int> base_163
		{0, 99, 64, 32, 131, 16, 115, 83, 48, 147, 8, 107, 72, 40, 139, 24, 123, 91, 56, 155, 4,
		 103, 68, 36, 135, 20, 119, 87, 52, 151, 12, 111, 76, 44, 143, 28, 127, 95, 60, 159, 80,
		 2, 101, 66, 34, 133, 18, 117, 85, 50, 149, 10, 109, 74, 42, 141, 26, 125, 93, 58, 157,
		 6, 105, 70, 38, 137, 22, 121, 89, 54, 153, 14, 113, 78, 46, 145, 30, 129, 97, 62, 161,
		 81, 1, 100, 65, 33, 132, 17, 116, 84, 49, 148, 9, 108, 73, 41, 140, 25, 124, 92, 57,
		 156, 5, 104, 69, 37, 136, 21, 120, 88, 53, 152, 13, 112, 77, 45, 144, 29, 128, 96, 61,
		 160, 82, 3, 102, 67, 35, 134, 19, 118, 86, 51, 150, 11, 110, 75, 43, 142, 27, 126, 94,
		 59, 158, 7, 106, 71, 39, 138, 23, 122, 90, 55, 154, 15, 114, 79, 47, 146, 31, 130, 98,
		 63, 162};

static const std::vector<int> base_167
		{0, 103, 64, 32, 135, 16, 119, 87, 48, 151, 8, 111, 72, 40, 143, 24, 127, 95, 56, 159, 80,
		 4, 107, 68, 36, 139, 20, 123, 91, 52, 155, 12, 115, 76, 44, 147, 28, 131, 99, 60, 163,
		 82, 2, 105, 66, 34, 137, 18, 121, 89, 50, 153, 10, 113, 74, 42, 145, 26, 129, 97, 58,
		 161, 85, 6, 109, 70, 38, 141, 22, 125, 93, 54, 157, 14, 117, 78, 46, 149, 30, 133, 101,
		 62, 165, 83, 1, 104, 65, 33, 136, 17, 120, 88, 49, 152, 9, 112, 73, 41, 144, 25, 128,
		 96, 57, 160, 81, 5, 108, 69, 37, 140, 21, 124, 92, 53, 156, 13, 116, 77, 45, 148, 29,
		 132, 100, 61, 164, 84, 3, 106, 67, 35, 138, 19, 122, 90, 51, 154, 11, 114, 75, 43, 146,
		 27, 130, 98, 59, 162, 86, 7, 110, 71, 39, 142, 23, 126, 94, 55, 158, 15, 118, 79, 47,
		 150, 31, 134, 102, 63, 166};

static const std::vector<int> base_173
		{0, 109, 64, 32, 141, 16, 125, 93, 48, 157, 80, 8, 117, 72, 40, 149, 24, 133, 101, 56, 165,
		 84, 4, 113, 68, 36, 145, 20, 129, 97, 52, 161, 89, 12, 121, 76, 44, 153, 28, 137, 105,
		 60, 169, 2, 111, 66, 34, 143, 18, 127, 95, 50, 159, 82, 10, 119, 74, 42, 151, 26, 135,
		 103, 58, 167, 87, 6, 115, 70, 38, 147, 22, 131, 99, 54, 163, 91, 14, 123, 78, 46, 155,
		 30, 139, 107, 62, 171, 86, 1, 110, 65, 33, 142, 17, 126, 94, 49, 158, 81, 9, 118, 73,
		 41, 150, 25, 134, 102, 57, 166, 85, 5, 114, 69, 37, 146, 21, 130, 98, 53, 162, 90, 13,
		 122, 77, 45, 154, 29, 138, 106, 61, 170, 3, 112, 67, 35, 144, 19, 128, 96, 51, 160, 83,
		 11, 120, 75, 43, 152, 27, 136, 104, 59, 168, 88, 7, 116, 71, 39, 148, 23, 132, 100, 55,
		 164, 92, 15, 124, 79, 47, 156, 31, 140, 108, 63, 172};

static const std::vector<int> base_179
		{0, 115, 64, 32, 147, 80, 16, 131, 99, 48, 163, 8, 123, 72, 40, 155, 91, 24, 139, 107, 56,
		 171, 4, 119, 68, 36, 151, 84, 20, 135, 103, 52, 167, 12, 127, 76, 44, 159, 95, 28, 143,
		 111, 60, 175, 88, 2, 117, 66, 34, 149, 82, 18, 133, 101, 50, 165, 10, 125, 74, 42, 157,
		 93, 26, 141, 109, 58, 173, 6, 121, 70, 38, 153, 86, 22, 137, 105, 54, 169, 14, 129, 78,
		 46, 161, 97, 30, 145, 113, 62, 177, 89, 1, 116, 65, 33, 148, 81, 17, 132, 100, 49, 164,
		 9, 124, 73, 41, 156, 92, 25, 140, 108, 57, 172, 5, 120, 69, 37, 152, 85, 21, 136, 104,
		 53, 168, 13, 128, 77, 45, 160, 96, 29, 144, 112, 61, 176, 90, 3, 118, 67, 35, 150, 83,
		 19, 134, 102, 51, 166, 11, 126, 75, 43, 158, 94, 27, 142, 110, 59, 174, 7, 122, 71, 39,
		 154, 87, 23, 138, 106, 55, 170, 15, 130, 79, 47, 162, 98, 31, 146, 114, 63, 178};

static const std::vector<int> base_181
		{0, 117, 64, 32, 149, 80, 16, 133, 101, 48, 165, 8, 125, 72, 40, 157, 93, 24, 141, 109, 56,
		 173, 88, 4, 121, 68, 36, 153, 84, 20, 137, 105, 52, 169, 12, 129, 76, 44, 161, 97, 28,
		 145, 113, 60, 177, 2, 119, 66, 34, 151, 82, 18, 135, 103, 50, 167, 10, 127, 74, 42, 159,
		 95, 26, 143, 111, 58, 175, 91, 6, 123, 70, 38, 155, 86, 22, 139, 107, 54, 171, 14, 131,
		 78, 46, 163, 99, 30, 147, 115, 62, 179, 90, 1, 118, 65, 33, 150, 81, 17, 134, 102, 49,
		 166, 9, 126, 73, 41, 158, 94, 25, 142, 110, 57, 174, 89, 5, 122, 69, 37, 154, 85, 21,
		 138, 106, 53, 170, 13, 130, 77, 45, 162, 98, 29, 146, 114, 61, 178, 3, 120, 67, 35, 152,
		 83, 19, 136, 104, 51, 168, 11, 128, 75, 43, 160, 96, 27, 144, 112, 59, 176, 92, 7, 124,
		 71, 39, 156, 87, 23, 140, 108, 55, 172, 15, 132, 79, 47, 164, 100, 31, 148, 116, 63, 180};

static const std::vector<int> base_191
		{0, 127, 64, 32, 159, 80, 16, 143, 111, 48, 175, 88, 8, 135, 72, 40, 167, 103, 24, 151, 119,
		 56, 183, 92, 4, 131, 68, 36, 163, 84, 20, 147, 115, 52, 179, 99, 12, 139, 76, 44, 171,
		 107, 28, 155, 123, 60, 187, 94, 2, 129, 66, 34, 161, 82, 18, 145, 113, 50, 177, 90, 10,
		 137, 74, 42, 169, 105, 26, 153, 121, 58, 185, 97, 6, 133, 70, 38, 165, 86, 22, 149, 117,
		 54, 181, 101, 14, 141, 78, 46, 173, 109, 30, 157, 125, 62, 189, 95, 1, 128, 65, 33, 160,
		 81, 17, 144, 112, 49, 176, 89, 9, 136, 73, 41, 168, 104, 25, 152, 120, 57, 184, 93, 5,
		 132, 69, 37, 164, 85, 21, 148, 116, 53, 180, 100, 13, 140, 77, 45, 172, 108, 29, 156, 124,
		 61, 188, 96, 3, 130, 67, 35, 162, 83, 19, 146, 114, 51, 178, 91, 11, 138, 75, 43, 170,
		 106, 27, 154, 122, 59, 186, 98, 7, 134, 71, 39, 166, 87, 23, 150, 118, 55, 182, 102, 15,
		 142, 79, 47, 174, 110, 31, 158, 126, 63, 190};

static const std::vector<int> base_193
		{0, 64, 129, 32, 97, 161, 16, 80, 145, 48, 113, 177, 8, 72, 137, 40, 105, 169, 24, 88, 153,
		 56, 121, 185, 4, 68, 133, 36, 101, 165, 20, 84, 149, 52, 117, 181, 12, 76, 141, 44, 109,
		 173, 28, 92, 157, 60, 125, 189, 2, 66, 131, 34, 99, 163, 18, 82, 147, 50, 115, 179, 10,
		 74, 139, 42, 107, 171, 26, 90, 155, 58, 123, 187, 6, 70, 135, 38, 103, 167, 22, 86, 151,
		 54, 119, 183, 14, 78, 143, 46, 111, 175, 30, 94, 159, 62, 127, 191, 96, 1, 65, 130, 33,
		 98, 162, 17, 81, 146, 49, 114, 178, 9, 73, 138, 41, 106, 170, 25, 89, 154, 57, 122, 186,
		 5, 69, 134, 37, 102, 166, 21, 85, 150, 53, 118, 182, 13, 77, 142, 45, 110, 174, 29, 93,
		 158, 61, 126, 190, 3, 67, 132, 35, 100, 164, 19, 83, 148, 51, 116, 180, 11, 75, 140, 43,
		 108, 172, 27, 91, 156, 59, 124, 188, 7, 71, 136, 39, 104, 168, 23, 87, 152, 55, 120, 184,
		 15, 79, 144, 47, 112, 176, 31, 95, 160, 63, 128, 192};

static const std::vector<int> base_197
		{0, 64, 133, 32, 101, 165, 16, 80, 149, 48, 117, 181, 8, 72, 141, 40, 109, 173, 24, 88, 157,
		 56, 125, 189, 96, 4, 68, 137, 36, 105, 169, 20, 84, 153, 52, 121, 185, 12, 76, 145, 44,
		 113, 177, 28, 92, 161, 60, 129, 193, 2, 66, 135, 34, 103, 167, 18, 82, 151, 50, 119, 183,
		 10, 74, 143, 42, 111, 175, 26, 90, 159, 58, 127, 191, 99, 6, 70, 139, 38, 107, 171, 22,
		 86, 155, 54, 123, 187, 14, 78, 147, 46, 115, 179, 30, 94, 163, 62, 131, 195, 98, 1, 65,
		 134, 33, 102, 166, 17, 81, 150, 49, 118, 182, 9, 73, 142, 41, 110, 174, 25, 89, 158, 57,
		 126, 190, 97, 5, 69, 138, 37, 106, 170, 21, 85, 154, 53, 122, 186, 13, 77, 146, 45, 114,
		 178, 29, 93, 162, 61, 130, 194, 3, 67, 136, 35, 104, 168, 19, 83, 152, 51, 120, 184, 11,
		 75, 144, 43, 112, 176, 27, 91, 160, 59, 128, 192, 100, 7, 71, 140, 39, 108, 172, 23, 87,
		 156, 55, 124, 188, 15, 79, 148, 47, 116, 180, 31, 95, 164, 63, 132, 196};

static const std::vector<int> base_199
		{0, 64, 135, 32, 103, 167, 16, 80, 151, 48, 119, 183, 8, 72, 143, 40, 111, 175, 24, 88, 159,
		 56, 127, 191, 96, 4, 68, 139, 36, 107, 171, 20, 84, 155, 52, 123, 187, 12, 76, 147, 44,
		 115, 179, 28, 92, 163, 60, 131, 195, 98, 2, 66, 137, 34, 105, 169, 18, 82, 153, 50, 121,
		 185, 10, 74, 145, 42, 113, 177, 26, 90, 161, 58, 129, 193, 101, 6, 70, 141, 38, 109, 173,
		 22, 86, 157, 54, 125, 189, 14, 78, 149, 46, 117, 181, 30, 94, 165, 62, 133, 197, 99, 1,
		 65, 136, 33, 104, 168, 17, 81, 152, 49, 120, 184, 9, 73, 144, 41, 112, 176, 25, 89, 160,
		 57, 128, 192, 97, 5, 69, 140, 37, 108, 172, 21, 85, 156, 53, 124, 188, 13, 77, 148, 45,
		 116, 180, 29, 93, 164, 61, 132, 196, 100, 3, 67, 138, 35, 106, 170, 19, 83, 154, 51, 122,
		 186, 11, 75, 146, 43, 114, 178, 27, 91, 162, 59, 130, 194, 102, 7, 71, 142, 39, 110, 174,
		 23, 87, 158, 55, 126, 190, 15, 79, 150, 47, 118, 182, 31, 95, 166, 63, 134, 198};

static const std::vector<int> base_211
		{0, 64, 147, 32, 115, 179, 96, 16, 80, 163, 48, 131, 195, 8, 72, 155, 40, 123, 187, 107, 24,
		 88, 171, 56, 139, 203, 4, 68, 151, 36, 119, 183, 100, 20, 84, 167, 52, 135, 199, 12, 76,
		 159, 44, 127, 191, 111, 28, 92, 175, 60, 143, 207, 104, 2, 66, 149, 34, 117, 181, 98, 18,
		 82, 165, 50, 133, 197, 10, 74, 157, 42, 125, 189, 109, 26, 90, 173, 58, 141, 205, 6, 70,
		 153, 38, 121, 185, 102, 22, 86, 169, 54, 137, 201, 14, 78, 161, 46, 129, 193, 113, 30, 94,
		 177, 62, 145, 209, 105, 1, 65, 148, 33, 116, 180, 97, 17, 81, 164, 49, 132, 196, 9, 73,
		 156, 41, 124, 188, 108, 25, 89, 172, 57, 140, 204, 5, 69, 152, 37, 120, 184, 101, 21, 85,
		 168, 53, 136, 200, 13, 77, 160, 45, 128, 192, 112, 29, 93, 176, 61, 144, 208, 106, 3, 67,
		 150, 35, 118, 182, 99, 19, 83, 166, 51, 134, 198, 11, 75, 158, 43, 126, 190, 110, 27, 91,
		 174, 59, 142, 206, 7, 71, 154, 39, 122, 186, 103, 23, 87, 170, 55, 138, 202, 15, 79, 162,
		 47, 130, 194, 114, 31, 95, 178, 63, 146, 210};

static const std::vector<int> base_223
		{0, 64, 159, 32, 127, 191, 96, 16, 80, 175, 48, 143, 207, 104, 8, 72, 167, 40, 135, 199, 119,
		 24, 88, 183, 56, 151, 215, 108, 4, 68, 163, 36, 131, 195, 100, 20, 84, 179, 52, 147, 211,
		 115, 12, 76, 171, 44, 139, 203, 123, 28, 92, 187, 60, 155, 219, 110, 2, 66, 161, 34, 129,
		 193, 98, 18, 82, 177, 50, 145, 209, 106, 10, 74, 169, 42, 137, 201, 121, 26, 90, 185, 58,
		 153, 217, 113, 6, 70, 165, 38, 133, 197, 102, 22, 86, 181, 54, 149, 213, 117, 14, 78, 173,
		 46, 141, 205, 125, 30, 94, 189, 62, 157, 221, 111, 1, 65, 160, 33, 128, 192, 97, 17, 81,
		 176, 49, 144, 208, 105, 9, 73, 168, 41, 136, 200, 120, 25, 89, 184, 57, 152, 216, 109, 5,
		 69, 164, 37, 132, 196, 101, 21, 85, 180, 53, 148, 212, 116, 13, 77, 172, 45, 140, 204, 124,
		 29, 93, 188, 61, 156, 220, 112, 3, 67, 162, 35, 130, 194, 99, 19, 83, 178, 51, 146, 210,
		 107, 11, 75, 170, 43, 138, 202, 122, 27, 91, 186, 59, 154, 218, 114, 7, 71, 166, 39, 134,
		 198, 103, 23, 87, 182, 55, 150, 214, 118, 15, 79, 174, 47, 142, 206, 126, 31, 95, 190, 63,
		 158, 222};

static const std::vector<int> base_227
		{0, 64, 163, 96, 32, 131, 195, 16, 80, 179, 115, 48, 147, 211, 8, 72, 171, 104, 40, 139, 203,
		 24, 88, 187, 123, 56, 155, 219, 4, 68, 167, 100, 36, 135, 199, 20, 84, 183, 119, 52, 151,
		 215, 12, 76, 175, 108, 44, 143, 207, 28, 92, 191, 127, 60, 159, 223, 112, 2, 66, 165, 98,
		 34, 133, 197, 18, 82, 181, 117, 50, 149, 213, 10, 74, 173, 106, 42, 141, 205, 26, 90, 189,
		 125, 58, 157, 221, 6, 70, 169, 102, 38, 137, 201, 22, 86, 185, 121, 54, 153, 217, 14, 78,
		 177, 110, 46, 145, 209, 30, 94, 193, 129, 62, 161, 225, 113, 1, 65, 164, 97, 33, 132, 196,
		 17, 81, 180, 116, 49, 148, 212, 9, 73, 172, 105, 41, 140, 204, 25, 89, 188, 124, 57, 156,
		 220, 5, 69, 168, 101, 37, 136, 200, 21, 85, 184, 120, 53, 152, 216, 13, 77, 176, 109, 45,
		 144, 208, 29, 93, 192, 128, 61, 160, 224, 114, 3, 67, 166, 99, 35, 134, 198, 19, 83, 182,
		 118, 51, 150, 214, 11, 75, 174, 107, 43, 142, 206, 27, 91, 190, 126, 59, 158, 222, 7, 71,
		 170, 103, 39, 138, 202, 23, 87, 186, 122, 55, 154, 218, 15, 79, 178, 111, 47, 146, 210, 31,
		 95, 194, 130, 63, 162, 226};

static const std::vector<int> base_229
		{0, 64, 165, 96, 32, 133, 197, 16, 80, 181, 117, 48, 149, 213, 8, 72, 173, 104, 40, 141, 205,
		 24, 88, 189, 125, 56, 157, 221, 112, 4, 68, 169, 100, 36, 137, 201, 20, 84, 185, 121, 52,
		 153, 217, 12, 76, 177, 108, 44, 145, 209, 28, 92, 193, 129, 60, 161, 225, 2, 66, 167, 98,
		 34, 135, 199, 18, 82, 183, 119, 50, 151, 215, 10, 74, 175, 106, 42, 143, 207, 26, 90, 191,
		 127, 58, 159, 223, 115, 6, 70, 171, 102, 38, 139, 203, 22, 86, 187, 123, 54, 155, 219, 14,
		 78, 179, 110, 46, 147, 211, 30, 94, 195, 131, 62, 163, 227, 114, 1, 65, 166, 97, 33, 134,
		 198, 17, 81, 182, 118, 49, 150, 214, 9, 73, 174, 105, 41, 142, 206, 25, 89, 190, 126, 57,
		 158, 222, 113, 5, 69, 170, 101, 37, 138, 202, 21, 85, 186, 122, 53, 154, 218, 13, 77, 178,
		 109, 45, 146, 210, 29, 93, 194, 130, 61, 162, 226, 3, 67, 168, 99, 35, 136, 200, 19, 83,
		 184, 120, 51, 152, 216, 11, 75, 176, 107, 43, 144, 208, 27, 91, 192, 128, 59, 160, 224, 116,
		 7, 71, 172, 103, 39, 140, 204, 23, 87, 188, 124, 55, 156, 220, 15, 79, 180, 111, 47, 148,
		 212, 31, 95, 196, 132, 63, 164, 228};

static const std::array<std::vector<int>, 51> all_tables
		{base_3, base_3, base_3, base_5, base_7, base_11, base_13, base_17, base_19, base_23, base_29, base_31, base_37, base_41, base_43, base_47, base_53, base_59, base_61, base_67, base_71, base_73, base_79, base_83, base_89, base_97, base_101, base_103, base_107, base_109, base_113, base_127, base_131, base_137, base_139, base_149, base_151, base_157, base_163, base_167, base_173, base_179, base_181, base_191, base_193, base_197, base_199, base_211, base_223, base_227, base_229};

//End of the Faure tables


static constexpr std::array<int, 50> prims
		{1, 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167,
		 173, 179, 181, 191, 193, 197, 199, 211, 223, 227};

static constexpr std::array<double, 50> inv_prims
		{1.000000000, 0.500000000, 0.333333333, 0.200000000, 0.142857143, 0.090909091, 0.076923077, 0.058823529, 0.052631579, 0.043478261, 0.034482759, 0.032258065, 0.027027027, 0.024390244, 0.023255814, 0.021276596, 0.018867925, 0.016949153, 0.016393443, 0.014925373, 0.014084507, 0.013698630, 0.012658228, 0.012048193, 0.011235955, 0.010309278, 0.009900990, 0.009708738, 0.009345794, 0.009174312, 0.008849558, 0.007874016, 0.007633588, 0.007299270, 0.007194245, 0.006711409, 0.006622517, 0.006369427, 0.006134969, 0.005988024, 0.005780347, 0.005586592, 0.005524862, 0.005235602, 0.005181347, 0.005076142, 0.005025126, 0.004739336, 0.004484305, 0.004405286};

} //namespace halton

/** Low Discrepancy Halton sampling */
// dim MUST NOT be larger than 50! Above that, random numbers may be
// the better choice anyway, not even scrambling is realiable at high dimensions.
double Halton::lowDiscrepancySampling(FastRandom &fast_random, int dim, unsigned int n)
{
	double value = 0.0;
	if(dim < 50)
	{
		const std::vector<int> &sigma = faure::all_tables[dim];
		const unsigned int base = faure::prims[dim];
		const double f = faure::inv_prims[dim];
		auto dn = static_cast<double>(n);
		double factor = f;
		while(n > 0)
		{
			value += static_cast<double>(sigma[n % base]) * factor;
			dn *= f;
			n = static_cast<unsigned int>(dn);
			factor *= f;
		}
	}
	else value = static_cast<double>(fast_random.getNextFloatNormalized());
	return value;
}

END_YAFARAY