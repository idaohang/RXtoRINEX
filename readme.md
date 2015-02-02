##Introduction

RXtoRINEX project is aimed to provide tools:
 - To collect data from GPS / GNSS receivers embedded in mobile devices or connected to them through a serial port.
 - To analyze or print data collected
 - To convert data collected to RINEX or RTK formats

RINEX is the standard format used to feed with observation and navigation data files software packages for computing positioning solutions with high accuracy.

RINEX data files are not aimed to compute solutions in real time, but to perform post-processing, that is, GNSS/GPS receiver data are first collected into files using the receiver specific format, then converted to standard RINEX data files, and finally processed using, may be, facilities having “number-crunching” capabilities, and additional data (like data from reference stations available in UNAVCO, EUREF, GDC, IGN, and other data repositories) to allow removal of receiver data errors.

A detailed definition of the RINEX format can be found in the document "RINEX: The Receiver Independent Exchange Format Version 2.10" from Werner Gurtner; Astronomical Institute; University of Berne. An updated document exists also for Version 3.00.

RTK refers here to the data format defined for the RTKLIB (http://www.rtklib.com/) that would allow analysis of data collected with tools in this project using the RTKLIB.


##Current Implementation

The current implementation has the following scope:
- Binaries provided are for win32 environment.
- GPS receivers shall be SiRF IV(TM) (for L1 signals) based.

Future versions to include support for other receivers would depend on availability of information from manufacturers.

Any information on ICDs for other receivers (like SiRFIV-t, Broadcom, etc) is welcome.

Future releases would provide versions for Linux and Android.


##Data Files

The data tools provided are used to process or generate the following file types.


###RINEX observation

A RINEX observation file is a text file containing a header with data related to the data acquisition, and sequences of epoch data. For each epoch the observables for each satellite tracked are printed. See above reference on RINEX for a detailed description of this file format.


###RINEX GPS navigation

A RINEX GPS navigation file is a text file containing a header with data related to the data acquisition, and the satellite ephemeris obtained from the navigation message in the GPS signal. See above reference on RINEX for a detailed description of this file format.


###OSP binary

OSP binary are files containing OSP message data obtained from embedded SiRFIV-E(TM) based GPS receivers, or connected to the computer or device through a serial port.

The file contains a sequence of message data, where for each message they are stored the two bytes with the payload length and the n bytes of the payload.

A detailed definition of OSP messages can be found in the document "SiRFstarIV(TM) One Socket Protocol Interface Control Document Issue 9" from CSR Inc.

Note that some unnecessary data for processing contained in the full OSP message are removed when written to OSP files.


###GP2 debug

GP2 debug are files containing receiver specific measurement data obtained in Android Smartphones with embedded SiRFIV-T(TM) GPS receivers.

They are text files containing in each line a time tag followed by an OSP message with byte contents written in hexadecimal.

Each line in the GP2 file has format as per the following example:

29/10/2014 20:31:08.942 (0) A0 A2 00 12 33 06 00 00 00 00 00 00 00 19 00 00 00 00 00 00 64 E1 01 97 B0 B3

Where:
       - Time tag: 29/10/2014 20:31:08.942
       - Unknown: (0) 
       - Head: A0 A2
       - Payload length: 00 12
       - Payload: 33 06 00 00 00 00 00 00 00 19 00 00 00 00 00 00 64 E1
       - Checksum: 01 97
       - Tail: B0 B3

Note that in above format, data from "head" to "tail" is an OSP messages with values written in hexadecimal.


##Data Tools

###RXtoOSP command

This command line program can be used to capture OSP message data from a SiRF IV receiver connected to the computer / device serial port and to store them in an OSP binary file.

Data acquisition can be controlled using options to:
 - Show usage data
 - Set log level (SEVERE, WARNING, INFO, CONFIG, FINE, FINER, FINEST)
 - State the serial port name where receiver is connected
 - Set the serial port baud rate
 - State the name of the OSP binary output file
 - Set duration of the acquisition period
 - Capture GPS ephemeris data from MID15 messages or not
 - Capture GPS 50bps nav message (MID8) or not
 - Set the observation interval (in seconds) for epoch data
 - Stop epoch data acquisition when a message with given MID arrives

Note: use of this command requires a GPS receiver state compatible with data being requested: transmitting serial data at the bit rate, length and parity expected, and in OSP format. See SynchroRX command below for details.


###GP2toOSP command

This command line program is used to translate data captured in GP2 debug files to OSP message files.

Data translation can be controlled using options to:
 - Show usage data and stops
 - Set log level (SEVERE, WARNING, INFO, CONFIG, FINE, FINER, FINEST)
 - State the GP2 input file name
 - State the time interval for extracting lines in the GP2 file
 - Set the OSP binary output file name
 - State the list of wanted messages MIDs


###OSPtoTXT

This command line program is used to print contents of OSP files into readable format. Printed data are sent to the stdout.

The output contains descriptive relevant data from each OSP message:
 - Message identification (MID, in decimal) and payload length for all messages
 - Payload parameter values for relevant messages used to generate RINEX or RTK files
 - Payload bytes in hexadecimal, for MID 255


###OSPtoRINEX

This command line program is used to generate RINEX files from an OSP data file containing SiRF IV receiver message data.

The generation of RINEX files can be controlled using options to:
 - Show usage data and stops
 - Set log level (SEVERE, WARNING, INFO, CONFIG, FINE, FINER, FINEST)
 - Set RINEX file name prefix
 - Set RINEX version to generate (V210, V300)
 - State the specific data to be included in the RINEX file header, like receiver marker name, observer name, agency name, who run the RINEX file generator, receiver antenna type, antenna number.
 - Set code measurements to include (like C1C,L1C, etc.)
 - Set minimum satellites needed in a fix to include its observations
 - Set if clock bias will be applied to measurements and time, or not
 - Set if end-of-file comment lines will be appended or not to RINEX observation file
 - Generate or not RINEX GPS navigation file, and which data has to be used to generate it: MID8 messages with 50bps data, or MID15 with receiver collected ephemeris


###OSPtoRTK

This command line program is used to generate a RTK file with positioning data extracted from an OSP data file containing SiRF IV receiver messages.

The generation of RTK files can be controlled using options to:
 - Show usage data and stops
 - Set the log level (SEVERE, WARNING, INFO, CONFIG, FINE, FINER, FINEST)
 - Set the minimum number of satellites in a fix to include its positioning data


###SynchroRX

This command line can be used to synchronize receiver and computer to allow communication between both.

The receiver state is defined by the following:
 - The serial port bit rate, stop bits, and parity being used by receiver
 - The protocol mode being used by receiver to send data: NMEA or OSP
 - If data is flowing from the receiver or not.

The computer data acquisition process state is defined by:
 - The port (port name) used for the communication with the GPS receiver
 - The serial port bit rate, stop bits, and parity being used
 - The type of data required: NMEA or OSP

To allow communication between GPS receiver and computer, states of both elements shall be synchronized according to the needs stated.

Synchronization requires first to know the current state of receiver and computer port, and then change both to the new state. But, to know the receiver state is necessary to communicate with it, setting the computer port at the same speed, stop bits and parity that the receiver. This will allow receiving and analyzing the data being generated by the receiver to know the protocol it is using. As receiver state may be not known initially, it would be necessary to scan different communication alternatives to know it.

To allow synchronization it is assumed the following for the receiver:
 - It is providing data continuously: ASCII NMEA or binary OSP messages, depending on the mode.
 - It is receiving/sending data at 1200, 2400, 4800, 9600, 38400, 57600 or 115200 bps, with one stop bit and parity set to none.

After synchronization, bit rates will be set to 9600 bps when exchanging NMEA data or to 57600 bps when exchanging OSP messages.

The synchronization process can be controlled using options to:
 - Show usage data and stops
 - Set the log level (SEVERE, WARNING, INFO, CONFIG, FINE, FINER, FINEST)
 - Set the serial port name where receiver is connected
 - Set the receiver protocol to NMEA or OSP


##Data files

The directory ./Data contains sample files obtained with the above described tools.

###GStarIV

Contains GPS data acquired with an external GlobalSat G-Star IV model BU-353S4 (SiRFIV-e receiver). Each subdirectory inside includes data files from a survey:
 - The OSP file acquired with RXtoOSP in a given point.
 - A readable TXT file generated with OSPtoTXT
 - The RINEX files generated with OSPtoRINEX
 - a RTK file generated with OSPtoRTK
 - LogFile.txt from executions of above commands

In addition there are files with analysis data from teqc and from PRSolve (part of the GPS ToolKit).

###SamsungGlxyS2

Contains GPS data acquired with a Samsung Galaxy S2 (GT-I9100) from its  embedded receiver (a SiRFIV-t). Each subdirectory inside includes data for different surveys, including:
 - The gp2 file downloaded from the smartphone
 - An OSP file generated with GP2toOSP
 - Readable TXT file generated with OSPtoTXT
 - RINEX files generated with OSPtoRINEX
 - RTK files generated with OSPtoRTK
 - LogFile.txt from executions of above commands

