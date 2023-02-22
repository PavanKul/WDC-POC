// C++ Header File(s)
#include <iostream>
#include <cstdlib>
#include <ctime>

// Code Specific Header Files(s)
#include "chord/chord.h"
#include "chord/Logger.h"

#define FILE_LINE_SIZE 300

using namespace std;
using namespace Chord;

Logger* Logger::m_Instance = 0;
//std::ofstream Logger::m_File;
FILE* Logger::m_File = NULL;

// Log file name. File name should be change from here only
std::string logFileName = "";

Logger::Logger()
{
	try
	{
//	readConfig();
//	m_File.open("/home/msys/logfile.txt", ios::out|ios::app);
	m_File = fopen("/home/msys/logfile.txt", "a+");
	m_LogLevel	= LOG_LEVEL_TRACE;
	m_LogType	= FILE_LOG;

	// Initialize mutex
#ifdef WIN32
	InitializeCriticalSection(&m_Mutex);
#else
	int ret=0;
//	ret = pthread_mutexattr_settype(&m_Attr, PTHREAD_MUTEX_ERRORCHECK_NP);
	if(ret != 0)
	{
		printf("Logger::Logger() -- Mutex attribute not initialize!!\n");
		exit(0);
	}
//	ret = pthread_mutex_init(&m_Mutex,&m_Attr);
	if(ret != 0)
	{
		printf("Logger::Logger() -- Mutex not initialize!! :: %d\n", ret);
		exit(0);
	}
#endif
	}
	catch(...)
	{
		printf("Exception...\n");
	}
}

Logger::~Logger()
{
//	m_File.close();
	if (m_File)
	{
		fclose(m_File);
		m_File = NULL;
	}
#ifdef WIN32
	DeleteCriticalSection(&m_Mutex);
#else
	pthread_mutexattr_destroy(&m_Attr);
	pthread_mutex_destroy(&m_Mutex);
#endif
}

Logger* Logger::getInstance() throw ()
{
	if (m_Instance == 0)
	{
		m_Instance = new Logger();
	}

	return m_Instance;
}

void Logger::lock()
{
#ifdef WIN32
	EnterCriticalSection(&m_Mutex);
#else
//	pthread_mutex_lock(&m_Mutex);
#endif
}

void Logger::unlock()
{
#ifdef WIN32
	LeaveCriticalSection(&m_Mutex);
#else
//	pthread_mutex_unlock(&m_Mutex);
#endif
}

void Logger::logIntoFile(const std::string& data)
{
	lock();
//	m_File << getCurrentTime() << "  " << data << endl;
	char logData[FILE_LINE_SIZE] = {'\0'};
	sprintf(logData, "%s::%s\n", getCurrentTime().c_str(), data.c_str());
	//cout << "Priyashree :: " << logData << endl;
	fprintf(m_File, "%s", logData);
	unlock();
}

void Logger::logOnConsole(const std::string& data)
{
	cout << getCurrentTime() << "  " << data << endl;
}

string Logger::getCurrentTime()
{
	string currTime;
	//Current date/time based on current time
	time_t now = time(0);
	// Convert current time to string
	currTime.assign(ctime(&now));

	// Last charactor of currentTime is "\n", so remove it
	string currentTime = currTime.substr(0, currTime.size()-1);
	return currentTime;
}

void Logger :: readConfig()
{
	try
	{
		std::ifstream infile("/home/msys/LogConfig");
		std::getline(infile, logFileName);
		infile.close();
	}
	catch(const ifstream::failure& e)
	{
		cout << "Invalid Filename" << endl;
	}
	catch(...)
	{
		cout << "Catch all Exception" << endl;
	}
}

void Logger :: chord_print(LogLevel level, const std::string& text, LogType type)
{
	string data = "";

	switch(level)
	{
		case LOG_LEVEL_INFO:
			data.append("[INFO]: ");
			break;
		case LOG_LEVEL_BUFFER:
			data.append("[BUFFER]: ");
			break;
		case LOG_LEVEL_TRACE:
			data.append("[TRACE]: ");
			break;
		case LOG_LEVEL_DEBUG:
			data.append("[DEBUG]: ");
			break;
		case LOG_LEVEL_ERROR:
			data.append("[ERROR]: ");
			break;
		default:
			break;
	}

	data.append(text);

	if(m_LogType == FILE_LOG)
	{
		logIntoFile(data);
	}
	else if(m_LogType == CONSOLE)
	{
		logOnConsole(data);
	}
}

// Interface for Error Log
void Logger::chord_error(const char* text) throw()
{
	string data;

	if ((text == NULL) && (text[0] == '\0'))
		printf("Text is empty\n");

	data.append("[ERROR]: ");
	data.append(text);


	// ERROR must be capture
	if(m_LogType == FILE_LOG)
	{
		logIntoFile(data);
	}
	else if(m_LogType == CONSOLE)
	{
		logOnConsole(data);
	}
}

void Logger::chord_error(std::string& text) throw()
{
	chord_error(text.data());
}

void Logger::chord_error(std::ostringstream& stream) throw()
{
	string text = stream.str();
	chord_error(text.data());
}

