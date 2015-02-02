/** @file GNSSDataAcq.cpp
 * Contains the implementation of the GNSSDataAcq class for OSP data collected from SiRFIV receivers.
 * This class provides public methods to allow extraction from an OSP file of:
 * - data parameters to be included in the header of RINEX or RTK files (by method acqHeaderData),
 * - epoch observation data (by method acqEpochData)
 * Above public methods use private ones defined for extracting from each type of OSP message parameters
 * and data required in the RINEX and RTK file formats. 
 */

#include "GNSSDataAcq.h"

/**Construct a GNSSDataAcq object using parameters passed.
 *
 *@param rcv the receiver name
 *@param minxfix the minimum of satellites required for a fix to be considered
 *@param f the FILE pointer to the input OSP binary file
 *@param pl a pointer to the Logger to be used to record logging messages
 */
GNSSDataAcq::GNSSDataAcq(string rcv, int minxfix, FILE* f, Logger * pl) {
	receiver = rcv;
	minSVSfix = minxfix;
	ospFile = f;
	log = pl;
	for (int i=0; i<MAXCHANNELS; i++)
		for (int j=0; j<MAXSUBFR; j++)
			subfrmCh[i][j].sv = 0;
}

/**Destroys a GNSSDataAcq object
 */
GNSSDataAcq::~GNSSDataAcq(void) {
}

/**acqHeaderData extracts data from the binary file for a RINEX file header.
 * The RINEX header data to be extracted from the binary file are:
 * - the receiver identification contained in the first MID6 message
 * - the initial X,Y,Z position contained in the first MID2 message
 * - the time of first epoch contained in the first valid MID7 message
 * - the measurement interval computed as the time difference between two consecutive valid MID7 
 * The method iterates over the input file extracting messages until above describe data are acquired
 * or it reaches the end of file.
 * It logs at FINE level a message stating which header data have been acquired or not.
 *
 * @param rinex the RinexData object where data got from receiver will be placed
 * @return	true if all above described header data are properly extracted, false otherwise
 */
bool GNSSDataAcq::acqHeaderData(RinexData& rinex) {
	bool rxIdSet = false;	//identification of receiver not set
	bool apxSet = false;	//approximate position not set
	bool frsEphSet = false;	//first epoch time not set
	bool intrvBegin = false; //interval begin time has been stated		
	bool intrvSet = false;	//observations interval not set
	int mid;
	while (message.fill(ospFile) &&		//there are messages in the binary file
			!(apxSet && rxIdSet && frsEphSet && intrvSet)) {	//not all header data have been adquired
		mid = message.get();		//get first byte (MID)
		switch(mid) {
		case 2:	//collect first MID2 data to obtain approximate position (X, Y, Z)
			if (!apxSet) apxSet = getMID2PosData(rinex);
			break;
		case 6:	//extract the software version
			if (!rxIdSet) rxIdSet = getMID6RxData(rinex);
			break;
		case 7: //collect MID7 data for first epoch and the following MID7 data for interval
			if (!frsEphSet) {
				intrvBegin = frsEphSet = getMID7TimeData(rinex);
				rinex.setFistObsTime();
			}
			else if (!intrvBegin) intrvBegin = getMID7TimeData(rinex);
			else if (!intrvSet) intrvBegin = intrvSet = getMID7Interval(rinex);
			break;
		default:
			break;
		}
		//printf("mid: %d rxIdSet=%s apxSet=%s frsEphSet=%s\n", mid, rxIdSet? "Y":"N", apxSet? "Y":"N", frsEphSet? "Y":"N");
	}
	//log data sources available or not
	string logMessage = "RINEX header data available: AproxPosition ";
	logMessage += apxSet?  "YES": "NO";
	logMessage += ";First epoch time ";
	logMessage += frsEphSet?   "YES": "NO";
	logMessage += ";Observation interval ";
	logMessage += intrvSet?   "YES": "NO";
	logMessage += ";Receiver version ";
	logMessage += rxIdSet?   "YES": "NO";
	log->fine(logMessage);
	return (apxSet && frsEphSet && rxIdSet && intrvSet);
}

