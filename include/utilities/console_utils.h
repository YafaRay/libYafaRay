/****************************************************************************
 * 		console_utils.h: General command line parsing and other utilities (soon)
 *      This is part of the YafaRay package
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

#ifndef Y_CONSOLE_UTILS_H
#define Y_CONSOLE_UTILS_H

#include <yafray_config.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>

__BEGIN_YAFRAY

#define ystring std::string
#define ysstream std::stringstream
#define yisstream std::istringstream
#define yvector std::vector

//! Struct that holds the state of the registrered options and the values parsed from command line args

struct cliParserOption_t
{
	cliParserOption_t(ystring sOpt, ystring lOpt, bool isFlag, ystring desc);
	ystring shortOpt;
	ystring longOpt;
	bool isFlag;
	ystring desc;
	ystring value;
	bool isSet;
};

cliParserOption_t::cliParserOption_t(ystring sOpt, ystring lOpt, bool isFlag, ystring desc) : shortOpt(sOpt), longOpt(lOpt), isFlag(isFlag), desc(desc)
{
	if(!shortOpt.empty())
	{
		shortOpt = "-" + sOpt;
	}
	if(!longOpt.empty())
	{
		longOpt = "--" + lOpt;
	}
	value = "";
	isSet = false;
}

//! The command line option parsing and handling class
/*!parses GNU style command line arguments pairs and flags with space (' ') as pair separator*/

class cliParser_t
{
	public:
	cliParser_t() {} //! Default constructor for 2 step init
	cliParser_t( int argc, char **argv, int cleanArgsNum, int cleanOptArgsNum, const ystring cleanArgError); //! One step init constructor
	~cliParser_t();
	void setCommandLineArgs( int argc, char **argv); //! Initialization method for 2 step init
	void setCleanArgsNumber(int argNum, int optArg, const ystring cleanArgError); //! Configures the parser to get arguments non-paired at the end of the command string with optional arg number
	void setOption(ystring sOpt, ystring lOpt, bool isFlag, ystring desc); //! Option registrer method, it adds a valid parsing option to the list
	ystring getOptionString(const ystring sOpt, const ystring lOpt = ""); //! Retrieves the string value asociated with the option if any, if no option returns an empty string
	int getOptionInteger(const ystring sOpt, const ystring lOpt = ""); //! Retrieves the integer value asociated with the option if any, if no option returns -65535
	bool getFlag(const ystring sOpt, const ystring lOpt = ""); //! Returns true is the flas was set in command line, false else
	bool isSet(const ystring sOpt, const ystring lOpt = ""); //! Returns true if the option was set on the command line (only for paired options)
	yvector<ystring> getCleanArgs() const;
	void setAppName(const ystring name, const ystring bUsage);
	void printUsage() const; //! Prints usage instructions with the registrered options
	void clearOptions(); //! Clears the registrered options list
	bool parseCommandLine(); //! Parses the input values from command line and fills the values on the right options if they exist on the args
	
	private:
	size_t argCount; //! Input arguments count
	ystring appName; //! Holds the app name used in the usage construction, defaults to argv[0]
	ystring binName; //! Holds the name of the executabl binary (argv[0])
	ystring basicUsage; //! Holds the basic usage instructions of the command
	yvector<ystring> argValues; //! Holds argv values
	yvector<ystring> cleanValues; //! Holds clean (non-paired options) values
	yvector<cliParserOption_t*> regOptions; //! Holds registrered options
	size_t cleanArgs;
	size_t cleanArgsOptional;
	ystring cleanArgsError;
};

cliParser_t::cliParser_t(int argc, char **argv, int cleanArgsNum, int cleanOptArgsNum, const ystring cleanArgError)
{
	setCommandLineArgs( argc, argv );
	setCleanArgsNumber(cleanArgsNum, cleanOptArgsNum, cleanArgError);
}

cliParser_t::~cliParser_t()
{
	clearOptions();
}

void cliParser_t::setCommandLineArgs( int argc, char **argv)
{
	argCount = (size_t)argc;
	argValues.clear();
	appName = ystring(argv[0]);
	binName = ystring(argv[0]);
	for(size_t i = 1; i < argCount; i++)
	{
		argValues.push_back(ystring(argv[i]));
	}
}

void cliParser_t::setCleanArgsNumber(int argNum, int optArg, const ystring cleanArgError)
{
	cleanArgs = argNum;
	cleanArgsOptional = optArg;
	cleanArgsError = cleanArgError;
}

void cliParser_t::setOption(ystring sOpt, ystring lOpt, bool isFlag, ystring desc)
{
	if(!sOpt.empty() || !lOpt.empty()) regOptions.push_back(new cliParserOption_t(sOpt, lOpt, isFlag, desc));
}

