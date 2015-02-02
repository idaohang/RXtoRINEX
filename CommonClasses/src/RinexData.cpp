/** @file RinexData.cpp
 * Contains the implementation of the RinexData class.
 */

#include "RinexData.h"

#include <algorithm>
#include <stdio.h>
#include <math.h>
//from CommonClasses
#include "Utilities.h"

//local functions used by several methods
/**obsCompare compares two SatObsData objects to allow sorting SatObsData vector in increasing order.
 *Ordering criteria takes into account system, satellite and observation type.
 *It assumes all observations in the vector belong to the same epoch.
 *
 * @param i	the object item in position i
 * @param j	the object item in position j
 * @return true if the element passed as first argument is considered to go before the second
 */
bool obsCompare(const SatObsData& i, const SatObsData& j) {
	if (i.sysIndex > j.sysIndex) return false;
	if (i.sysIndex < j.sysIndex) return true;
	//same system
	if (i.satellite > j.satellite) return false;
	if (i.satellite < j.satellite) return true;
	//same system and satellite
	if (i.obsTypeIndex > j.obsTypeIndex) return false;
	return true;
}

/**navCompare compares two GPSsatNav objects to allow sorting the GPSsatNav vector in increasing order.
 *Ordering criteria takes into account epoch data and satellite
 *
 * @param i	the object item in position i
 * @param j	the object item in position j
 * @return true if the element passed as first argument is considered to go before the second
 */
bool navCompare(const GPSsatNav& i, const GPSsatNav& j) {
	if (i.broadcastOrbit[5][2] > j.broadcastOrbit[5][2]) return false;
	if (i.broadcastOrbit[5][2] < j.broadcastOrbit[5][2]) return true;
	//same week
	if (i.broadcastOrbit[0][0] > j.broadcastOrbit[0][0]) return false;
	if (i.broadcastOrbit[0][0] < j.broadcastOrbit[0][0]) return true;
	//same week and t0c
	if (i.satellite > j.satellite) return false;
	//same week, t0c satellite
	return true;
}

/**getRINEXFileName gets a standard RINEX observation file name from the firts epoch time data and prefix given.
 *
 * @param designator the file name prefix with a 4-character station name designator
 * @param week the GPS week number whitout roll out (that is, increased by 1024 for current week numbers)
 * @param sec the seconds from the beginning of the week
 * @param ftype the file type ('O', 'N', ...)
 * @return the RINEX observation file name in the standard format (f.e.; PRFXdddamm.yyO)
 */
string RinexData::getRINEXfileName(string designator, int week, int sec, char ftype) {
	//get calendar for the GPS time
	string fileName;
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[30];

	time (&rawtime);
	timeinfo = localtime (&rawtime);
	//set GPS ephemeris 6/1/1980, adding gps time data to get current time
	timeinfo->tm_year = 80;
	timeinfo->tm_mon = 0;
	timeinfo->tm_mday = 6 + week * 7;
	timeinfo->tm_hour = 0;
	timeinfo->tm_min = 0;
	timeinfo->tm_sec = sec;
	mktime(timeinfo);
	designator += "----";
	designator = designator.substr(0,4);
	sprintf(buffer, "%4s%03d%1c%02d.%02d%c",
		designator.c_str(),
		timeinfo->tm_yday + 1,
		'a'+timeinfo->tm_hour,
		timeinfo->tm_min,
		timeinfo->tm_year % 100,
		ftype);
	return string(buffer);
}

//class methods
/**GNSSsystem constructor
 *
 *@param sys the system identification
 *@param obsT the observation types for this system ( C L1 L2 L5 L6 L7 L8 )
 */