/**acqHeaderData extracts data from the binary file for a RTK file header.
 * The RTK header data to be extracted from the binary file are:
 * - the time of first computed solution contained in the first valid MID2 message
 * - the time of last computed solution contained in the last valid MID2 message
 * - the elevation and SNR masks used by the receiver contained in the first valid MID19 message
 *<p>
 * The method iterates over the input file extracting messages until above describe data are acquired
 * or it reaches the end of file.
 *<p>
 * It logs at FINE level a message stating which header data have been acquired or not.
 *
 * @param rtko the RTKobservation object where data got from receiver will be placed
 * @return true if all above described header data are properly extracted, false otherwise
 */
bool GNSSDataAcq::acqHeaderData(RTKobservation& rtko) {
	bool maskSet = false;	//mask data set
	bool fetSet = false;	//first epoch time set
	int mid;
	//acquire mask data and first and last epoch time
	while (message.fill(ospFile)) {	//there are messages in the binary file
		mid = message.get();		//get first byte (MID)
		switch(mid) {
		case 2:
			if (getMID2PosData(rtko)) {
				if (!fetSet)	{
					rtko.setStartTime();
					fetSet = true;
				}
				rtko.setEndTime();
			}
			break;
		case 19:	//collect MID19 with masks
			maskSet = getMID19Masks(rtko);
			break;
		default:
			break;
		}
	}
	//log data sources available or not
	string logMessage = "RTKO header data available: Fist epoch time ";
	logMessage += fetSet?  "YES": "NO";
	logMessage += ";Mask data ";
	logMessage += maskSet?   "YES": "NO";
	log->fine(logMessage);
	return maskSet && fetSet;
}

/**acqEpochData extracts observation and time data from binary file messages for a RINEX epoch.
 * Epoch RINEX data are contained in a sequence of {MID28} [MID2] [MID7] messages.
 *<p>
 * Each MID28 contains observables for a given satellite. Successive MID28 messages belong to the same epoch
 * if they have the same receiver time. In each epoch the receiver sends a MID28 message for each satellite
 * being tracked.
 *<p>
 * After sending the epoch MID28 messages, the receiver would send a MID2 message containing the computed
 * position solution data.
 *<p>
 * The epoch finishes with a MID7 message. It contains clock data for the current epoch, including its GPS time.
 * An unexpected end of epoch occurs when a MID28 message shows different receiver time from former received messages.
 *<p>
 * The method acquires data reading messages recorded in the binary file according to the above sequence. 
 * Epoch data are stored in the RINEX data structure for further generation/printing of RINEX observation records.
 * Method returns when it is read the last message of the current epoch. The "unexpected end" event above described
 * would be logged at info level.
 *<p>
 * Ephemeris data messages (MID15 or MID8) can appear in any place of the input message sequence. Their data
 * would be stored (depending on useMID15 and useMID8 passed values) for further use to generate the RINEX
 * navigation file.
 *<p>
 * Other messages in the binary file are ignored.
 *
 * @param rinex the RinexObsData object where data got from receiver will be placed
 * @param useMID15 to acquire (when true) or ignore (when false) navigation data in MID15 messages
 * @param useMID8 to acquire (when true) or ignore (when false) navigation data in MID8 messages
 * @return	true when observation data from an epoch have been acquired, false otherwise (End Of File reached)
 */
bool GNSSDataAcq::acqEpochData(RinexData& rinex, bool useMID15, bool useMID8) {
	int mid;
	bool sameEpoch;
	fpos_t msgPos;
	bool dataAvailable = false;	//there are data available when at least a MID28 msg has been received
	fgetpos(ospFile, &msgPos);	//get the current position in the binary file 
	while (message.fill(ospFile)) {	//one message has been read from the binary file
		mid = message.get();		//get first byte (MID) from message
		switch(mid) {
		case 7:		//the Rx sends MID7 when position for current epoch is computed (after sending MID28 msgs)
			if (getMID7TimeData(rinex) && dataAvailable) return true;
			break;
		case 8:		//collect 50BPS ephemerides data in MID8
			if (useMID8) getMID8NavData(rinex);
			break;
		case 15:	//collect complete ephemerides data in MID15
			if (useMID15) getMID15NavData(rinex);
			break;
		case 28:	//collect satellite measurements from a channel in MID28
			if (getMID28NavData(rinex, sameEpoch)) {	//message data are correct and have been stored
				if (sameEpoch) {	//data belong to the same epoch as former messages
					dataAvailable = true;
				}
				else {	//all data for the current epoch have been acquired, and no MID7 has arrived!
					fsetpos(ospFile, &msgPos);	//rewind to allow further re-extraction of last message
					rinex.clearObs();	//as no MID7 has been received, the bias to apply is unknown
					log->info("A MID28 sequence without MID7  in epoch " + to_string((long double) rinex.getGPSTime()));
					return dataAvailable;
				}
			}
			break;
		default:
			break;
		}
		fgetpos(ospFile, &msgPos);
	}
	return  dataAvailable;
}

