/** @file Utilities.cpp
 * Contains the implementation of routines used in several places.
 */
#include "Utilities.h"

/**getTokens gets tokens from a string separated by the given separator
 *
 * @param source a string to be split into tokens
 * @param separator the character used to separate tokens
 * @return a vector of strings containing the extracted tokens 
 */
vector<string> getTokens (string source, char separator) {
	string strBuf;
	stringstream ss(source);
	vector<string> tokensFound;
	while (getline(ss, strBuf, separator))
		tokensFound.push_back(strBuf);
	return tokensFound;
}

/**formatGPStime gives text GPS calendar data using the format provided (as per strftime). 
 * Note that seconds, if given, is an integer number (as per strftime)
 *
 * @param buffer the text buffer where calendar data are placed
 * @param bufferSize of the text buffer in bytes
 * @param fmt the the format to be used for conversion, as per strftime
 * @param week the GPS week from 6/1/1980
 * @param second the GPS seconds from the beginning of the week
 */
void formatGPStime (char* buffer, int bufferSize, char* fmt, int week, double second) {
	//get calendar for the GPS time of this epoch
	time_t rawtime;
	struct tm* timeinfo;
	time (&rawtime);
	timeinfo = localtime (&rawtime);
	//set GPS ephemeris 6/1/1980, adding gps time data to get current time
	timeinfo->tm_year = 80;
	timeinfo->tm_mon = 0;
	timeinfo->tm_mday = 6 + week * 7;
	timeinfo->tm_hour = 0;
	timeinfo->tm_min = 0;
	timeinfo->tm_sec = 0 + (int) second;
	mktime(timeinfo);
	strftime (buffer, bufferSize, fmt, timeinfo);
}

/**formatLocalTime gives text calendar data of local time using the format provided (as per strftime). 
 *
 * @param buffer the text buffer where calendar data are placed
 * @param bufferSize of the text buffer in bytes
 * @param fmt the format to be used for conversion, as per strftime
 */
void formatLocalTime (char* buffer, int bufferSize, char* fmt) {
	//get local time and format it as needed
	time_t rawtime;
	struct tm * timeinfo;
	time (&rawtime);
	timeinfo = localtime (&rawtime);
	strftime (buffer, bufferSize, fmt, timeinfo);
}

/**getGPSseconds gets the remaining seconds modulo minute (<60.0 seconds) from the TOW.
 * 
 * @param tow the number of seconds (and fraction) 
 * @return the remaining seconds and fraction after extracting minutes
 */
double getGPSseconds (double tow) {
	return tow - floor(tow / 60.0) * 60.0;	//return the seconds (allways positive)
}