GNSSsystem::GNSSsystem (char sys, vector <string> obsT) {
	system = sys;
	obsType.insert(obsType.end(), obsT.begin(), obsT.end());
	biasFactor.resize(obsT.size());
	for (unsigned i=0; i<obsT.size(); i++)
		if (obsType[i].find("C") == 0) biasFactor[i] = LSPEED;
		else if (obsType[i].find("L1") == 0) biasFactor[i] = L1CFREQ;
		else if (obsType[i].find("L2") == 0) biasFactor[i] = L2CFREQ;
		else if (obsType[i].find("L5") == 0) biasFactor[i] = L5CFREQ;
		else if (obsType[i].find("L6") == 0) biasFactor[i] = L6CFREQ;
		else if (obsType[i].find("L7") == 0) biasFactor[i] = L7CFREQ;
		else if (obsType[i].find("L8") == 0) biasFactor[i] = L8CFREQ;
		else biasFactor[i] = 0.0;
}

/**Constructs a RinexData object initialized with RINEX header data passed in parameters.
 *
 * @param v the RINEX version to be generated. (V210 or V300)
 * @param p the program used to create the current file
 * @param r who executed the program (run by)
 * @param mn the name of the antenna marker
 * @param mu the number of antenna marker
 * @param aN the antenna number
 * @param aT the antenna type
 * @param o the observer name
 * @param a the agency name
 * @param ae if end of file comment will be appended or not
 * @param ab if the receiver clock bias shall be applied to observations and time or not
 * @param sy the vector with systems data
 */
RinexData::RinexData(string v, string p, string r, string mn, string mu, string aN, string aT, string o, string a, bool ae, bool ab, vector <GNSSsystem> sy) {
	//assign values to class data members from passed arguments or using default ones 
	version = V210;
	if (v.compare("V300") == 0) version = V300;
	pgm = p;
	runby = r;
	markerName = mn;
	markerNumber = mu;
	antNumber = aN;
	antType = aT;
	observer = o;
	agency = a;
	rxNumber = "RXnumber?";
	rxType = "RXtype?";
	rxVersion = "RXversion?";
	antHigh = 0.0;
	eccEast = 0.0;
	eccNorth = 0.0;
	appEnd = ae;
	applyBias = ab;
	epochFlag = 0;
	systems = sy;
	//fill scale factors for GPS navigation data bradcast orbits
	//SV clock data
	SCALEFACTORS[0][0] = pow(2.0, 4.0);		//T0c
	SCALEFACTORS[0][1] = pow(2.0, -31.0);	//Af0: SV clock bias
	SCALEFACTORS[0][2] = pow(2.0, -43.0);	//Af1: SV clock drift
	SCALEFACTORS[0][3] = pow(2.0, -55.0);	//Af2: SV clock drift rate
	//broadcast orbit 1
	SCALEFACTORS[1][0] = 1.0;				//IODE
	SCALEFACTORS[1][1] = pow(2.0, -5.0);	//Crs
	SCALEFACTORS[1][2] = pow(2.0, -43.0) * ThisPI;	//Delta N
	SCALEFACTORS[1][3] = pow(2.0, -31.0) * ThisPI;	//M0
	//broadcast orbit 2
	SCALEFACTORS[2][0] = pow(2.0, -29.0);	//Cuc
	SCALEFACTORS[2][1] = pow(2.0, -33.0);	//e
	SCALEFACTORS[2][2] = pow(2.0, -29.0);	//Cus
	SCALEFACTORS[2][3] = pow(2.0, -19.0);	//sqrt(A) ????
	//broadcast orbit 3
	SCALEFACTORS[3][0] = pow(2.0, 4.0);		//TOE
	SCALEFACTORS[3][1] = pow(2.0, -29.0);	//Cic
	SCALEFACTORS[3][2] = pow(2.0, -31.0) * ThisPI;	//Omega0
	SCALEFACTORS[3][3] = pow(2.0, -29.0);	//Cis
	//broadcast orbit 4
	SCALEFACTORS[4][0] = pow(2.0, -31.0) * ThisPI;	//i0
	SCALEFACTORS[4][1] = pow(2.0, -5.0);	//Crc
	SCALEFACTORS[4][2] = pow(2.0, -31.0) * ThisPI;	//w
	SCALEFACTORS[4][3] = pow(2.0, -43.0) * ThisPI;	//w dot
	//broadcast orbit 5
	SCALEFACTORS[5][0] = pow(2.0, -43.0) * ThisPI;	//Idot ??
	SCALEFACTORS[5][1] = 1.0;				//codes on L2
	SCALEFACTORS[5][2] = 1.0;				//GPS week + 1024
	SCALEFACTORS[5][3] = 1.0;				//L2 P data flag
	//broadcast orbit 6
	SCALEFACTORS[6][0] = 1.0;				//SV accuracy (index)
	SCALEFACTORS[6][1] = 1.0;				//SV health
	SCALEFACTORS[6][2] = pow(2.0, -31.0);	//TGD
	SCALEFACTORS[6][3] = 1.0;				//IODC
	//broadcast orbit 7
	SCALEFACTORS[7][0] = 0.01;	//Transmission time of message in sec x 100
	SCALEFACTORS[7][1] = 1.0;	//Fit interval
	SCALEFACTORS[7][2] = 0.0;	//Spare
	SCALEFACTORS[7][3] = 0.0;	//Spare
	//URA table to obtain in meters the value associated to the satellite accuracy index
	//fill URA table as per GPS ICD 20.3.3.3.1.3
	URA[0] = 2.0;		//0
	URA[1] = 2.8;		//1
	URA[2] = 4.0;		//2
	URA[3] = 5.7;		//3
	URA[4] = 8.0;		//4
	URA[5] = 11.3;		//5
	URA[6] = pow(2.0, 4.0);		//<=6
	URA[7] = pow(2.0, 5.0);
	URA[8] = pow(2.0, 6.0);
	URA[9] = pow(2.0, 7.0);
	URA[10] = pow(2.0, 8.0);
	URA[11] = pow(2.0, 9.0);
	URA[12] = pow(2.0, 10.0);
	URA[13] = pow(2.0, 11.0);
	URA[14] = pow(2.0, 12.0);
	URA[15] = 6144.00;
}

