/** @file ArgParser.h
 *Contains the ArgParser class definition. 
 *An ArgParser object is a container for the command line options and operands passed to a program through argv.
 *
 *Copyright 2015 Francisco Cancillo
 *<p>
 *This file is part of the RXtoRINEX tool.
 *<p>
 *RXtoRINEX is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
 *as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *RXtoRINEX is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 *warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *See the GNU General Public License for more details.
 *<p>
 *A copy of the GNU General Public License can be found at <http://www.gnu.org/licenses/>.
 *
 *V1.0	First release
 */

#pragma once

#include <string>
#include <forward_list>
#include <vector>

using namespace std;

//@cond DUMMY
//Constant to define error messages
#define MSG_UnknOption " is an unknown option"
#define MSG_ValueNotSet " is a string option. It requires a value"
#define MSG_ValueSet " ia a boolean option. A string value cannot be assigned"
#define MSG_TooOpe " too much operators"
#define MSG_UnkId "Unknown identification"

//Option class defines an abstract type for storing options in an ArgParser container.
class Option {
	friend class ArgParser;
	int identification;	///< the unique identifier for the option
	string sName;		///< its short name (-x)
	string lName;		///< its long name (--x{x})
	string description;	///< a text word to identify the option value. Used in usage method output
	string usage;		///< a message for helping use of this option. Used in usage method output
	bool isStr;			///< true if its value is a string, false if boolean
	string defStr;		///< default value if it is string
	string strSet;		///< value set if it is string
	bool defBool;		///< default value if it is boolean
	bool boolSet;		///< value set if it is boolean

	Option (void);
	Option(int, string, string, bool, bool);
	Option(int, char*, char*, char*, char*, char*);
	Option(int, char*, char*, char*, char*, bool);
};
//@endcond 

/**ArgParser class defines a data container for options and operators passed as arguments in the command line.
 * A program using ArgParser would perform the following steps after declaring an ArgParser object:
 *	-# Define each option the program can accept using the addOption method. Data to be provided for each one are:
 *		- its short name in the form -x (like "-f")
 *		- its long name in the form --x{x}	(like "--file")
 *		- a word describing the option value (like "INFILE")
 *		- a explanation for usage of this option (like "binary input file name")
 *		- default value for the option (like "data.dat" if it is a string, or true if it is boolean)
 *	-# Define each operand the program can accept using the addOperator method. For each one a default value has to be provided
 *	-# Process arguments in the command line using the method parseArgs. User should catch any error detected and provide
 *	information to user with the usage method
 *	-# Get values of options using the getBoolOpt or getStrOpt methods, and of operands using getOperator
 */
class ArgParser {
	friend class Option;
	forward_list<Option> optionsLst;	///< to place options data
	int lastId;							///< the last identification assigned
	vector<string> operatorsVector;		///< to place operator values

	bool isShortOption(string, bool);
	bool isLongOption(string, bool);
	void checkOption(string, Option);
	void setOptVal (Option);
public:
	ArgParser(void);
	~ArgParser(void);
	int addOption (char *, char *, char *, char *, char *);
	int addOption (char *, char *, char *, char *, bool);
	int addOperator(char *);
	bool getBoolOpt (int);
	string getStrOpt (int);
	string getOperator (int);
	void usage(string, string);
	void parseArgs (int, char **); 
	string ArgParser::showOptValues();
	string ArgParser::showOpeValues();
};


