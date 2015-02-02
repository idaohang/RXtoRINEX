/** @file Logger.cpp
 * contains the implementation of the Logger class
 */

#include "Logger.h"

/**Constructs an empty Logger object.
 *It sets the default log level to INFO, and states the stderr as log file.
 */
Logger::Logger(void) {
	levelSet = INFO;
	fileLog = stderr;
}

/**Constructs a Logger object.
 *It sets the default log level to INFO, and opens log file having fileName for appending
 *log messages to its current content. If the file does not exist, it is created.
 *
 *@param fileName the name of the log file
 */
Logger::Logger(string fileName) {
	levelSet = INFO;
	fileLog = fopen(fileName.c_str(), "a");
	if (fileLog == NULL) fileLog = stderr;
}

/**Destruct the Logger object after closing its log file. 
 */
Logger::~Logger(void) {
	if (fileLog != stderr) fclose(fileLog);
}

/**setPrgName sets the program name to be used in message tagging
 *
 *@param prg the name of the program
 */
void Logger::setPrgName(string prg) {
	program = prg;
}

/**setLevel states the current log level to be taken into account when logging messages.
 *Log level can be SEVERE, WARNING, INFO, CONFIG, FINE, FINER or FINEST.
 *Only meesages having log level from SEVERE to setLevel will be actually recorded.
 *
 *@param level the log level to set
 */
void Logger::setLevel(logLevel level) {
	levelSet = level;
}

/**logMsg is an internal method to tag, format, and log messages data passed by log level methods.
 *
 *@param logLevel states the level to tag the message
 *@param message contains its description
 */
void Logger::logMsg (logLevel msgLevel, string msg) {
	time_t rawtime;
	struct tm * timeinfo;
	char txtBuf[80];

	time (&rawtime);
	timeinfo = localtime (&rawtime);
	strftime(txtBuf, sizeof txtBuf, " %d/%m/%y %H:%M ", timeinfo);
	fprintf(fileLog, "%s %s ", program.c_str(), txtBuf);
	switch (msgLevel) {
	case SEVERE: fprintf(fileLog, "(SVR) "); break;
	case WARNING: fprintf(fileLog, "(WRN) "); break;
	case INFO: fprintf(fileLog, "(INF) "); break;
	case CONFIG: fprintf(fileLog, "(CFG) "); break;
	case FINE: 
	case FINER:
	case FINEST: fprintf(fileLog, "(FNE) "); break;
	}
	fprintf(fileLog, "%s\n", msg.c_str());
	fflush(fileLog);
}

/**severe logs a message at SEVERE level.
 * All SEVERE messages are appended to the log file.
 *
 *@param toLog the message description to log
 */
void Logger::severe (string toLog) {
	logMsg(SEVERE, toLog);
}
/**warning logs a message at WARNING level.
 * The message is appended to the log file if the current level is in the range SEVERE to WARNING
 *
 *@param toLog the message description to log
 */
void Logger::warning (string toLog) {
	if (levelSet < WARNING) return;
	logMsg(WARNING, toLog);
}

/** info logs message at INFO level.
 * The message is appended to the log file if the current level is in the range SEVERE to INFO
 *
 *@param toLog the message description to log
 */
void Logger::info (string toLog) {
	if (levelSet < INFO) return;
	logMsg(INFO, toLog);
}

/**config logs the message at CONFIG level.
 * The message is appended to the log file if the current level is in the range SEVERE to CONFIG
 *
 *@param toLog the message description to log
 */
void Logger::config (string toLog) {
	if (levelSet < CONFIG) return;
	logMsg(CONFIG, toLog);
}

/**fine logs the message at FINE level.
 * The message is appended to the log file if the current level is in the range SEVERE to FINE
 *
 *@param toLog the message description to log
 */
void Logger::fine (string toLog) {
	if (levelSet < FINE) return;
	logMsg(FINE, toLog);
}

/**finer logs the message at FINER level.
 * The message is appended to the log file if the current level is in the range SEVERE to FINER
 *
 *@param toLog the message description to log
 */
void Logger::finer (string toLog) {
	if (levelSet < FINER) return;
	logMsg(FINER, toLog);
}

/**finest log the message at FINEST level.
 * The message is appended to the log file if the current level is in the range SEVERE to FINEST
 *
 *@param toLog the message description to log
 */
void Logger::finest (string toLog) {
	if (levelSet < FINEST) return;
	logMsg(FINEST, toLog);
}