/**Destructor.
 */
RinexData::~RinexData(void) {
}

/**Constructs a SatObsData object for storing satellite observations (pseudorrange, phase, ...) in one epoch.
 *
 * @param sys the index inside a GNSSsystem vector of the system this observation belongs
 * @param sat the satellite PRN this observation belongs
 * @param epoch the epoch time for this observation stimated by the receiver before computing the solution
 * @param obsTi the index in obsType vector inside GNSSsystem of the observation type this data belong
 * @param obsVal the value of this observation
 * @param lol if loss of lock happened when observation was taken
 * @param str the signal strength when observation was taken
 */
SatObsData::SatObsData (int sys, int sat, double epoch, int obsTi, double obsVal, int lol, int str) {
	sysIndex = sys;
	satellite = sat;
	epochTime = epoch;
	obsTypeIndex = obsTi;
	obsValue = obsVal;
	lossOfLock = lol;
	strength = str;
}

/**Construct an object used to define storage for navigation data for a given GPS satellite
 *
 * @param sat the PRN of the satellite they belong
 * @param bo the eigth lines of RINEX broadcast orbit data with four parameters each
 */
GPSsatNav::GPSsatNav(int sat, unsigned int bo[8][4]) {
	satellite = sat;
	for (int i=0; i<8; i++) 
		for (int j=0; j<4; j++)
			broadcastOrbit[i][j] = bo[i][j];
}

/**setPosition sets APROX POSITION data to be used in the RINEX file header.
 * 
 * @param x : the X coordinate of the position
 * @param y : the Y coordinate of the position
 * @param z : the Z coordinate of the position
 */
void RinexData::setPosition(float x, float y, float z) {
	aproxX = x;
	aproxY = y;
	aproxZ = z;
}

/**setReceiver sets GNSS receiver and antenna data  to be printed in the RINEX file header.
 *  
 * @param number the receiver number
 * @param type the receiver type
 * @param version the receiver version
 * @param wlfL1 the wave length factor for L1
 * @param wlfL2 the wave length factor for L2
 */
void RinexData::setReceiver(string number, string type, string version, int wlfL1, int wlfL2) {
	rxNumber = number;
	rxType = type;
	rxVersion = version;
	wvlenFactorL1 = wlfL1;
	wvlenFactorL2 = wlfL2;
}