ystring cliParser_t::getOptionString(const ystring sOpt, const ystring lOpt)
{
	ystring cmpSOpt = "-" + sOpt;
	ystring cmpLOpt = "--" + lOpt;
	
	for(size_t j = 0; j < regOptions.size(); j++)
	{
		if(regOptions[j]->shortOpt.compare(cmpSOpt) == 0 || regOptions[j]->longOpt.compare(cmpLOpt) == 0)
		{
			if(!regOptions[j]->isFlag)
			{
				return regOptions[j]->value;
			}
		}
	}
	
	return ystring("");
}

int cliParser_t::getOptionInteger(const ystring sOpt, const ystring lOpt)
{
	int ret = -65535;
	ystring cmpSOpt = "-" + sOpt;
	ystring cmpLOpt = "--" + lOpt;
	
	for(size_t j = 0; j < regOptions.size(); j++)
	{
		if(regOptions[j]->shortOpt.compare(cmpSOpt) == 0 || regOptions[j]->longOpt.compare(cmpLOpt) == 0)
		{
			if(!regOptions[j]->isFlag)
			{
				yisstream i(regOptions[j]->value.c_str());
				if(!(i >> ret))
					return -65535;
				else
					return ret;
			}
		}
	}
	
	return ret;
}

bool cliParser_t::getFlag(const ystring sOpt, const ystring lOpt)
{
	ystring cmpSOpt = "-" + sOpt;
	ystring cmpLOpt = "--" + lOpt;
	
	for(size_t j = 0; j < regOptions.size(); j++)
	{
		if(regOptions[j]->shortOpt.compare(cmpSOpt) == 0 || regOptions[j]->longOpt.compare(cmpLOpt) == 0)
		{
			if(regOptions[j]->isFlag)
			{
				return regOptions[j]->isSet;
			}
		}
	}
	
	return false;
}

bool cliParser_t::isSet(const ystring sOpt, const ystring lOpt)
{
	ystring cmpSOpt = "-" + sOpt;
	ystring cmpLOpt = "--" + lOpt;
	
	for(size_t j = 0; j < regOptions.size(); j++)
	{
		if(regOptions[j]->shortOpt.compare(cmpSOpt) == 0 || regOptions[j]->longOpt.compare(cmpLOpt) == 0)
		{
			if(!regOptions[j]->isFlag)
			{
				return regOptions[j]->isSet;
			}
		}
	}
	
	return false;
}

yvector<ystring> cliParser_t::getCleanArgs() const
{
	return cleanValues;
}

void cliParser_t::setAppName(const ystring name, const ystring bUsage)
{
	appName.clear();
	appName = name;
	basicUsage.clear();
	basicUsage = bUsage;
}
void cliParser_t::printUsage() const
{
	Y_INFO << appName << std::endl
	<< "Usage: " << binName << " " << basicUsage << std::endl
	<< "OPTIONS:\n";
	for(size_t i = 0; i < regOptions.size(); i++)
	{
		std::cout << "\t"
		<< regOptions[i]->shortOpt << " or "
		<< regOptions[i]->longOpt << (regOptions[i]->isFlag?" : ":" <value> : ")
		<< regOptions[i]->desc << std::endl << std::endl;
	}
	Y_INFO << "Usage instructions end." << std::endl;
}

void cliParser_t::clearOptions()
{
	if(regOptions.size() > 0)
	{
		for(size_t i = 0; i < regOptions.size(); i++)
		{
			delete regOptions[i];
		}
		regOptions.clear();
	}
}

bool cliParser_t::parseCommandLine()
{
	cleanValues.clear();
	for(size_t i = 0; i < argValues.size(); i++)
	{
		if((i >= argValues.size() - (cleanArgs - cleanArgsOptional)) || (i >= argValues.size() - cleanArgs))
		{
			if(argValues[i].compare(0,1,"-") != 0)
			{ 
				cleanValues.push_back(argValues[i]);
				continue;
			}
		}

		for(size_t j = 0; j < regOptions.size(); j++)
		{
			if(regOptions[j]->shortOpt.compare(argValues[i]) == 0 || regOptions[j]->longOpt.compare(argValues[i]) == 0)
			{
				if(!regOptions[j]->isFlag)
				{
					if(i < argValues.size())
					{
						i++;
						if(argValues[i].compare(0,1,"-") != 0)
						{ 
							regOptions[j]->value.clear();
							regOptions[j]->value.append(argValues[i]);
							regOptions[j]->isSet = true;
						}
						else
						{
							Y_ERROR << "Option " << regOptions[j]->longOpt << " has no value";
							return false;
						}
					}
					else
					{
						Y_ERROR << "Option " << regOptions[j]->longOpt << " has no value";
						return false;
					}
				}
				else
				{
					regOptions[j]->isSet = true;
				}
			}
		}
	}
	
	if(cleanValues.size() < cleanArgs && cleanValues.size() < cleanArgs - cleanArgsOptional)
	{
		Y_ERROR << cleanArgsError << std::endl;
		return false;
	}
	
	return true;
}

__END_YAFRAY

#endif /* Y_CONSOLE_UTILS_H */
