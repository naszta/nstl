#ifndef _NSTL_COMPILER
#define _NSTL_COMPILER

#ifdef _WIN32

#define NSTL_FENV_ACCESS_ON _Pragma("fenv_access (on)")

#define NSTL_WRN_SWITCH_ENUM_PUSH
#define NSTL_WRN_SWITCH_ENUM_POP

#define NSTL_WRN_DATE_PUSH
#define NSTL_WRN_DATE_POP

#else

#define NSTL_FENV_ACCESS_ON

#define NSTL_WRN_SWITCH_ENUM_PUSH _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wswitch-enum\"")
#define NSTL_WRN_SWITCH_ENUM_POP _Pragma("GCC diagnostic pop")

#define NSTL_WRN_DATE_PUSH _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wold-style-cast\"") \
    _Pragma("GCC diagnostic ignored \"-Wswitch-enum\"") \
    _Pragma("GCC diagnostic ignored \"-Wswitch-default\"")
#define NSTL_WRN_DATE_POP _Pragma("GCC diagnostic pop")

#endif

#endif
