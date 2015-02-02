/** @file RinexData.h
 * Contains RinexData class definition.
 * A RinexData object contains data for the header and epochs of the RINEX files.
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

#include <vector>

using namespace std;

//@cond DUMMY
//Constants usefull for computations
const double LSPEED = 299792458.0;		//the speed of light
const double L1CFREQ = 1575420000.0;	//the L1 carrier frequency
const double L2CFREQ = 1227600000.0;	//the L2 carrier frequency
const double L5CFREQ = 1176450000.0;	//the L5/E5a carrier frequency
const double L6CFREQ = 1278750000.0;	//the E6 carrier frequency
const double L7CFREQ = 1207140000.0;	//the E5b carrier frequency
const double L8CFREQ = 1191795000.0;	//the E5a+b carrier frequency
const double L1WLINV = L1CFREQ / LSPEED; //the inverse of L1 wave length
const double ThisPI = 3.1415926535898;
const double MAXOBSVAL = 9999999999.999; //the maximum value for any observable to fit the F14.4 RINEX format
const double MINOBSVAL = -999999999.999; //the minimum value for any observable to fit the F14.4 RINEX format

//data types
enum RINEXversion {V210, V300};
//internal classes
//SatObsData defines data for a satellite observation (pseudorrange, phase, ...) in one epoch.
struct SatObsData {	
	int sysIndex;		//the index inside a GNSSsystem vector of the system this observation belongs
	int satellite;		//the satellite PRN this observation belongs
	double epochTime;	//the epoch time for this observation according to the receiver (before solution)
	int obsTypeIndex;	//the index in obsType vector inside GNSSsystem of the observation type
 	double obsValue;	//the value of this observation
	int lossOfLock;		//if loss of lock happened when observation was taken
	int strength;		//the signal strength when observation was taken

	SatObsData (int, int, double, int, double, int, int);
};
//GPSsatNav used to define storage for navigation data for a given GPS satellite
struct GPSsatNav {
	int satellite;		//the PRN of the satellite they belong
	unsigned int broadcastOrbit[8][4];	//the eigth lines of RINEX data with four parameters each 

	GPSsatNav(int, unsigned int [8][4]);
};
//@endcond 
/**GNSSsystem defines data for each GNSS system that can provide data to the RINEX file.
 *
 */
struct GNSSsystem {
	char system;				///<system identification: G, R, S, E ... (see RINEX document)
	vector <string> obsType;	///<identifier of each obsType type: C1C, L1C, D1C, S1C... (see RINEX document)
	vector <double> biasFactor;	///<a factor to apply bias to observations, like speed of light for pseudoranges, carrier frequency for phase 

	GNSSsystem (char sys, vector <string> obsT);
};

/**RinexData class defines a data container for the RINEX file data and the parameters to be used to generate it.
 * Usually programs use the GNSSDataAcq class to extract data from binary files which destination is a RinexData object.
 *<p>
 * A detailed definition of the RINEX format can be found in the document "RINEX: The Receiver Independent Exchange
 * Format Version 2.10" from Werner Gurtner; Astronomical Institute; University of Berne. An updated document exists
 * also for Version 3.00.
 *<p>
 * NOTE: RINEX version 2.10 is implemented with some limitations:
 *	- all systems shall have the same observables
 *	- observables shall be in the same order for all systems
 *	- the maximum number of observables is 9
 */
class RinexData {
	//Header data
	RINEXversion version;	//The format version to be generated
	string pgm;				//Program used to create current file
	string runby;			//Who executed the program
	string markerName;		//Name of antenna marker
	string markerNumber;	//Number of antenna marker (HUMAN)
	string markerType;		//Marker type as per V300
	string observer;		//Name of observer
	string agency;			//Name of agency
	string rxNumber;		//Receiver number
	string rxType;			//Receiver type
	string rxVersion;		//Receiver version (e.g. Internal Software Version)
	string antNumber;		//Antenna number
	string antType;			//Antenna type
	float aproxX;			//Geocentric approximate marker position
	float aproxY;
	float aproxZ;
	float antHigh;		//Antenna height: Height of the antenna reference point (ARP) above the marker
	float eccEast;		//Horizontal eccentricity of ARP relative to the marker (east/north)
	float eccNorth;
	int wvlenFactorL1;
	int wvlenFactorL2;
	int firstObsWeek;	//Time of the first observation
	double firstObsTOW;
	float obsInterval;
	//Generation parameters
	int gpsWeek;		//Extended (0 to NO LIMIT) GPS week number of current epoch). From MID7
	double gpsTOW;		//Seconds into the current week, accounting for clock bias, when the current measurement was made. From MID7
	double epochTimeTag;	//The estimated GPS time of current epoch time as computed before fix. From MID28
	double clkBias;			//Difference between estimated GPS time (before fix) and the one computed after fix. From MID7
	bool applyBias;			//if the receiver clock bias shall be applied to observations and time
	int epochFlag;			//see RINEX definition
	vector <GNSSsystem> systems;
	vector <SatObsData> observations;
	vector <GPSsatNav> gpsEphmNav;
	bool appEnd;			//if end of file comment will be appended or not
	double SCALEFACTORS[8][4];	//the scale factors to apply to obtain broadcast orbit data
	double URA[16];

	string getRINEXfileName(string designator, int week, int sec, char ftype);

public:
	RinexData(string, string, string, string, string, string, string, string, string, bool, bool, vector <GNSSsystem>);
	~RinexData(void);
	void setPosition(float, float, float);
	void setReceiver(string, string, string, int, int);
	void setGPSTime(int, double, double);
	double getGPSTime ();
	string getObsFileName(string ); 
	string getGPSnavFileName(string );
	void setFistObsTime();
	void setIntervalTime(int, double);
	bool addMeasurement (char, int, string, double, int, int, double);
	bool addGPSNavData (int, unsigned int [8][4]);
	void clearObs();
	void printObsHeader(FILE* out);
	void printObsEpoch(FILE* out);
	bool printSatObsValues(FILE* out);
	void printObsEOF(FILE* out);
	void printGPSnavHeader(FILE* out);
	void printGPSnavEpoch(FILE* out);
};
