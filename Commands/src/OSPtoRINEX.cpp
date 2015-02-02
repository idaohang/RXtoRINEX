/** @file OSPtoRINEX.cpp
 * Contains the command line program to generate RINEX files from an OSP data file containing SiRF IV receiver messages. 
 *<p>Usage:
 *<p>OSPtoRINEX.exe {options} [OSPfilename]
 *<p>Options are:
 *	- -a or --aend : Don't append end-of-file comment lines to Rinex file. Default value AEND=TRUE
 *	- -b or --bias : Don't apply receiver clock bias to measurements and time. Default value BIAS=TRUE
 *	- -c GPS or --gpsc=GPS : GPS code measurements to include (comma separated). Default value GPS = C1C,L1C,D1C,S1C
 *	- -e or --ephemeris : Don't use MID15 (rx ephemeris) to generate GPS nav file. Default value EPHEM=TRUE
 *	- -g or --GPS50bps : Use MID8 (50bps data) to generate GPS nav file. Default value G50BPS=FALSE
 *	- -h or --help : Show usage data and stops. Default value HELP=FALSE
 *	- -i MINSV or --minsv=MINSV : Minimun satellites in a fix to acquire observations. Default value MINSV = 4
 *	- -j ANTN or --antnum=ANTN : Receiver antenna number. Default value ANTN = Antenna#
 *	- -k ANTT or --antype=ANTT : Receiver antenna type. Default value ANTT = AntennaType
 *	- -l LOGLEVEL or --llevel=LOGLEVEL : Maximum level to log (SEVERE, WARNING, INFO, CONFIG, FINE, FINER, FINEST). Default value LOGLEVEL = INFO
 *	- -m MRKNAM or --mrkname=MRKNAM : Marker name. Default value MRKNAM = MRKNAM
 *	- -n or --nRINEX : Generate RINEX GPS navigation file. Default value NAVI=FALSE
 *	- -o OBSERVER or --observer=OBSERVER : Observer name. Default value OBSERVER = OBSERVER
 *	- -p RUNBY or --runby=RUNBY : Who runs the RINEX file generator. Default value RUNBY = RUNBY
 *	- -r RINEX or --rinex=RINEX : RINEX file name prefix. Default value RINEX = PNT1
 *	- -s SBAS or --sbas=SBAS : SBAS measurements to include. Default value SBAS = C1C,L1C,D1C,S1C
 *	- -t MID or --last=MID : MID (Message ID) of last OSP message in an epoch. Default value MID = 7
 *	- -u MRKNUM or --mrknum=MRKNUM : Marker number. Default value MRKNUM = MRKNUM
 *	- -v VER or --ver=VER : RINEX version to generate (V210, V300). Default value VER = V210
 *	- -y AGENCY or --agency=AGENCY : Agency name. Default value AGENCY = AGENCY
 *Default values for operators are: DATA.OSP 
 *<p>
 *Copyright 2015 Francisco Cancillo
 *<p>
 *This file is part of the RXtoRINEX tool.
 *<p>
 *RXtoRINEX is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
 *as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *RXtoRINEX is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 *warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *See the GNU General Public License for more details.
 *A copy of the GNU General Public License can be found at <http://www.gnu.org/licenses/>.
 *
 *V1.0	First release
 */

//from CommonClasses
#include "ArgParser.h"
#include "Logger.h"
#include "Utilities.h"
#include "GNSSDataAcq.h"
#include "RinexData.h"

using namespace std;

///The command line format
const string CMDLINE = "OSPtoRINEX.exe {options} [OSPfilename]";
///The receiver name
const string RECEIVER = "SiRFIV";
//@cond DUMMY
///The parser object to store options and operators passed in the comman line
ArgParser parser;
//Metavariables for options
int AGENCY, AEND, ANTN, ANTT, BIAS, EPHEM, G50BPS, GPS, HELP, LOGLEVEL, NAVI, MID, MINSV, MRKNAM, MRKNUM, OBSERVER, RINEX, RUNBY, SBAS, VER;
//Metavariables for operators
int OSPF;
//@endcond 
//functions in this file
int generateRINEX(FILE*, Logger*);