/**acqEpochData acquires epoch position data for RTK observation files.
 * Epoch RTK data are contained in a MID2 message.
 *<p>
 * The method skips messages from the input binary file until a MID2 message is read.
 * When this happens, it stores MID2 data in the RTKobservation object passed and returns.
 *
 * @param rtko the RTKobservation object where data acquired will be placed
 * @return true if epoch position data properly extracted, false otherwise
 */
bool GNSSDataAcq::acqEpochData(RTKobservation& rtko) {
	int mid;
	while (message.fill(ospFile)) {	//one message has been read from the binary file
		mid = message.get();		//get first byte (MID) from message
		switch(mid) {
		case 2:		//the MID2 contains position data for this epoch
			if (getMID2PosData(rtko)) return true;
			break;
		default:
			break;
		}
	}
	return  false;
}

/**getMID2PosData gets solution data from a MID2 message for a RINEX file.
 *
 *@param rinex	the object where acquired data are stored
 *@return true if data properly extracted (correct message length and
 *		SVs in solution greather than minimum), false otherwise
 */
bool GNSSDataAcq::getMID2PosData(RinexData& rinex) {
	if (message.payloadLen() != 41) {
		log->info("MID2 msg len <> 41");
		return false;
	}
	//extract X, Y, Z for the rinex header
	float x = (float) message.getInt();	//get X
	float y = (float) message.getInt();	//get Y
	float z = (float) message.getInt();	//get Z
	message.skipBytes(15);	//skip from vX to GPS TOW: 3*2S + 3*1U +2U +4U = 15
	//check if fix has the minimum SVs required
	if (message.get() < minSVSfix) {
		log->finest("MID2 wrong fix: SVs less than minimum");
		return false;
	}
	rinex.setPosition( x, y, z);
	return true;
}

/**getMID2PosData gets solution data from a MID2 message for a RTK file.
 *
 *@param rtko the object where acquired data are stored
 *@return true if data properly extracted (correct message length), false otherwise
 */
bool GNSSDataAcq::getMID2PosData(RTKobservation& rtko) {
	if (message.payloadLen() != 41) {
		log->info("MID2 msg len <> 41");
		return false;
	}
	double x = (double) message.getInt();
	double y = (double) message.getInt();
	double z = (double) message.getInt();
	message.skipBytes(9);	//skip from vX to Mode2: 3*2S 3*1U
	int week = (int) message.getUShort() + 1024;
	double tow = (double) message.getInt() / 100.0;	//get GPS TOW (scaled by 100)
	int nsv = message.get();
	//check if fix has the minimum SVs required
	if (nsv < minSVSfix) {
		log->finest("MID2 wrong fix: SVs less than minimum");
		return false;
	}
	//it is assumed that "quality" is 5. No data exits in OSP messages to obtain it
	rtko.setPosition(week, tow, x, y, z, 5, nsv);
	return true;
}

/**getMID6RxData gets receiver identification data from a MID6 message.
 * Data acquired are stored in the RinexData object passed.
 *
 *@param rinex the class instance where data are stored
 *@return true if data properly extracted, false otherwise
 */
bool GNSSDataAcq::getMID6RxData(RinexData& rinex) {
	//Note: current structure of this message does not correspond with what is stated in ICD
	//next byte is SWversionLen
	int SWversionLen = message.get();
	//next one is SWcustomerLen
	int SWcustomerLen = message.get();
	//verify length of fields and message
	if (message.payloadLen() != (1 + 2 + SWversionLen + SWcustomerLen)) {
		log->info("In MID6, message/receiver/customer length don't match");
		return false;
	}
	//extract SWversion from message char by char
	string SWversion;
	string SWcustomer;
	char c;
	while (SWversionLen-- > 0) {
		c = (char) (message.get() & 0xFF);
		SWversion += c;
	}
	//extract SWcustomer from buffer
	while (SWcustomerLen-- > 0) {
		c = (char) (message.get() & 0xFF);
		SWcustomer += c;
	}
	rinex.setReceiver(SWversion, receiver, SWversion.substr(SWversion.find("GSD4")), 1, 0);
	return true;
}

