#ifndef _LOGGER_H_
#define _LOGGER_H_

// C++ Header File(s)
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "chord_fwd.h"
#include "local_node.h"

#ifdef WIN32
// Win Socket Header File(s)
#include <Windows.h>
#include <process.h>
#else
// POSIX Socket Header File(s)
#include <errno.h>
#include <pthread.h>
#endif

namespace Chord
{
   	// Direct Interface for logging into log file or console using MACRO(s)
   	#define LOG_ERROR(x)    Logger::getInstance()->error(x)
   	#define LOG_ALARM(x)	Logger::getInstance()->alarm(x)
   	#define LOG_ALWAYS(x)   Logger::getInstance()->always(x)
   	#define LOG_INFO(x)     Logger::getInstance()->info(x)
   	#define LOG_BUFFER(x)   Logger::getInstance()->buffer(x)
   	#define LOG_TRACE(x)    Logger::getInstance()->trace(x)
   	#define LOG_DEBUG(x)    Logger::getInstance()->debug(x)

// enum for LOG_LEVEL
typedef enum LOG_LEVEL
{
	DISABLE_LOG       = 1,
      	LOG_LEVEL_INFO	  = 2,
      	LOG_LEVEL_BUFFER  = 3,
      	LOG_LEVEL_TRACE   = 4,
      	LOG_LEVEL_DEBUG   = 5,
      	LOG_LEVEL_ERROR   = 6,
      	ENABLE_LOG        = 7,
   	}LogLevel;

// enum for LOG_TYPE
typedef enum LOG_TYPE
{
      	NO_LOG            = 1,
      	CONSOLE           = 2,
      	FILE_LOG          = 3,
   	}LogType;

class Logger
{
      	public:
        static Logger* getInstance() throw ();

        void chord_print(const LogLevel level, const std::string& text, LogType type = FILE_LOG) throw();

         // Interface for Error Log 
         void chord_error(const char* text) throw();
         void chord_error(std::string& text) throw();
         void chord_error(std::ostringstream& stream) throw();

         // Interface for Alarm Log 
         void chord_alarm(const char* text) throw();
         void chord_alarm(std::string& text) throw();
         void chord_alarm(std::ostringstream& stream) throw();

         // Interface for Always Log 
         void chord_always(const char* text) throw();
         void chord_always(std::string& text) throw();
         void chord_always(std::ostringstream& stream) throw();

         // Interface for Buffer Log 
         void chord_buffer(const char* text) throw();
         void chord_buffer(std::string& text) throw();
         void chord_buffer(std::ostringstream& stream) throw();

         // Interface for Info Log 
         void chord_info(const char* text) throw();
         void chord_info(std::string& text) throw();
         void chord_info(std::ostringstream& stream) throw();

         // Interface for Trace log 
         void chord_trace(const char* text) throw();
         void chord_trace(std::string& text) throw();
         void chord_trace(std::ostringstream& stream) throw();

         // Interface for Debug log 
         void chord_debug(const char* text) throw();
         void chord_debug(std::string& text) throw();
         void chord_debug(std::ostringstream& stream) throw();

         // Error and Alarm log must be always enable
         // Hence, there is no interfce to control error and alarm logs

         // Interfaces to control log levels
         void updateLogLevel(LogLevel logLevel);
         void enaleLog();  // Enable all log levels
         void disableLog(); // Disable all log levels, except error and alarm

         // Interfaces to control log Types
         void updateLogType(LogType logType);
         void enableConsoleLogging();
         void enableFileLogging();

      	 protected:
         Logger();
         ~Logger();

         // Wrapper function for lock/unlock
         // For Extensible feature, lock and unlock should be in protected
         void lock();
         void unlock();

         std::string getCurrentTime();

         private:
         void logIntoFile(const std::string& data);
         void logOnConsole(const std::string& data);
         void readConfig(); // /home/sagar55/Downloads/Chord/bin/LogConfig
         Logger(const Logger& obj) {}
         void operator=(const Logger& obj) {}

         private:
         static Logger*          m_Instance;
         static std::ofstream    m_File;

#ifdef	WIN32
         CRITICAL_SECTION        m_Mutex;
#else
         pthread_mutexattr_t     m_Attr; 
         pthread_mutex_t         m_Mutex;
#endif
	 LogLevel                m_LogLevel;
         LogType                 m_LogType;
   };

} // End of namespace

#endif // End of _LOGGER_H_