/**main
 * gets the command line arguments, set parameters accordingly and triggers the data acquisition to generate RINEX files.
 * Input data are contained  in a OSP binary file containing receiver messages (see SiRF IV ICD for details).
 * The output is a RINEX observation data file, and optionally a RINEX navigation data file.
 * A detailed definition of the RINEX format can be found in the document "RINEX: The Receiver Independent Exchange
 * Format Version 2.10" from Werner Gurtner; Astronomical Institute; University of Berne. An updated document exists
 * also for Version 3.00.
 *
 *@param argc the number of arguments passed from the command line
 *@param argv the array of arguments passed from the command line
 *@return  the exit status according to the following values and meaning::
 *		- (0) no errors have been detected
 *		- (1) an error has been detected in arguments
 *		- (2) error when opening the input file
 *		- (3) error when creating output files or no epoch data exist
 */
int main(int argc, char* argv[]) {
	/**The main process sequence follows:*/
	/// 1- Defines and sets the error logger object
	Logger log("LogFile.txt");
	log.setPrgName(argv[0]);
	/// 2- Setups the valid options in the command line. They will be used by the argument/option parser
	AGENCY = parser.addOption("-y", "--agency", "AGENCY", "Agency name", "AGENCY");
	VER = parser.addOption("-v", "--ver", "VER", "RINEX version to generate (V210, V300)", "V210");
	MRKNUM = parser.addOption("-u", "--mrknum", "MRKNUM", "Marker number", "MRKNUM");
	MID = parser.addOption("-t", "--last", "MID", "MID (Message ID) of last OSP message in an epoch", "7");
	SBAS = parser.addOption("-s", "--sbas", "SBAS", "SBAS measurements to include", "C1C,L1C,D1C,S1C");
	RINEX = parser.addOption("-r", "--rinex", "RINEX", "RINEX file name prefix", "PNT1");
	RUNBY = parser.addOption("-p", "--runby", "RUNBY", "Who runs the RINEX file generation", "RUNBY");
	OBSERVER = parser.addOption("-o", "--observer", "OBSERVER", "Observer name", "OBSERVER");
	NAVI = parser.addOption("-n", "--nRINEX", "NAVI", "Generate RINEX GPS navigation file", false);
	MRKNAM = parser.addOption("-m", "--mrkname", "MRKNAM", "Marker name", "MRKNAM");
	LOGLEVEL = parser.addOption("-l", "--llevel", "LOGLEVEL", "Maximum level to log (SEVERE, WARNING, INFO, CONFIG, FINE, FINER, FINEST)", "INFO");
	ANTT = parser.addOption("-k", "--antype", "ANTT", "Receiver antenna type", "AntennaType");
	ANTN = parser.addOption("-j", "--antnum", "ANTN", "Receiver antenna number", "Antenna#");
	MINSV = parser.addOption("-i", "--minsv", "MINSV", "Minimun satellites in a fix to acquire observations", "4");
	HELP = parser.addOption("-h", "--help", "HELP", "Show usage data and stops", false);
	G50BPS = parser.addOption("-g", "--GPS50bps", "G50BPS", "Use MID8 (50bps data) to generate GPS nav file", false);
	EPHEM = parser.addOption("-e", "--ephemeris", "EPHEM", "Don't use MID15 (rx ephemeris) to generate GPS nav file", true);
	GPS = parser.addOption("-c", "--gpsc", "GPS", "GPS code measurements to include (comma separated)", "C1C,L1C,D1C,S1C");
	BIAS = parser.addOption("-b", "--bias", "BIAS", "Don't apply receiver clock bias to measurements and time", true);
	AEND = parser.addOption("-a", "--aend", "AEND", "Don't append end-of-file comment lines to Rinex file", true);
	/// 3- Setups the default values for operators in the command line
	OSPF = parser.addOperator("DATA.OSP");
	/// 4- Parses arguments in the command line extracting options and operators
	try {
		parser.parseArgs(argc, argv);
	}  catch (string error) {
		parser.usage("Argument error: " + error, CMDLINE);
		log.severe(error);
		return 1;
	}
	log.info(parser.showOptValues());
	log.info(parser.showOpeValues());
	if (parser.getBoolOpt(HELP)) {
		//help info has been requested
		parser.usage("Generates RINEX files from an OSP data file containing SiRF IV receiver messages", CMDLINE);
		return 0;
	}
	/// 5- Sets logging level stated in option
	string s = parser.getStrOpt(LOGLEVEL);
	if (s.compare("SEVERE") == 0) log.setLevel(SEVERE);
	else if (s.compare("WARNING") == 0) log.setLevel(WARNING);
	else if (s.compare("INFO") == 0) log.setLevel(INFO);
	else if (s.compare("CONFIG") == 0) log.setLevel(CONFIG);
	else if (s.compare("FINE") == 0) log.setLevel(FINE);
	else if (s.compare("FINER") == 0) log.setLevel(FINER);
	else if (s.compare("FINEST") == 0) log.setLevel(FINEST);
	/// 6- Opens the OSP binary file
	FILE* inFile;
	string fileName = parser.getOperator (OSPF);
	if ((inFile = fopen(fileName.c_str(), "rb")) == NULL) {
		log.severe("Cannot open file " + fileName);
		return 2;
	}
	/// 7- Calls generateRINEX to generate RINEX files extracting data from messages in the binary OSP file
	int n = generateRINEX(inFile, &log);
	fclose(inFile);
	log.info("End of RINEX generation. Epochs read: " + to_string((long long) n));
	return n>0? 0:3;
}
/**generateRINEX iterates over the input OSP file processing GNSS receiver messages to extract RINEX data and print them.
 *
 *@param inFile is the FILE containing the binary OSP messages
 *@param plog point to the Logger
 *@return the number of epochs read in the inFile
 */
