/** @file Logger.h
 *A simple logger class to tag and record loging messages.
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
#include <time.h>
#include <stdio.h>

using namespace std;

/// The log levels
enum logLevel {SEVERE, WARNING, INFO, CONFIG, FINE, FINER, FINEST};

/** Logger class allows recording of tagged messages.
 * A program using Logger would perform the following steps:
 *	-# Define a Logger object stating the fileName of the logging file, or using the default stderr.
 *	-# State the desired log level. Can be SEVERE, WARNING, INFO, CONFIG, FINE, FINER or FINEST.
		If the log level is not explicitly stated, the default level is INFO.
 *	-# Log any message that would be necessary using the method corresponding to the desired log level of the message.
 *		Only those messages having level from SEVERE to the current level stated are recorded in the log file.
 */
class Logger {
	string program;		//program name to tag logs
	logLevel levelSet;	//maximum level to log
	FILE * fileLog;
	void logMsg (logLevel msgLevel, string msg);
public:
	Logger(string);
	Logger(void);
	~Logger(void);
	void setPrgName(string);
	void setLevel (logLevel);
	void severe (string);
	void warning (string);
	void info (string);
	void config (string);
	void fine (string);
	void finer (string);
	void finest (string);
};

