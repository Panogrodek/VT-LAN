#pragma once

#ifndef DISTRIBUTE
#include "spdlog defined types.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace fs {
	namespace fs_priv {
		inline std::shared_ptr<spdlog::logger> StrippedLogger  = spdlog::stdout_color_mt("StrippedLogger");
		inline std::shared_ptr<spdlog::logger> DefaultLogger   = spdlog::stdout_color_mt("DefaultLogger");
		inline std::shared_ptr<spdlog::logger> AssertionLogger = spdlog::stdout_color_mt("AssertionLogger");

		static void InitializeLogger() {
			StrippedLogger->set_pattern("%v");
			DefaultLogger->set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
			AssertionLogger->set_pattern("[%H:%M:%S.%e] [%^ASSERTION%$] Triggered in: %v");
		}
	}
}

#define INITIALIZE_LOGGER() fs::fs_priv::InitializeLogger();

#define ASSERT(VAR)	   if(!(VAR))	__debugbreak();
#define DB_ASSERT(VAR) if(!(VAR))	__debugbreak();

#define LOG(...)       fs::fs_priv::StrippedLogger->info(__VA_ARGS__)
#define ILOG(...)      fs::fs_priv::DefaultLogger->info(__VA_ARGS__)
#define ELOG(...)      fs::fs_priv::DefaultLogger->error(__VA_ARGS__)
#define WLOG(...)	   fs::fs_priv::DefaultLogger->warn(__VA_ARGS__)
#define CLOG(...)	   fs::fs_priv::DefaultLogger->critical(__VA_ARGS__)
#define ALOG(...)                                                        \
	do {                                                                 \
		fs::fs_priv::AssertionLogger->critical(                          \
			"[{}()] " __VA_ARGS__, __func__);							 \
		DB_ASSERT(false);                                                \
	} while (0)

#define DB_ONLY(VAR)   VAR

// helper macros to concatenate names
#define CONCAT_DETAIL(x, y) x##y
#define CONCAT(x, y) CONCAT_DETAIL(x, y)

#define WARN_ONCE(...)												 \
        do {                                                         \
            static bool CONCAT(_warned_already_, __LINE__) = false;  \
            if (!CONCAT(_warned_already_, __LINE__)) {               \
                CONCAT(_warned_already_, __LINE__) = true;           \
                fs::fs_priv::DefaultLogger->warn(__VA_ARGS__);       \
            }                                                        \
        } while (0)

#else
#define INITIALIZE_LOGGER()

#define ASSERT(VAR) VAR
#define DB_ASSERT(VAR)

#define LOG(...)  
#define ILOG(...) 
#define ELOG(...) 
#define WLOG(...)
#define CLOG(...)

#define DB_ONLY(VAR)

#define WARN_ONCE(...)

#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif 
