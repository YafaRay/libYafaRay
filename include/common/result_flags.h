//
// Created by david on 23/10/22.
//

#ifndef LIBYAFARAY_RESULT_FLAGS_H
#define LIBYAFARAY_RESULT_FLAGS_H

#include "common/enum.h"
#include "common/enum_map.h"
#include <string>
#include <vector>
#include <sstream>

namespace yafaray {

struct ResultFlags : Enum<ResultFlags, int>
{
	using Enum::Enum;
	enum : ValueType_t
	{
		Ok = 0,
		ErrorTypeUnknownParam = 1 << 0,
		WarningUnknownParam = 1 << 1,
		WarningParamNotSet = 1 << 2,
		ErrorWrongParamType = 1 << 3,
		WarningUnknownEnumOption = 1 << 4,
		ErrorAlreadyExists = 1 << 5,
		ErrorWhileCreating = 1 << 6,
		ErrorNotFound = 1 << 7,
		WarningOverwritten = 1 << 8,
		ErrorDuplicatedName = 1 << 9,
	};
	inline static const EnumMap<ValueType_t> map_{{
			{"None", Ok, ""},
			{"ErrorTypeUnknownParam", ErrorTypeUnknownParam, ""},
			{"WarningUnknownParam", WarningUnknownParam, ""},
			{"ErrorWrongParamType", ErrorWrongParamType, ""},
			{"WarningUnknownEnumOption", WarningUnknownEnumOption, ""},
			{"ErrorAlreadyExists", ErrorAlreadyExists, ""},
			{"ErrorWhileCreating", ErrorWhileCreating, ""},
			{"ErrorNotFound", ErrorNotFound, ""},
			{"WarningOverwritten", WarningOverwritten, ""},
			{"ErrorDuplicatedName", ErrorDuplicatedName, ""},
		}};
	[[nodiscard]] bool isOk() const { return value() == Ok; }
	[[nodiscard]] bool notOk() const { return !isOk(); }
	[[nodiscard]] bool hasError() const;
	[[nodiscard]] bool hasWarning() const;
};

inline bool ResultFlags::hasError() const
{
	return has(ResultFlags::ErrorTypeUnknownParam)
		|| has(ResultFlags::ErrorWrongParamType)
		|| has(ResultFlags::ErrorAlreadyExists)
		|| has(ResultFlags::ErrorWhileCreating)
		|| has(ResultFlags::ErrorNotFound)
		|| has(ResultFlags::ErrorDuplicatedName);
}

inline bool ResultFlags::hasWarning() const
{
	return has(ResultFlags::WarningUnknownParam)
		|| has(ResultFlags::WarningUnknownEnumOption)
		|| has(ResultFlags::WarningParamNotSet)
		|| has(ResultFlags::WarningOverwritten);
}

} //namespace yafaray

#endif //LIBYAFARAY_RESULT_FLAGS_H