/**getMID7TimeData gets GPS time data from a MID7 message.
 * Data acquired are stored in the RinexData object passed.
 *
 * @param rinex	the class instance where data will be stored
 * @return true if data properly extracted (correct message length and
 *		SVs in solution greather than minimum), false otherwise
 */
bool GNSSDataAcq::getMID7TimeData(RinexData& rinex) {
	if (message.payloadLen() != 20) {
		log->info("MID7 msg len <> 20");
		return false;
	}
	int week = (int) message.getUShort();		//get GPS Week (includes rollover)
	double tow = (double) message.getUInt() / 100.0;	//get GPS TOW (scaled by 100)
	int sats = (int) message.get();			//get number of satellites in the solution
	if (sats < minSVSfix) {
		log->finest("MID7 ignored: solution only " + to_string((long long) sats) + " sats");
		return false;
	}
	double drift = (double) message.getUInt();	//get receiver clock drift (change rate of bias in Hz)
	//get receiver clock bias in nanoseconds (unsigned 32 bits int) and convert to seconds
	double bias = (double) message.getUInt() * 1.0e-9;
	rinex.setGPSTime(week, tow, bias);
	return true;
}

/**getMID7Interval get GPS time data from MID7 message and compute interval from first observation
 * 
 * Time interval is stored in the rinex data class
 * 
 * @param rinex	the class instance where data will be stored
 * @return	true if MID data is usable, false otherwise
 */
bool GNSSDataAcq::getMID7Interval(RinexData& rinex) {
	if (message.payloadLen() != 20) {
		log->info("MID7 msg len <> 20");
		return false;
	}
	int week = (int) message.getUShort();		//get GPS Week (includes rollover)
	double tow = (double) message.getUInt() / 100.0;	//get GPS TOW (scaled by 100)
	int sats = (int) message.get();			//get number of satellites in the solution
	if (sats < minSVSfix) {
		log->finest("MID7 ignored: solution only " + to_string((long long) sats) + " sats");
		return false;
	}
	rinex.setIntervalTime(week, tow);
	return true;
}
/**
 * getMID8NavData gets navigation data from a MID 8 message
 * 
 * @param rinex		the class instance where data are stored
 */
