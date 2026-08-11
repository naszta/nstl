#ifndef _NSTL_COMPILER
#define _NSTL_COMPILER

#ifdef _WIN32

#define NSTL_WRN_SWITCH_ENUM_PUSH
#define NSTL_WRN_SWITCH_ENUM_POP

#else

#define NSTL_WRN_SWITCH_ENUM_PUSH _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wswitch-enum\"")
#define NSTL_WRN_SWITCH_ENUM_POP _Pragma("GCC diagnostic pop")

#endif

#endif
