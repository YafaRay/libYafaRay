#pragma once
/****************************************************************************
 *      console_utils.h: General command line parsing and other utilities (soon)
 *      This is part of the libYafaRay package
 *      Copyright (C) 2010  Rodrigo Placencia
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

#ifndef YAFARAY_CONSOLE_H
#define YAFARAY_CONSOLE_H

#include "constants.h"
#include "common/logger.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <sstream>

BEGIN_YAFARAY

//! Struct that holds the state of the registrered options and the values parsed from command line args

struct CliParserOption
{
	CliParserOption(const std::string &s_opt, const std::string &l_opt, bool is_flag, std::string desc);
	std::string short_opt_;
	std::string long_opt_;
	bool is_flag_;
	std::string desc_;
	std::string value_;
	bool is_set_;
};

CliParserOption::CliParserOption(const std::string &s_opt, const std::string &l_opt, bool is_flag, std::string desc) : short_opt_(s_opt), long_opt_(l_opt), is_flag_(is_flag), desc_(desc)
{
	if(!short_opt_.empty())
	{
		short_opt_ = "-" + s_opt;
	}
	if(!long_opt_.empty())
	{
		long_opt_ = "--" + l_opt;
	}
	value_ = "";
	is_set_ = false;
}

//! The command line option parsing and handling class
/*!parses GNU style command line arguments pairs and flags with space (' ') as pair separator*/

class CliParser final
{
	public:
		CliParser() = default; //! Default constructor for 2 step init
		CliParser(int argc, char **argv, int clean_args_num, int clean_opt_args_num, const std::string &clean_arg_error);  //! One step init constructor
		void setCommandLineArgs(int argc, char **argv);  //! Initialization method for 2 step init
		void setCleanArgsNumber(int arg_num, int opt_arg, const std::string &clean_arg_error); //! Configures the parser to get arguments non-paired at the end of the command string with optional arg number
		void setOption(const std::string &s_opt, const std::string &l_opt, bool is_flag, const std::string &desc); //! Option registrer method, it adds a valid parsing option to the list
		std::string getOptionString(const std::string &s_opt, const std::string &l_opt = ""); //! Retrieves the string value asociated with the option if any, if no option returns an empty string
		int getOptionInteger(const std::string &s_opt, const std::string &l_opt = ""); //! Retrieves the integer value asociated with the option if any, if no option returns -65535
		bool getFlag(const std::string &s_opt, const std::string &l_opt = ""); //! Returns true is the flas was set in command line, false else
		bool isSet(const std::string &s_opt, const std::string &l_opt = ""); //! Returns true if the option was set on the command line (only for paired options)
		std::vector<std::string> getCleanArgs() const;
		void setAppName(const std::string &name, const std::string &b_usage);
		void printUsage() const; //! Prints usage instructions with the registrered options
		void printError() const; //! Prints error found during parsing (if any)
		bool parseCommandLine(); //! Parses the input values from command line and fills the values on the right options if they exist on the args

	private:
		size_t arg_count_; //! Input arguments count
		std::string app_name_; //! Holds the app name used in the usage construction, defaults to argv[0]
		std::string bin_name_; //! Holds the name of the executabl binary (argv[0])
		std::string basic_usage_; //! Holds the basic usage instructions of the command
		std::vector<std::string> arg_values_; //! Holds argv values
		std::vector<std::string> clean_values_; //! Holds clean (non-paired options) values
		std::vector<std::unique_ptr<CliParserOption>> reg_options_; //! Holds registrered options
		size_t clean_args_;
		size_t clean_args_optional_;
		std::string clean_args_error_;
		std::string parse_error_;
};

CliParser::CliParser(int argc, char **argv, int clean_args_num, int clean_opt_args_num, const std::string &clean_arg_error)
{
	setCommandLineArgs(argc, argv);
	setCleanArgsNumber(clean_args_num, clean_opt_args_num, clean_arg_error);
}

void CliParser::setCommandLineArgs(int argc, char **argv)
{
	arg_count_ = static_cast<size_t>(argc);
	arg_values_.clear();
	app_name_ = std::string(argv[0]);
	bin_name_ = std::string(argv[0]);
	for(size_t i = 1; i < arg_count_; i++)
	{
		arg_values_.push_back(std::string(argv[i]));
	}
}

void CliParser::setCleanArgsNumber(int arg_num, int opt_arg, const std::string &clean_arg_error)
{
	clean_args_ = arg_num;
	clean_args_optional_ = opt_arg;
	clean_args_error_ = clean_arg_error;
}

void CliParser::setOption(const std::string &s_opt, const std::string &l_opt, bool is_flag, const std::string &desc)
{
	if(!s_opt.empty() || !l_opt.empty()) reg_options_.emplace_back(new CliParserOption(s_opt, l_opt, is_flag, desc));
}