bool GNSSDataAcq::getMID8NavData(RinexData& rinex) {
	if (message.payloadLen() != 43) {
		log->info("MID8 msg len <> 43");
		return false;
	}
	unsigned int wd[10];	//a place to store the ten words of GPS message
	unsigned int dt[45];	//a place to pack message data as per MID 15 (see SiRF ICD)
	int ch = (int) message.get();
	int sv = (int) message.get();
	//debug//printf("MID8 CH:%2d;SV:%2d\n\t", ch, sv);
	if (!(ch>=0 && ch<MAXCHANNELS)) {
		log->finest("MID8 channel not in range");
		return false;	//data not valid
	}
	//read ten words from the message. Bits in each 32 bits word are: D29 D30 d1 d2 ... d30
	//that is: two last parity bits from previous word followed by the 30 bits of the current word
	for (int i=0; i<10; i++) wd[i] = message.getUInt();
	//check parity of each subframe word
	bool parityOK = checkParity(wd[0]);
	for (int i=1; parityOK && i<10; i++) parityOK &= checkParity(wd[i]);
	//debug//printf(";P:%s\n\t", parityOK? "true" : "false");
	//debug//for (int i=0; i<10; i++) printf("%08X ", wd[i]);
	//if parity not OK, ignore all subframe data and return
	if (!parityOK) {
		log->finest("MID8 parity not OK");
		//debug//printf("\n");
		return false;
	}
	//remove parity from each GPS word getting the useful 24 bits
	//Note that when D30 is set, data bits are complemented (a non documented SiRF OSP feature)
	for (int i=0; i<10; i++)
		if ((wd[i] & 0x40000000) == 0) wd[i] = (wd[i]>>6) & 0xFFFFFF;
		else wd[i] = ~(wd[i]>>6) & 0xFFFFFF;
	//debug//printf("\n\t");
	//debug//for (int i=0; i<10; i++) printf("%06X   ", wd[i]);
	//get subframe and page identification (page identification valid only for subframes 4 & 5)
	unsigned int subfrmID = (wd[1]>>2) & 0x07;
	unsigned int pgID = (wd[2]>>16) & 0x3F;
	//debug//printf("\n\tFR:%2u;PG:%2u", subfrmID, pgID);
	//only have interest subframes: 1,2,3 & page 18 of subframe 4 (pgID = 56 in GPS ICD Table 20-V)
	if ((subfrmID>0 && subfrmID<4) || (subfrmID==4 && pgID==56)) {
		subfrmID--;		//convert it to its index
		//store satellite number and message words
		subfrmCh[ch][subfrmID].sv = sv;
		for (int i=0; i<10; i++) subfrmCh[ch][subfrmID].words[i] = wd[i];
		//check if all ephemerides have been already received
		if (allEphemReceived(ch)) {
			//if all 3 frames received , pack their data as per MID 15 (see SiRF ICD)
			for (int i=0; i<3; i++) {	//for each subframe index 0, 1, 2
				for (int j=0; j<5; j++) { //for each 2 WORDs group
					dt[i*15+j*3] = (subfrmCh[ch][i].words[j*2]>>8) & 0xFFFF;
					dt[i*15+j*3+1] = ((subfrmCh[ch][i].words[j*2] & 0xFF)<<8) | ((subfrmCh[ch][i].words[j*2+1]>>16) & 0xFF);
					dt[i*15+j*3+2] = subfrmCh[ch][i].words[j*2+1] & 0xFFFF;
				}
				//the exception is WORD1 (TLM word) of each subframe, whose data are not needed
				dt[i*15] = sv;
				dt[i*15+1] &= 0xFF;
			}
			//extract ephemeris data and store them into the RINEX instance
			extractEphemeris (rinex, dt);
			//TBW check if iono data exist & extract and store iono data in subfrmCh[ch][3]
			//debug//printf(";IONO:%b\n\t",subfrmCh[ch][3].sv != 0);
			//clear storage
			for (int i=0; i<3; i++) subfrmCh[ch][i].sv = 0;
		}
	}
	//debug//printf("\n");
	return true;
}
/**
 * getMID15NavData gets ephemeris data from a MID 15 message
 * 
 * @param rinex		the class instance where data are stored
 */
bool GNSSDataAcq::getMID15NavData(RinexData& rinex) {
	if (message.payloadLen() != 92) {
		log->info("MID15 msg len <> 92");
		return false;
	}
	unsigned int dt[45];		//to store the 3x15 data items in the message
	int svID = (int) message.get();
	for (int i=0; i<45; i++) dt[i] = (unsigned int) message.getUShort();
	//set HOW bits in dt[1] and dt[2] to 0 (MID15 does not provide data from HOW)
	dt[1] &= 0xFF00;
	dt[2] &= 0x0003;
	//extract ephemerides data and store them into the RINEX instance
	return extractEphemeris(rinex, dt);
}
/**
 * getMID19Masks gets receiver setting data (elevation and SNR masks) from a MID 19 message
 * 
 * @param rtko	the RTKobservation class instance where data are stored
 */
bool GNSSDataAcq::getMID19Masks(RTKobservation& rtko) {
	if (message.payloadLen() != 65) {
		log->info("MID19 msg len <> 65");
		return false;
	}
	double elevationMask;
	double snrMask;
	message.skipBytes(19);	//skip from SubID to DOPmask: 1U 2U 3*1U 2S 6*1U 4U 1U
	elevationMask = (double) message.getShort();
	snrMask = (double) message.get();
	rtko.setMasks(elevationMask/10.0, snrMask);
	return true;
}
/**
 * getMID28NavData gets measurements (time, pseudorange, carrier phase, etc.) for a satellite from a MID28 message
 * 
 * @param rinex	the RinexData class instance where data are to be stored
 * @param sameEpoch true when measurements added belongs to the current epoch, false otherwise
 * @return true if nav message and data are valid and have been added to RinexData, false otherwise
 */
