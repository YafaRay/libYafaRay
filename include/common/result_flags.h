//
// Created by david on 23/10/22.
//

#ifndef LIBYAFARAY_RESULT_FLAGS_H
#define LIBYAFARAY_RESULT_FLAGS_H

#include "common/enum.h"
#include "common/enum_map.h"
#include "public_api/yafaray_c_api.h"
#include <string>
#include <vector>
#include <sstream>

namespace yafaray {

struct ResultFlags : Enum<ResultFlags, int>
{
	using Enum::Enum;
	inline static const EnumMap<ValueType_t> map_{{
			{"Ok", YAFARAY_RESULT_OK, ""},
			{"ErrorTypeUnknownParam", YAFARAY_RESULT_ERROR_TYPE_UNKNOWN_PARAM, ""},
			{"WarningUnknownParam", YAFARAY_RESULT_WARNING_UNKNOWN_PARAM, ""},
			{"WarningParamNotSet", YAFARAY_RESULT_WARNING_PARAM_NOT_SET, ""},
			{"ErrorWrongParamType", YAFARAY_RESULT_ERROR_WRONG_PARAM_TYPE, ""},
			{"WarningUnknownEnumOption", YAFARAY_RESULT_WARNING_UNKNOWN_ENUM_OPTION, ""},
			{"ErrorAlreadyExists", YAFARAY_RESULT_ERROR_ALREADY_EXISTS, ""},
			{"ErrorWhileCreating", YAFARAY_RESULT_ERROR_WHILE_CREATING, ""},
			{"ErrorNotFound", YAFARAY_RESULT_ERROR_NOT_FOUND, ""},
			{"WarningOverwritten", YAFARAY_RESULT_WARNING_OVERWRITTEN, ""},
			{"ErrorDuplicatedName", YAFARAY_RESULT_ERROR_DUPLICATED_NAME, ""},
		}};
	[[nodiscard]] bool isOk() const { return value() == YAFARAY_RESULT_OK; }
	[[nodiscard]] bool notOk() const { return !isOk(); }
	[[nodiscard]] bool hasError() const;
	[[nodiscard]] bool hasWarning() const;
};

inline bool ResultFlags::hasError() const
{
	return has(YAFARAY_RESULT_ERROR_TYPE_UNKNOWN_PARAM)
		|| has(YAFARAY_RESULT_ERROR_WRONG_PARAM_TYPE)
		|| has(YAFARAY_RESULT_ERROR_ALREADY_EXISTS)
		|| has(YAFARAY_RESULT_ERROR_WHILE_CREATING)
		|| has(YAFARAY_RESULT_ERROR_NOT_FOUND)
		|| has(YAFARAY_RESULT_ERROR_DUPLICATED_NAME);
}

inline bool ResultFlags::hasWarning() const
{
	return has(YAFARAY_RESULT_WARNING_UNKNOWN_PARAM)
		|| has(YAFARAY_RESULT_WARNING_UNKNOWN_ENUM_OPTION)
		|| has(YAFARAY_RESULT_WARNING_PARAM_NOT_SET)
		|| has(YAFARAY_RESULT_WARNING_OVERWRITTEN);
}

} //namespace yafaray

#endif //LIBYAFARAY_RESULT_FLAGS_H
