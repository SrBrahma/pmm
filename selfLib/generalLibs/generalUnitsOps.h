#ifndef GENERAL_UNITS_OPS_h
#define GENERAL_UNITS_OPS_h

#include <stdint.h> // For uint32_t

// Converts the latitude or longitude in uint32_t type to float type.
inline float coord32ToFloat(uint32_t latOrLon)
{
    return latOrLon * 1.0e-7; // That's how it happens on Location.h, of NeoGps lib.
}

inline uint32_t secondsToMicros(double seconds) { return seconds * 1000000; }
inline uint32_t secondsToMillis(double seconds) { return seconds *    1000; }
inline uint32_t millisToMicros (double millis ) { return millis  *    1000; }

// Also check time difference if the time had overflowed.
inline uint32_t timeDifference(uint32_t newTime, uint32_t oldTime)
{
    if (newTime < oldTime) // If the time has overflowed.
        return (0xFFFFFFFF - oldTime + newTime);

    return (newTime - oldTime);
}

// https://forum.arduino.cc/index.php?topic=371564.0
inline double randomDouble(double minf, double maxf)
{
  return minf + random(1UL << 31) * (maxf - minf) / (1UL << 31);  // use 1ULL<<63 for max double values)
}
#endif