bool GNSSDataAcq::getMID28NavData(RinexData& rinex, bool& sameEpoch) {
	if (message.payloadLen() != 56) {
		log->info("MID28 msg len <> 20");
		return false;
	}
	sameEpoch = false;
	char sys = 'G';
	//get data from message MID28
	int channel = message.get();	
	message.getInt();			//a time tag not used
	int satID = message.get();
	if (satID > 100) {			//it is a SBAS satellite
		sys = 'S';
		satID -= 100;
	}
	double gpsSWtime = message.getDouble();
	double pseudorange = message.getDouble();
	double carrierFrequency = (double) message.getFloat(); //signo - ¿?
	//carrier phase is given in meters; convert it to cycles
	double carrierPhase = message.getDouble() * L1WLINV;
	message.getUShort();		//the timeIntrack is not used
	int syncFlags = message.get();
	//get the signal strength as the worst of the C/N0 given
	int carrier2noise = 0;
	int strength = message.get();
	for (int i=1; i<10; i++)
		if ((carrier2noise = message.get()) < strength) strength = carrier2noise;
	//compute strengthIndex as per RINEX spec (5.7): min(max(strength / 6, 1), 9)
	int strengthIndex = strength / 6;
	if (strengthIndex < 1) strengthIndex = 1;
	if (strengthIndex > 9) strengthIndex = 9;
	//according SiRF ICD, if deltaRangeInterval == 0, dopplerFreq 
	//unsigned short int deltaRangeInterval = message.getUShort();
	//TBW Phase Error Count puede usarse para obtener LoL:
	//si PhEC != PhEC anterior de este satélite hay "slip" en carrierPhase
	//debug//System.out.format("MID28 CH:%2d;SV:%2d;SWt:%17.10f;%04X\n", channel, satID, gpsSWtime, syncFlags);
	if ((syncFlags & 0x01) != 0) {	//bit 0 set only when acquisition complete
		sameEpoch = rinex.addMeasurement(sys, satID, "S1C", (double) strength, 0, 0, gpsSWtime);
		rinex.addMeasurement(sys, satID, "C1C", pseudorange, 0, strengthIndex, gpsSWtime);
		//check syncFlags to see if carrier phase measurement is valid
		if ((syncFlags & 0x02) != 0) {
			rinex.addMeasurement(sys, satID, "L1C", carrierPhase, 0, strengthIndex, gpsSWtime);
		}
		//check deltaRangeInterval. If 0 carrierFrequency is the Doppler frequency
		//if (deltaRangeInterval == 0) {
			rinex.addMeasurement(sys, satID, "D1C", carrierFrequency  * L1WLINV, 0, 0, gpsSWtime);
		//}
		return true;
	}
	string error =  "MID28 data NOK. Ch:" + to_string((long long) channel);
	error += " Eph:" + to_string((long double) gpsSWtime) + " SV:";
	error.push_back(sys);
	error += to_string((long long) satID);
	error += " SynchFlag:" + to_string((long long) syncFlags);
	log->info(error);
	return false;
}
/**
 * checkParity checks the parity of a GPS message subframe word using procedure in GPS ICD
 * 
 * To check the parity, the six bits of parity are computes for the word contents,
 *  and then compared with the current parity in the 6 LSB of the word passed
 *  
 * @param d The subframe word passed, where the two LSB bits of the previous word have been added  
 * @return True if parity computed is equal to the current parity in the six LSB of the word
 */
bool GNSSDataAcq::checkParity (unsigned int d) {
	//d has de form: D29 D30 d1 .. d30)
	unsigned int toCheck = d;
	if ((d & 0x40000000) != 0) toCheck = (d & 0xC0000000) | (~d & 0x3FFFFFFF);
	//compute the parity of the bit stream
	unsigned int parity = 0;
	for (int i=0; i<6; i++) parity |= (bitsSet(parityBitMask[i] & toCheck) % 2) << (5-i);
	//debug//printf("%02X:%02X:%02X;", parity, d & 0x3FL, toCheck & 0x3FL);
	return parity == (d & 0x3F);
}
/**
 * Count the number of bits set in a given 32 bits stream
 * 
 * Used to compute the XOR of a bit stream for computing parity
 * 
 * @param lw The 32 bit stream to be processed (32 LSB in the long)
 * @return	The number of bits set to 1
 */
unsigned int GNSSDataAcq::bitsSet(unsigned int lw) {
	int count = 0;
	for (int i=0; i<32; i++) count += (int)((lw>>i) & 0x01);
	return count;
}
/**
 * allEphemReceived checks if all ephemerides in a given channel have been received
 * 
 * All ephemerides have been received if subframes 1, 2 and 3 have been received and their data
 * belong to the same IOD
 * 
 * @param ch The data channel to be checked
 * @return True, if all data received, false otherwise
 */
