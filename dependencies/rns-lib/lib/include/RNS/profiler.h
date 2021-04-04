#pragma once

#if SL_INSTR
#include <Superluminal/PerformanceAPI.h>
#define RNS_PROFILE_FUNCTION() PERFORMANCEAPI_INSTRUMENT_FUNCTION()
#else
#define RNS_PROFILE_FUNCTION()
#endif
