/**  @file GNSSDataAcq.h
 * Contains the GNSSDataAcq class definition.
 * Using the methods provided, data acquired are stored in RinexData objects (header data, observations data, etc.)
 * or RTKobservation data objects (header, measurements, etc.) to allow further printing using RINEX or RTK file formats. 
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

//from CommonClasses
#include "Logger.h"
#include "OSPMessage.h"
#include "RinexData.h"
#include "RTKobservation.h"

//Receiver and GPS specific data
/// The maximum number of channels in the receiver
#define MAXCHANNELS 12
///The maximum number of subframes in the nav message
#define MAXSUBFR 4

//@cond DUMMY
//other constants needed here derived from ones defined in RinexData.h
//a bit mask definition for the bits participating in the computation of parity (see GPS ICD)
//bit mask order: D29 D30 d1 d2 d3 ... d24 ... d29 d30
//parityBitMask[i] identifies bits participating (set to 1) or not (set to 0) in the computation of parity bit i.
const unsigned int parityBitMask[] = {0xBB1F3480, 0x5D8F9A40, 0xAEC7CD00, 0x5763E680, 0x6BB1F340, 0x8B7A89C0};

//A type to store 50bps message data
struct SubframeData {
	int sv;					//the satelite number
	unsigned int words[10];	//the ten words with nav data
};
//@endcond

/**GNSSDataAcq class defines data and methods used to acquire header and epoch data from a binary file containing receiver messages.
 * Header and epoch data can be used to generate and print RINEX or RTK files.
 *<p>
 * A program using GNSSDataAcq would perform the following steps:
 *	-# Declare a GNSSDataAcq object stating the receiver, the file name with the binary messages containing the data
 *		to be acquired, and the logger to be used
 *	-# Acquire header data to be placed in the header of the output file (RINEX or RTK)
 *	-# As header data may be sparse among the binary file, rewind it before performing any other data acquisition
 *	-# Iterate epoch by epoch acquiring its data until end of file reached
 *<p>
 * This version implements acquisition from binary files containing OSP messages collected from SiRFIV receivers.
 * Each OSP message starts with the payload length (2 bytes) and follows the n bytes of the message payload.
 *<p>
 *A detailed definition of OSP messages can be found in the document "SiRFstarIV™ One Socket Protocol 
 * Interface Control Document Issue 9" from CSR Inc.
 *<p>
 * Note that messages recorded in the OSP binary file have the structure described in this ICD for the receiver messages,
 * except that the start sequence (A0 A3), the checksum, and the end sequence (B0 B3) have been removed.
 */
class GNSSDataAcq {
	string receiver;
	int minSVSfix;
	FILE* ospFile;
	Logger* log;
	OSPMessage message;
	struct SubframeData subfrmCh[MAXCHANNELS][MAXSUBFR];

	bool checkParity (unsigned int );
	unsigned int bitsSet(unsigned int );
	bool allEphemReceived(int );
	bool extractEphemeris (RinexData&, unsigned int* );
	unsigned int getTwosComplement(unsigned int, unsigned int );

	bool getMID2PosData(RinexData& );
	bool getMID2PosData(RTKobservation& );
	bool getMID6RxData(RinexData& );
	bool getMID7TimeData(RinexData& );
	bool getMID7Interval(RinexData& );
	bool getMID8NavData(RinexData& );
	bool getMID15NavData(RinexData& );
	bool getMID19Masks(RTKobservation& );
	bool getMID28NavData(RinexData&, bool&);

public:
	GNSSDataAcq(string, int, FILE*, Logger*);
	~GNSSDataAcq(void);
	bool acqHeaderData(RinexData& );
	bool acqHeaderData(RTKobservation& );
	bool acqEpochData(RinexData& , bool, bool);
	bool acqEpochData(RTKobservation& );
};