bool GNSSDataAcq::allEphemReceived(int ch) {
	bool allReceived =  (subfrmCh[ch][0].sv == subfrmCh[ch][1].sv)
						&& (subfrmCh[ch][0].sv == subfrmCh[ch][2].sv);
	//debug//printf("\n\tCh:%2d , svs:" ,ch);
	//debug//for(int i=0;i<3;i++) printf("%2d ", subfrmCh[ch][i].sv);
	//IODC (8LSB in subframe 1) must be equal to IODE in subframe 2 and IODE in subframe 3
	unsigned int iodcLSB = (subfrmCh[ch][0].words[7]>>16) & 0xFF;
	//debug//printf(";IODClsb:%3d",iodcLSB);
	allReceived &= iodcLSB == ((subfrmCh[ch][1].words[2]>>16) & 0xFF);
	//debug//printf(";IODE2:%3d",(subfrmCh[ch][1].words[2]>>16) & 0xFF);
	allReceived &= iodcLSB == ((subfrmCh[ch][2].words[9]>>16) & 0xFF);
	//debug//printf(";IODE3:%3d",(subfrmCh[ch][2].words[9]>>16) & 0xFF);
	return allReceived;
}
/**
 * extractEphemeris as transmitted by satellites from a dt array of 45 items (OSP format) and store them in a rinex object
 * 
 * The dt array is an arrangement to compact the 3 x 10 x 24 bits GPS data words (without parity) into 3 x 15 x 16 bits words
 * See SiRF ICD (A.5 Message # 15: Ephemeris Data) for details.
 * 
 * @param rinex		the class instance where data are stored
 * @param dt		the 3 x 15 items containing data
 */