int generateRINEX(FILE* inFile, Logger* plog) {
	/**The generateRINEX process sequence follows:*/
	int epochCount;		//to count the number of epochs processed
	string outFileName;	//the output file name for RINEX files
	FILE* outFile;		//the open file where RINEX data will be printed
	/// 1- Setups the RinexData object elements with data given in command line options  
	//a vector to contain the GNSS system data used by this receiver
	vector <GNSSsystem> systems; 
	systems.push_back(GNSSsystem('G', getTokens(parser.getStrOpt(GPS), ',')));
	systems.push_back(GNSSsystem('S', getTokens(parser.getStrOpt(SBAS), ',')));
	//the RinexData object where RINEX data are to be placed for further printing
	RinexData rinex(
		parser.getStrOpt(VER),
		"OSPtoRINEX",
		parser.getStrOpt(RUNBY),
		parser.getStrOpt(MRKNAM),
		parser.getStrOpt(MRKNUM),
		parser.getStrOpt(ANTN),
		parser.getStrOpt(ANTT),
		parser.getStrOpt(OBSERVER),
		parser.getStrOpt(AGENCY),
		parser.getBoolOpt(AEND),
		parser.getBoolOpt(BIAS),
		systems);
	/// 2- Setups the GNSSDataAcq object used to extract message data from the OSP file
	GNSSDataAcq gnssAcq(RECEIVER, stoi(parser.getStrOpt(MINSV)), inFile, plog);
	/// 3- Starts data acquisition extracting RINEX header data located in the binary file
	if(!gnssAcq.acqHeaderData(rinex)) {
		plog->warning("All, or some header data not acquired");
	};
	/// 4- Generates RINEX observation filename in standard format and creates it
	outFileName = rinex.getObsFileName(parser.getStrOpt (RINEX));
	if ((outFile = fopen(outFileName.c_str(), "w")) == NULL) {
		plog->severe("Cannot create file " + outFileName);
		return 0;
	}
	/// 5- Prints RINEX observation file header
	rinex.printObsHeader(outFile);
	/// 6- Iterates over the binary OSP file extracting epoch by epoch data and printing them
	epochCount = 0;
	bool useEphem = parser.getBoolOpt(EPHEM);
	bool useG50bps = parser.getBoolOpt(G50BPS);
	rewind(inFile);
	while (gnssAcq.acqEpochData(rinex, useEphem, useG50bps)) {
		rinex.printObsEpoch(outFile);
		epochCount++;
	}
	rinex.printObsEOF(outFile);
	fclose(outFile);
	/// 7- Generates the Rinex navigation file, if requested
	if (parser.getBoolOpt (NAVI)) {
		//get RINEX GPS navigation in standard format and open it
		outFileName = rinex.getGPSnavFileName(parser.getStrOpt (RINEX));
		if ((outFile = fopen(outFileName.c_str(), "w")) == NULL) {
			plog->severe("Cannot create file " + outFileName);
			return 0;
		}
		rinex.printGPSnavHeader(outFile);	//print GPS navigation file header
		rinex.printGPSnavEpoch(outFile);	//print GPS navigation file epoch data
		fclose(outFile);
	}
	return epochCount;
}