/**setGPSTime sets GPS time data of the epoch as obtained from the receiver.
 * Note that GPS time = Estimated epoch time - receiver clock offset.
 * 
 * @param weeks		week number (from 01/06/1980)
 * @param secs		seconds from the beginning of the week
 * @param bias		receiver clock offset
 */
void RinexData::setGPSTime(int weeks, double secs, double bias) {
	gpsWeek = weeks;
	gpsTOW = secs;
	clkBias = bias;
}

/**getGPSTime gets epoch time in seconds from the beginning of the week.
 * 
 * @return time in seconds from the beginning of the week
 */
double RinexData::getGPSTime () {
	return gpsTOW;
}

/**getObsFileName constructs a standard RINEX observation file name from the current data.
 *
 * @param prefix : the file name prefix
 * @return the RINEX observation file name in the standard format (PRFXdddamm.yyO)
 */
string RinexData::getObsFileName(string prefix) {
	return getRINEXfileName(prefix, gpsWeek, (int) gpsTOW, 'O');
}

/**getGPSnavFileName constructs a standard RINEX GPS navigation file name from the current data.
 *
 * @param prefix the file name prefix or 4-character station name designator
 * @return the RINEX GPS file name in the standard format (like PRFXdddamm.yyN)
 */
string RinexData::getGPSnavFileName(string prefix) {
	if (gpsEphmNav.size() == 0)	//there are not navigation data!
		return getRINEXfileName(prefix, gpsWeek, (int) gpsTOW, 'N');
	//sort navigation data items available by epoch and satellite
	sort(gpsEphmNav.begin(), gpsEphmNav.end(), navCompare);
	//the first item in vector has the oldest epoch
	int gpsW = gpsEphmNav[0].broadcastOrbit[5][2];
	int gpsT = (int) (gpsEphmNav[0].broadcastOrbit[0][0] * SCALEFACTORS[0][0]);
	return getRINEXfileName(prefix, gpsW, gpsT, 'N');
}

/**setFistObsTime sets the current epoch time (week and TOW) as the fist observation time.
 *
 */
void RinexData::setFistObsTime() {
	firstObsWeek = gpsWeek;
	firstObsTOW = gpsTOW;
}

/**setIntervalTime computes and sets in RINEX header the time interval of GPS measurements.
 * This time interval is computed as the time difference between the GPS time (gpsWeek and gpsTOW) in the RINEX data object
 * (obtained from the former epoch) and the week an TOW passed in arguments (from the last epoch).
 * 
 * @param weeks the week number (from 01/06/1980)
 * @param secs the seconds from the beginning of the week
 */
void RinexData::setIntervalTime(int weeks, double secs) {
	obsInterval = (float) ((secs - gpsTOW) + (weeks - gpsWeek) * 604800.0);
}

/**addMeasurement stores measurement data for an observable into the epoch data storage.
 * The new measurement data are stored only when given data belongs to the current epoch or
 * when the observations vector is empty.
 *
 * @param sys the system identification (G, S, ...) the measurement belongs
 * @param sat the satellite PRN the measurement belongs
 * @param obsType the type of measurement (C1C, L1C, D1C, ...) as per RINEX V3.00
 * @param value the value of the measurement
 * @param lol the loss o lock indicator. See V2.10
 * @param strg the signal strength. See V3.00
 * @param tTag the time when measurements where made used to tag them. To be corrected when solution found
 * @return true if data have been added, false otherwise
 */
bool RinexData::addMeasurement (char sys, int sat, string obsType, double value, int lol, int strg, double tTag) {
//	bool sameEpoch = (nSatsObs == 0) || (epochTimeTag == tTag);
	if (observations.size() == 0) epochTimeTag = tTag;
	bool sameEpoch = epochTimeTag == tTag;
	//check if this observation type for this system shall be stored
	for (unsigned int i=0; sameEpoch && i<systems.size(); i++)
		if (sys == systems[i].system)
			for (unsigned int j=0; j<systems[i].obsType.size(); j++)
				if (obsType.compare(systems[i].obsType[j]) == 0) {
					observations.push_back(SatObsData(i, sat, tTag, j, value, lol, strg));
					return sameEpoch;
				}
	return sameEpoch;
}