bool GNSSDataAcq::extractEphemeris (RinexData& rinex, unsigned int * dt) {
	char errorBuff[80];
	unsigned int sv = dt[0] & 0xFF;		//the SV identification
	//check for consistency in the channel data
	if (!((sv==(dt[15] & 0xFF)) && (sv==(dt[30] & 0xFF)))) {
		log->info("Different SVs in the channel data");
		return false;
	}
	//check for version data consistency
	unsigned int iodcLSB = dt[10] & 0xFF;
	unsigned int iode1 = (dt[15+3]>>8) & 0xFF;
	unsigned int iode2 = dt[30+13] & 0xFF;
	if (!(iode1==iode2 && iode1==iodcLSB)) {
		sprintf(errorBuff, "Different IODs:SV <%2u> IODs <%3u,%3u,%3u>", sv, iodcLSB, iode1, iode2);
		log->info(string(errorBuff));
		return false;
	}
	/* dudas sobre este codigo
	//check for SVhealth
	unsigned int svHealth = (dt[4]>>10) & 0x3F;
	if ((svHealth & 0x20) != 0) {
		sprintf(errorBuff, "Wrong SV %2u health <%02hx>", sv, svHealth);
		log->info(string(errorBuff));
		return false;
	}
	*/
	//storage for 8 lines (arranged as per RINEX GPS nav files) of broadcast orbit data,
	//but containing bit stream values as transmitted by satellites, w/o applying scale factors
	unsigned int bo[8][4];
	//fill bo extracting data according SiRF ICD (see A.5 Message # 15: Ephemeris Data) 
	//broadcast line 0
	bo[0][0] = dt[11];	//T0C
	bo[0][1] = getTwosComplement(((dt[13] & 0x00FF)<<14) | ((dt[14]>>2) & 0x3FFF), 22);	//Af0
	bo[0][2] = getTwosComplement(((dt[12] & 0x00FF)<<8) | ((dt[13]>>8) & 0x00FF), 16);	//Af1
	bo[0][3] = getTwosComplement((dt[12]>>8) & 0x00FF, 8);								//Af2
	//broadcast line 1
	bo[1][0] = iode1;	//IODE
	bo[1][1] = getTwosComplement(((dt[15+3] & 0x00FF)<<8) | ((dt[15+4]>>8) & 0x00FF), 16);	//Crs
	bo[1][2] = getTwosComplement(((dt[15+4] & 0x00FF)<<8) | ((dt[15+5]>>8) & 0x00FF), 16);	//Delta n
	bo[1][3] = getTwosComplement(((dt[15+5] & 0x00FF)<<24) | ((dt[15+6] & 0xFFFF)<<8) | ((dt[15+7]>>8) & 0x00FF),32);//M0
	//broadcast line 2
	bo[2][0] = getTwosComplement(((dt[15+7] & 0x00FF)<<8) | ((dt[15+8]>>8) & 0x00FF), 16);			//Cuc															//Cuc
	bo[2][1] = (((dt[15+8] & 0x00FF)<<24) | ((dt[15+9] & 0xFFFF)<<8) | ((dt[15+10]>>8) & 0x00FF));	//e
	bo[2][2] = getTwosComplement(((dt[15+10] & 0x00FF)<<8) | ((dt[15+11]>>8) & 0x00FF), 16);		//Cus
	bo[2][3] = ((dt[15+11] & 0x00FF)<<24) | ((dt[15+12] & 0xFFFF)<<8) | ((dt[15+13]>>8) & 0x00FF);	//sqrt(A)
	//broadcast line 3
	bo[3][0] = ((dt[15+13] & 0x00FF)<<8) | ((dt[15+14]>>8) & 0x00FF);	//Toe
	bo[3][1] = getTwosComplement(dt[30+3], 16);						//Cic
	bo[3][2] = getTwosComplement(((dt[30+4] & 0xFFFF)<<16) | (dt[30+5] & 0xFFFF), 32);	//OMEGA
	bo[3][3] = getTwosComplement(dt[30+6], 16);						//CIS
	//broadcast line 4
	bo[4][0] = getTwosComplement(((dt[30+7] & 0xFFFF)<<16) | (dt[30+8] & 0xFFFF), 32);	//i0
	bo[4][1] = getTwosComplement(dt[30+9], 16);											//Crc
	bo[4][2] = getTwosComplement(((dt[30+10] & 0xFFFF)<<16) | (dt[30+11] & 0xFFFF), 32);	//w (omega)
	bo[4][3] = getTwosComplement(((dt[30+12] & 0xFFFF)<<8) | ((dt[30+13]>>8) & 0x00FF), 24);//w dot
	//broadcast line 5
	bo[5][0] = getTwosComplement((dt[30+14]>>2) & 0x03FFF, 14);	//IDOT
	bo[5][1] = (dt[3]>>4) & 0x0003;				//Codes on L2
	bo[5][2] = ((dt[3]>>6) & 0x03FF) + 1024L;	//GPS week#
	bo[5][3] = (dt[4]>>7) & 0x0001;				//L2P data flag
	//broadcast line 6
	bo[6][0] = dt[3] & 0x000F;			//SV accuracy
	bo[6][1] = (dt[4]>>10) & 0x003F;	//SV health
	bo[6][2] = getTwosComplement((dt[10]>>8) & 0x00FF, 8);//TGD
	bo[6][3] = iodcLSB | (dt[4] & 0x0300);		//IODC
	//broadcast line 7
	bo[7][0] = (((dt[1] & 0x00FF)<<9) | ((dt[2]>>7) & 0x01FF)) * 600;	//the 17 MSB of the Zcount in HOW (W2) converted to sec and scaled by 100
	if (bo[7][0] == 0)	//data came from MID15 (no HOW data), then put current GPS seconds on it
		bo[7][0] = rinex.getGPSTime() * 100.0;	//transmission time in sec scaled by 100
	bo[7][1] = (dt[15+14]>>7) & 0x0001;			//Fit flag
	bo[7][2] = 0;		//Spare. Not used
	bo[7][3] = iode2;	//Spare. Used for temporary store of IODE in subframe 3
	rinex.addGPSNavData(sv, bo);
	return true;
}
/**
 * Convert a two's complement representation from a given number of bits to 32 bits
 * 
 * Number is an integer containing a bit stream of nbits length which represents a nbits integer in two's complement, that is:
 * - if number is in the range 0 to 2^(nbits-1) is positive. Its value in a 32 bits representation is the same.
 * - if number is in the range 2^(nbits-1) to 2^(nbits)-1, is negative and must be converted subtracting 2^(nbits)
 * 
 * @param number	The 32 bits pattern with the number to be interpreted
 * @param nbits		The number of significative bits in the pattern
 * @return			The value of the number (its 32 bits representation)
 */
unsigned int GNSSDataAcq::getTwosComplement(unsigned int number, unsigned int nbits) {
	if (nbits >= 32) return number;					//the conversion is imposible or not necessary
	if (number < ((unsigned int) 1 << (nbits-1))) return number;	//the number is posive, it do not need conversion
	return number - (1 << nbits);
}
