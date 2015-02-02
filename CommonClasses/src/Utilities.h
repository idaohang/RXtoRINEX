/** @file Utilities.h
 * Contains definition of routines used in several places.
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
#include <sstream>
#include <vector>
#include <time.h>
#include <math.h>

using namespace std;

//wstring s2ws(const string& );	//string to wstring-LPCWSTR conversion

vector<string> getTokens (string source, char separator);			//extract tokens from a string
void formatLocalTime (char* buffer, int bufferSize, char* fmt);		//format local time
void formatGPStime (char* buffer, int bufferSize, char* fmt, int week, double second); //format GPS date & time
double getGPSseconds (double tow); //Get the remaining seconds modulo minute