/**addGPSNavData stores navigation data from a GPS satellite into the GPS nav data storage.
 * The navigation data are stored only when they are new (different satellite and epoch), or when gpsEphmNav vector is empty.
 *
 * @param sat the satellite PRN the measurement belongs
 * @param bo the broadcast orbit data with the eight lines of RINEX navigation data with four parameters each
 * @return true if data have been added, false otherwise
 */
bool RinexData::addGPSNavData(int sat, unsigned int bo[8][4]) {
	//check if this epoch data already exists: same satellite and epoch time
	for (unsigned int i=0; i<gpsEphmNav.size(); i++)
		if((gpsEphmNav[i].satellite == sat)
			&& (bo[5][2] == gpsEphmNav[i].broadcastOrbit[5][2])
			&& (bo[0][0] == gpsEphmNav[i].broadcastOrbit[0][0])) return false;
	gpsEphmNav.push_back(GPSsatNav(sat, bo));
	return true;
}

/**clearObs clears the current observation data.
 *
 */
void RinexData::clearObs() {
	observations.clear();
}

/**printObsHeader prints the RINEX observation file header using current RINEX data.
 * 
 * @param out the already open print stream where RINEX header will be printed
 */
void RinexData::printObsHeader(FILE* out) {
	char timeBuffer[80];
	//1st header line
	float v = 0;
	switch(version) {
	case V210: v = (float) 2.1; break;
	case V300: v = (float) 3.0; break;
	}
	//header line 1 : file contents (observation data)
	fprintf(out, "%9.2f%11c%1c%-19s%1c%19c%-20s\n", 
 				v, ' ', 'O', "BSERVATION DATA", 'M', ' ', "RINEX VERSION / TYPE");
	//get local time and format it as needed
	formatLocalTime(timeBuffer, sizeof timeBuffer,"%Y%m%d %H%M%S ");
	//header line 2: identification of the receiver and file generation date
	fprintf(out, "%-20s%-20s%s%3s %-20s\n",
 				pgm.c_str(), runby.c_str(), timeBuffer, "LCL", "PGM / RUN BY / DATE");
  	//print 3 MARKER lines
 	fprintf(out, "%-60.60s%-20s\n",
				markerName.c_str(), "MARKER NAME");
 	fprintf(out, "%-60.60s%-20s\n",
 				markerNumber.c_str(), "MARKER NUMBER" );
 	if (version == V300)
 			fprintf(out, "%-20s%40c%-20s\n",
 				"NON GEODETIC", ' ', "MARKER TYPE" );
 	//print OBSERVER line
 	fprintf(out,"%-20.20s%-40.40s%-20s\n",
 				observer.c_str(), agency.c_str(), "OBSERVER / AGENCY" );
 	//print receiver and antenna lines
 	fprintf(out, "%-20.20s%-20.20s%-20.20s%-20s\n",
 				rxNumber.c_str(), rxType.c_str(), rxVersion.c_str(),"REC # / TYPE / VERS");
 	fprintf(out, "%-20.20s%-20.20s%20c%-20s\n",
 				antNumber.c_str(), antType.c_str(), ' ', "ANT # / TYPE" );
 	//print APPROXimate position data
	fprintf(out, "%14.4f%14.4f%14.4f%18c%-20s\n",
 			aproxX, aproxY, aproxZ, ' ', "APPROX POSITION XYZ" );
 	fprintf(out, "%14.4f%14.4f%14.4f%18c%-20s\n",
 				antHigh, eccEast, eccNorth, ' ', "ANTENNA: DELTA H/E/N");
 	if (version == V210)
 			fprintf(out, "%6d%6d%6d%42c%-20s\n",
 	 					wvlenFactorL1, wvlenFactorL2, 0, ' ', "WAVELENGTH FACT L1/2");
 	//print the lines with systems data
 	switch (version) {
 	case V210:	//version 2.10
 		//limited implementation assuming same observables and order for all systems, and maximum 9 observations
		fprintf(out, "%6d", systems[0].obsType.size());
		for (unsigned int j=0; j<9; j++)
			if (j < systems[0].obsType.size())
				fprintf(out, "%4c%2.2s", ' ', systems[0].obsType[j].c_str());
			else
				fprintf(out, "%6c", ' ');
 		fprintf(out, "%-20s\n", "# / TYPES OF OBSERV");
 		break;
 	case V300:	//version 3.00
 		//limited implementation assuming maximum 13 observations per system
		for (unsigned int i=0; i<systems.size(); i++) {
 			fprintf(out, "%1c  %3d", systems[i].system, systems[i].obsType.size());
 			for (unsigned int j=0; j < 13; j++)
				if (j < systems[i].obsType.size())
 					fprintf(out, " %3s", systems[i].obsType[j].c_str());
 				else
 					fprintf(out, "%4c", ' ');
 			fprintf(out, "  %-20s\n", "SYS / # / OBS TYPES");
 		}
 		break;
 	default:
		fprintf(out, "%-60s%-20s\n", "INTERNAL ERROR. INCONSISTENT VERSION","COMMENT");
 	}
 	//observation interval
 	fprintf(out, "%10.3f%50c%-20s\n", obsInterval, ' ',"INTERVAL");
 	//format the time of first observation
	formatGPStime (timeBuffer, sizeof timeBuffer, "  %Y    %m    %d    %H    %M  ", firstObsWeek, firstObsTOW);
	fprintf(out, "%s%11.7f",
				timeBuffer, getGPSseconds (firstObsTOW));
 	fprintf(out, "%5c%3s%9c%-20s\n",
 				' ', "GPS", ' ', "TIME OF FIRST OBS" );
	fprintf(out, "%60c%-20s\n", ' ', "END OF HEADER");
}

