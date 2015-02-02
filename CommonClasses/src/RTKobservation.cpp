/** @file RTKobservation.cpp
 * Contains the implementation of the class RTKobservation.
 */
#include "RTKobservation.h"

//from CommonClasses
#include "Utilities.h"

/**Constructor for RTKobservation objects
 *
 *@param plog a pointer to the logger to be used
 */
RTKobservation::RTKobservation(Logger* plog) {
		logger = plog;
}

/**Destructs an RTKobservation object
  */
RTKobservation::~RTKobservation(void) {
}

/**setId sets identification data in the header of the RTK file.
 *
 * @param prg	the name of program used to generate positioning data 
 * @param in	the input file from were data have been extracted
 */
void RTKobservation::setId (string prg, string in) {
	program = prg;
	inpFile = in;
	posMode = "Single";
	freqs = "L1";
	solution = "N/A";
	elevMask = 0.0;
	snrMask = 0.0;
	ionosEst = "Broadcast";
	troposEst = "OFF";
	ambEst = "N/A";
	valThres = "N/A";
	ephemeris = "Broadcast";
}

/**setMasks sets mask data in the header of the RTK file.
 *
 * @param elev	the elevation mask value 
 * @param snr	the signal to noise ratio value
 */
void RTKobservation::setMasks (double elev, double snr) {
	elevMask = elev;
	snrMask = snr;
}

/**setStartTime sets start time in the header for the RTK file as the gps Week and TOW of the current epoch.
 *
 */
void RTKobservation::setStartTime() {
	startWeek = gpsWeek;
	startTOW = gpsTOW;
}

/**setEndTime sets end time in the header for the RTK file as the gps Week and TOW of the current epoch.
 *
 */
void RTKobservation::setEndTime() {
	endWeek = gpsWeek;
	endTOW = gpsTOW;
}

/**setPosition sets position solution data of the current epoch
 *
 * @param week	the gps week value 
 * @param tow	the gps time of week value
 * @param x		the X coordinate value computed bu the receiver
 * @param y		the Y coordinate value computed bu the receiver
 * @param z		the Z coordinate value computed bu the receiver
 * @param qlty	the quality of the solution
 * @param nSat	the number of satellites used to compute the solution
 */
void RTKobservation::setPosition(int week, double tow, double x, double y, double z, int qlty, int nSat) {
	gpsWeek = week;
	gpsTOW = tow;
	xSol = x;
	ySol = y;
	zSol = z;
	qSol = qlty;
	nSol = nSat;
}

/**printHeader prints header data to the RTK file.
 *
 * @param out	the FILE were header will be printed
 */
void RTKobservation::printHeader(FILE* out) {
	char buffer[80];
 	fprintf(out, "%% program\t: %s\n", program.c_str());
	fprintf(out, "%% inp file\t: %s\n", inpFile.c_str());
	formatGPStime (buffer, sizeof buffer,"%Y/%m/%d %H:%M", startWeek, startTOW);
	fprintf(out, "%% obs start\t: %s:%06.3f GPST\n", buffer, getGPSseconds(startTOW));
	formatGPStime (buffer, sizeof buffer,"%Y/%m/%d %H:%M", endWeek, endTOW);
	fprintf(out, "%% obs end\t: %s:%-06.3f GPST\n", buffer, getGPSseconds(endTOW));
	fprintf(out, "%% pos mode\t: %s\n", posMode.c_str());
	fprintf(out, "%% elev mask\t: %4.1f\n", elevMask);
	fprintf(out, "%% snr mask\t: %4.1f\n", snrMask);
	fprintf(out, "%% ionos opt\t: %s\n", ionosEst.c_str());
	fprintf(out, "%% tropo opt\t: %s\n", troposEst.c_str());
	fprintf(out, "%% ephemeris\t: %s\n", ephemeris.c_str());
	fprintf(out, "%%\n%% (x/y/z-ecef=WGS84,Q=1:fix,2:float,3:sbas,4:dgps,5:single,6:ppp,ns=# of satellites)\n");
	fprintf(out, "%%  GPST%19c%s\n",
			' ',
			"   x-ecef(m)      y-ecef(m)      z-ecef(m)   Q  ns   sdx(m)   sdy(m)   sdz(m)  sdxy(m)  sdyz(m)  sdzx(m) age(s)  ratio");
}

/**printSolution prints a line to the RTK file with solution data from the current epoch.
 *
 * @param out	the FILE were header will be printed
 */
void RTKobservation::printSolution (FILE* out) {
	char buffer[80];
	formatGPStime (buffer, sizeof buffer,"%Y/%m/%d %H:%M", gpsWeek, gpsTOW);
	fprintf(out, "%s:%06.3f", buffer, getGPSseconds(gpsTOW));
	fprintf(out, " %14.4f %14.4f %14.4f %3d %3d", xSol, ySol, zSol, qSol, nSol);
	for (int i=0; i<6; i++)
		fprintf(out, " %8.4f", 0.0);
	fprintf(out, "   0.00    0.0");
	fprintf(out, "\n");
}