// Interface for Alarm Log
void Logger::chord_alarm(const char* text) throw()
{
	string data;

	if ((text == NULL) && (text[0] == '\0'))
		printf("Text is empty\n");

	data.append("[ALARM]: ");
	data.append(text);

	// ALARM must be capture
	if(m_LogType == FILE_LOG)
	{
		logIntoFile(data);
	}
	else if(m_LogType == CONSOLE)
	{
		logOnConsole(data);
	}
}

void Logger::chord_alarm(std::string& text) throw()
{
	chord_alarm(text.data());
}

void Logger::chord_alarm(std::ostringstream& stream) throw()
{
	string text = stream.str();
	chord_alarm(text.data());
}

// Interface for Always Log
void Logger::chord_always(const char* text) throw()
{
	string data;

	if ((text == NULL) && (text[0] == '\0'))
		printf("Text is empty\n");

	data.append("[ALWAYS]: ");
	data.append(text);

	// No check for ALWAYS logs
	if(m_LogType == FILE_LOG)
	{
		logIntoFile(data);
	}
	else if(m_LogType == CONSOLE)
	{
		logOnConsole(data);
	}
}

void Logger::chord_always(std::string& text) throw()
{
	chord_always(text.data());
}

void Logger::chord_always(std::ostringstream& stream) throw()
{
	string text = stream.str();
	chord_always(text.data());
}

// Interface for Buffer Log
void Logger::chord_buffer(const char* text) throw()
{
	// Buffer is the special case. So don't add log level
	// and timestamp in the buffer message. Just log the raw bytes.
	if((m_LogType == FILE_LOG) && (m_LogLevel >= LOG_LEVEL_BUFFER))
	{
		lock();
//		m_File << text << endl;
		fprintf(m_File, "%s", text);
		unlock();
	}
	else if((m_LogType == CONSOLE) && (m_LogLevel >= LOG_LEVEL_BUFFER))
	{
		cout << text << endl;
	}
}

void Logger::chord_buffer(std::string& text) throw()
{
	chord_buffer(text.data());
}

void Logger::chord_buffer(std::ostringstream& stream) throw()
{
	string text = stream.str();
	chord_buffer(text.data());
}

// Interface for Info Log
void Logger::chord_info(const char* text) throw()
{
	string data;

	if ((text == NULL) && (text[0] == '\0'))
		printf("Text is empty\n");

	data.append("[INFO]: ");
	data.append(text);

	if((m_LogType == FILE_LOG) && (m_LogLevel >= LOG_LEVEL_INFO))
	{
		logIntoFile(data);
	}
	else if((m_LogType == CONSOLE) && (m_LogLevel >= LOG_LEVEL_INFO))
	{
		logOnConsole(data);
	}
}

void Logger::chord_info(std::string& text) throw()
{
	chord_info(text.data());
}

void Logger::chord_info(std::ostringstream& stream) throw()
{
	string text = stream.str();
	chord_info(text.data());
}

// Interface for Trace Log
void Logger::chord_trace(const char* text) throw()
{
	string data;

	if ((text == NULL) && (text[0] == '\0'))
		printf("Text is empty\n");

	data.append("[TRACE]: ");
	data.append(text);

	if((m_LogType == FILE_LOG) && (m_LogLevel >= LOG_LEVEL_TRACE))
	{
		logIntoFile(data);
	}
	else if((m_LogType == CONSOLE) && (m_LogLevel >= LOG_LEVEL_TRACE))
	{
		logOnConsole(data);
	}
}

void Logger::chord_trace(std::string& text) throw()
{
	chord_trace(text.data());
}

void Logger::chord_trace(std::ostringstream& stream) throw()
{
	string text = stream.str();
	chord_trace(text.data());
}

// Interface for Debug Log
void Logger::chord_debug(const char* text) throw()
{
	string data;

	if ((text == NULL) && (text[0] == '\0'))
		printf("Text is empty\n");

	data.append("[DEBUG]: ");
	data.append(text);

	if((m_LogType == FILE_LOG) && (m_LogLevel >= LOG_LEVEL_DEBUG))
	{
		logIntoFile(data);
	}
	else if((m_LogType == CONSOLE) && (m_LogLevel >= LOG_LEVEL_DEBUG))
	{
		logOnConsole(data);
	}
}

void Logger::chord_debug(std::string& text) throw()
{
	chord_debug(text.data());
}

void Logger::chord_debug(std::ostringstream& stream) throw()
{
	string text = stream.str();
	chord_debug(text.data());
}

// Interfaces to control log levels
void Logger::updateLogLevel(const LogLevel logLevel)
{
	m_LogLevel = logLevel;
}

// Enable all log levels
void Logger::enaleLog()
{
	m_LogLevel = ENABLE_LOG;
}

// Disable all log levels, except error and alarm
void Logger:: disableLog()
{
	m_LogLevel = DISABLE_LOG;
}

// Interfaces to control log Types
void Logger::updateLogType(const LogType logType)
{
	m_LogType = logType;
}

void Logger::enableConsoleLogging()
{
	m_LogType = CONSOLE;
}

void Logger::enableFileLogging()
{
	m_LogType = FILE_LOG ;
}