/**printObsEpoch prints lines with one EPOCH observation data in the output RINEX file.
 * 
 * @param out the already open print stream where RINEX epoch data will be printed
 */
void RinexData::printObsEpoch(FILE* out) {
	char timeBuffer[80];
	//check if anything to print
 	if (observations.size() == 0) return;
	//sort observation data items available by system, satellite and measurement type
	sort(observations.begin(), observations.end(), obsCompare);
	//apply bias to measurements
	if (applyBias)
		for (unsigned int i=0; i<observations.size(); i++) {
			observations[i].obsValue -= clkBias * systems[observations[i].sysIndex].biasFactor[observations[i].obsTypeIndex];
		}
	//count the number of different satellites with data in this epoch (at least one)
	int nSatsEpoch = 1;
	for (unsigned int i=1; i<observations.size(); i++)
		if ((observations[i-1].sysIndex != observations[i].sysIndex) ||
			(observations[i-1].satellite != observations[i].satellite)) nSatsEpoch++;
	switch (version) {
	case V210:	//RINEX version 2.10
 		//print epoch 1st line
		//print calendar for the GPS time of this epoch
		formatGPStime (timeBuffer, 80, " %y %m %d %H %M", gpsWeek, epochTimeTag - (applyBias? clkBias: 0.0));
 		fprintf(out, "%s%11.7f", timeBuffer, getGPSseconds(epochTimeTag - (applyBias? clkBias: 0.0)));
 		fprintf(out, "  %1d%3d", epochFlag, nSatsEpoch);
		//print the different systems and satellites existing in this epoch
		fprintf(out, "%1c%02d", systems[observations[0].sysIndex].system, observations[0].satellite);
		for (unsigned int i=1; i<observations.size(); i++)
			if ((observations[i-1].sysIndex != observations[i].sysIndex) || (observations[i-1].satellite != observations[i].satellite)) 
				fprintf(out, "%1c%02d", systems[observations[i].sysIndex].system, observations[i].satellite);
		//fill the line and print clock bias used
		for (int i=nSatsEpoch; i<12; i++) fprintf(out, "%3c", ' ');	//???12 por constante
		fprintf(out, "%12.9f\n", clkBias);
		//for each satellite belonging to this epoch, print a line of measurements data and remove them from observations
		while (printSatObsValues(out));
 		break;
	case V300:	//RINEX version 3.00
		//print epoch 1st line
 		formatGPStime (timeBuffer, 80, "> %Y %m %d %H %M", gpsWeek, epochTimeTag - clkBias);
 		fprintf(out, "%s%11.7f", timeBuffer, getGPSseconds(epochTimeTag - clkBias));
 		fprintf(out, "  %1d%3d%5c%15.12f%3c\n", epochFlag, nSatsEpoch, ' ', clkBias, ' ');
		//for each satellite belonging to this epoch, print line of measurements data and remove them from observations
		do {
			fprintf(out, "%1c%02d", systems[observations[0].sysIndex].system, observations[0].satellite);
 		} while (printSatObsValues(out));
 		break;
 	default:
		fprintf(out, "%-60s%-20s\n", "INTERNAL ERROR. INCONSISTENT VERSION","COMMENT");
 	}
}