std::string CliParser::getOptionString(const std::string &s_opt, const std::string &l_opt)
{
	std::string cmp_s_opt = "-" + s_opt;
	std::string cmp_l_opt = "--" + l_opt;

	for(size_t j = 0; j < reg_options_.size(); j++)
	{
		if(reg_options_[j]->short_opt_.compare(cmp_s_opt) == 0 || reg_options_[j]->long_opt_.compare(cmp_l_opt) == 0)
		{
			if(!reg_options_[j]->is_flag_)
			{
				return reg_options_[j]->value_;
			}
		}
	}
	return std::string("");
}

int CliParser::getOptionInteger(const std::string &s_opt, const std::string &l_opt)
{
	int ret = -65535;
	std::string cmp_s_opt = "-" + s_opt;
	std::string cmp_l_opt = "--" + l_opt;

	for(size_t j = 0; j < reg_options_.size(); j++)
	{
		if(reg_options_[j]->short_opt_.compare(cmp_s_opt) == 0 || reg_options_[j]->long_opt_.compare(cmp_l_opt) == 0)
		{
			if(!reg_options_[j]->is_flag_)
			{
				std::istringstream i(reg_options_[j]->value_.c_str());
				if(!(i >> ret))
					return -65535;
				else
					return ret;
			}
		}
	}
	return ret;
}

bool CliParser::getFlag(const std::string &s_opt, const std::string &l_opt)
{
	std::string cmp_s_opt = "-" + s_opt;
	std::string cmp_l_opt = "--" + l_opt;

	for(size_t j = 0; j < reg_options_.size(); j++)
	{
		if(reg_options_[j]->short_opt_.compare(cmp_s_opt) == 0 || reg_options_[j]->long_opt_.compare(cmp_l_opt) == 0)
		{
			if(reg_options_[j]->is_flag_)
			{
				return reg_options_[j]->is_set_;
			}
		}
	}
	return false;
}

bool CliParser::isSet(const std::string &s_opt, const std::string &l_opt)
{
	std::string cmp_s_opt = "-" + s_opt;
	std::string cmp_l_opt = "--" + l_opt;

	for(size_t j = 0; j < reg_options_.size(); j++)
	{
		if(reg_options_[j]->short_opt_.compare(cmp_s_opt) == 0 || reg_options_[j]->long_opt_.compare(cmp_l_opt) == 0)
		{
			if(!reg_options_[j]->is_flag_)
			{
				return reg_options_[j]->is_set_;
			}
		}
	}
	return false;
}

std::vector<std::string> CliParser::getCleanArgs() const
{
	return clean_values_;
}

void CliParser::setAppName(const std::string &name, const std::string &b_usage)
{
	app_name_.clear();
	app_name_ = name;
	basic_usage_.clear();
	basic_usage_ = b_usage;
}
void CliParser::printUsage() const
{
	Y_INFO << app_name_ << YENDL
		   << "Usage: " << bin_name_ << " " << basic_usage_ << YENDL
		   << "OPTIONS:" << YENDL;
	for(size_t i = 0; i < reg_options_.size(); i++)
	{
		std::stringstream name;
		name << reg_options_[i]->short_opt_ << ", " << reg_options_[i]->long_opt_ << (reg_options_[i]->is_flag_ ? "" : " <value>");
		std::cout << "    "
				  << std::setiosflags(std::ios::left) << std::setw(35)
				  << name.str()
				  << reg_options_[i]->desc_ << YENDL;
	}
	Y_INFO << "Usage instructions end." << YENDL;
}

void CliParser::printError() const
{
	Y_ERROR << parse_error_ << YENDL;
}

bool CliParser::parseCommandLine()
{
	std::stringstream error;
	clean_values_.clear();
	for(size_t i = 0; i < arg_values_.size(); i++)
	{
		if((i >= arg_values_.size() - (clean_args_ - clean_args_optional_)) || (i >= arg_values_.size() - clean_args_))
		{
			if(arg_values_[i].compare(0, 1, "-") != 0)
			{
				clean_values_.push_back(arg_values_[i]);
				continue;
			}
		}

		for(size_t j = 0; j < reg_options_.size(); j++)
		{
			if(reg_options_[j]->short_opt_.compare(arg_values_[i]) == 0 || reg_options_[j]->long_opt_.compare(arg_values_[i]) == 0)
			{
				if(!reg_options_[j]->is_flag_)
				{
					if(i < arg_values_.size())
					{
						i++;
						if(arg_values_[i].compare(0, 1, "-") != 0)
						{
							reg_options_[j]->value_.clear();
							reg_options_[j]->value_.append(arg_values_[i]);
							reg_options_[j]->is_set_ = true;
						}
						else
						{
							error << "Option " << reg_options_[j]->long_opt_ << " has no value";
							parse_error_ = error.str();
							return false;
						}
					}
					else
					{
						error << "Option " << reg_options_[j]->long_opt_ << " has no value";
						parse_error_ = error.str();
						return false;
					}
				}
				else
				{
					reg_options_[j]->is_set_ = true;
				}
			}
		}
	}

	if(clean_values_.size() < clean_args_ && clean_values_.size() < clean_args_ - clean_args_optional_)
	{
		error << clean_args_error_;
		parse_error_ = error.str();
		return false;
	}
	return true;
}

END_YAFARAY

#endif /* YAFARAY_CONSOLE_H */