/**printSatObsValues prints a line with the satellite observation values.
 * Observation data are removed after printing.
 * Note that values are sorted in the observations storage by system, satellite and observation type.
 *
 * @param out the already open print stream where RINEX epoch data will be printed
 * @return true if they remain observations belonging to the current epoch, false when no data remains to print.
 */
bool RinexData::printSatObsValues(FILE* out) {
	double valueToPrint;
	if (observations.size() == 0) return false;
	int sysToPrint = observations[0].sysIndex;
	int satToPrint = observations[0].satellite;
	int obsToPrint = 0;
	while ((observations.size() > 0) &&
			(observations[0].sysIndex == sysToPrint) &&
			(observations[0].satellite == satToPrint)) {
		if (observations[0].obsTypeIndex == obsToPrint) {
			valueToPrint = observations[0].obsValue;
			//discard measurements out of range used in the RINEX format 14.3f
			if ((valueToPrint > MAXOBSVAL) || (valueToPrint < MINOBSVAL)) valueToPrint = 0.0;
			fprintf(out, "%14.3f", valueToPrint);
			if (observations[0].lossOfLock == 0) fprintf(out, " ");
			else fprintf(out, "%1d", observations[0].lossOfLock);
			if (observations[0].strength == 0) fprintf(out, " ");
			else fprintf(out, "%1d", observations[0].strength);
			observations.erase(observations.begin());
		} else {
			fprintf(out, "%14.3f  ", 0.0);
		}
		obsToPrint++;
	}
	fprintf(out, "\n");
	return (observations.size() > 0);
}

/**printEndOfFile prints the RINEX end of file event lines.
 * 
 * @param out	The already open print file where RINEX data will be printed
 */
 void RinexData::printObsEOF(FILE* out) {
	char timeBuffer[80];
	if (!appEnd) return;
 	//print header information for event "follows line"
	formatGPStime (timeBuffer, 80, " %y %m %d %H %M", gpsWeek, epochTimeTag - (applyBias? clkBias: 0.0));
 	fprintf(out, "%s%11.7f", timeBuffer, getGPSseconds(epochTimeTag - (applyBias? clkBias: 0.0)));
 	fprintf(out, "  %1d%3d\n", 4, 1);
	//print comment line
 	fprintf(out, "%-60s%-20s\n", "END OF FILE", "COMMENT");
}
 
 /**printGPSnavHeader prints RINEX GPS navigation file header using the current RINEX data.
 * 
 * @param out	The already open print file where RINEX header will be printed
 */
void RinexData::printGPSnavHeader(FILE* out) {
	char timeBuffer[80];
	//1st header line
	float v = 0;
	switch(version) {
	case V210: v = (float) 2.1; break;
	case V300: v = (float) 2.1; break;
	}
	//get local time and format it as needed
	formatLocalTime(timeBuffer, sizeof timeBuffer,"%Y%m%d %H%M%S ");
	fprintf(out, "%9.2f%11c%1c%-19s%20c%-20s\n",
 				v, ' ', 'N', " GPS NAV DATA", ' ', "RINEX VERSION / TYPE");
	fprintf(out, "%-20s%-20s%s%3s %-20s\n",
 				pgm.c_str(), runby.c_str(), timeBuffer, "LCL", "PGM / RUN BY / DATE");
/*TBW
  	//print iono parameters A0-A3 of almanac (page 18 of subframe 4)
	fprintf(out, "%-60.60s%-20s\n",
					, "ION ALPHA" );
	//print iono p<rameters B0-B3 of almanac
	fprintf(out, "%-60.60s%-20s\n",
					, "ION BETA" );
	//print almanac parameters to compute time in UTC (p 18, subframe 4)
	fprintf(out, "%-60.60s%-20s\n",
					, "DELTA-UTC: A0,A1,T,W" );
	//print Delta time due to leap seconds, V3.0: Number of leap seconds since 6-Jan-1980
	fprintf(out, "%-60.60s%-20s\n",
					, "LEAP SECONDS" );
*/
	fprintf(out, "%60c%-20s\n",
 				' ', "END OF HEADER");
}

/**printGPSnavEpoch prints lines with one EPOCH GPS navigation data in the output RINEX file
 * 
 * @param out the already open print file where RINEX epoch will be printed
 */
void RinexData::printGPSnavEpoch(FILE* out) {
	char timeBuffer[80];
	double d;
	int gpsW;
	double gpsT;

	//MSW especific!!
	_set_output_format(_TWO_DIGIT_EXPONENT);

	//for each satellite observed
	for (unsigned int i=0; i<gpsEphmNav.size(); i++) {
		//print epoch 1st line: first the satellite number
		fprintf(out, "%02d", gpsEphmNav[i].satellite);
		//next the calendar navigation data time (positive data in broadcastOrbit)
		gpsW = gpsEphmNav[i].broadcastOrbit[5][2];
		gpsT = gpsEphmNav[i].broadcastOrbit[0][0] * SCALEFACTORS[0][0];
		formatGPStime (timeBuffer, sizeof timeBuffer, "%y %m %d %H %M", gpsW, gpsT);
 		fprintf(out, " %s %4.1f", timeBuffer, getGPSseconds(gpsT));
		for (int k=1; k<4; k++)	//finally the Af0, 1 & 2 values (signed data)
			fprintf(out, "%19.12E", ((int) gpsEphmNav[i].broadcastOrbit[0][k]) * SCALEFACTORS[0][k]);
		fprintf(out, "\n");
		//print the other seven broadcast orbit data lines
		for (int j=1; j<8; j++) {
			fprintf(out, "   ");
			for (int k=0; k<4; k++) {
				//analyse special cases and do casting and assignement accordingly
				if (j==7 && k==2) break;	//do not print spares in last line
				if (j==7 && k==1) {			//compute the Fit Interval from fit flag
					if (gpsEphmNav[i].broadcastOrbit[7][1] == 0) d = 4.0;
					else {
						int iodc = gpsEphmNav[i].broadcastOrbit[6][3];
						if (iodc>=240 && iodc<=247) d = 8.0;
						else if (iodc>=248 && iodc<=255) d = 14.0;
						else if (iodc==496) d = 14.0;
						else if (iodc>=497 && iodc<=503) d = 26.0;
						else if (iodc>=1021 && iodc<=1023) d = 26.0;
						else d = 6.0;
					}
				} else if (j==6 && k==0)	//compute User Range Accuracy value
						d = URA[gpsEphmNav[i].broadcastOrbit[6][0]];
				else if (j==2 && (k==1 || k==3))	//e and sqrt(A) are 32 bits unsigned
					d = gpsEphmNav[i].broadcastOrbit[j][k] * SCALEFACTORS[j][k];
					//the rest signed, or unsigned but with less than 32 bits
				else d = ((int) gpsEphmNav[i].broadcastOrbit[j][k]) * SCALEFACTORS[j][k];
				fprintf(out, "%19.12E", d);
			}
			fprintf(out, "\n");
		}
	}
}

