/* pmmGps.cpp
 * Some commentaries were taken from the neoGps lib.
 * By Henrique Bruno Fantauzzi de Almeida (aka SrBrahma), Rio de Janeiro - Brazil */

#include <Arduino.h>
#include <NMEAGPS.h>
#include "pmmConsts.h"
#include "pmmDebug.h"

#include "pmmGps/gps.h"

//------------------------------------------------------------
#if PMM_DEBUG && PMM_DEBUG_MORE && PMM_GPS_DEBUG_MORE
    #include <Streamers.h>
    // example from "trace_all(DEBUG_PORT, mNMEAGPS, mFix);"
    // Status,UTC Date/Time,Lat,Lon,Hdg,Spd,Alt,Sats,Rx ok,Rx err,Rx chars,
    // 3,2018-04-25 16:32:02.840,-228587517,-432291933,4830,5200,,,5,0,356,
    // 3,2018-04-25 16:32:05.840,-228588867,-432293117,4330,1700,630,3,26,0,1661,
#endif

PmmGps::PmmGps(){}

int PmmGps::init()
{
    PMM_GPS_PORT.begin(9600);

    if (!PMM_GPS_PORT)
    {
        advPrintf("Initialization failed!\n");
        mGpsIsWorking = 0;
        return 1;
    }

    gpsDebugMorePrintf("Initialized successfully!\n");
    mGpsIsWorking = 1;
    mLastLocationTimeMs = 0;
    return 0;
}

// Be aware that there isn't being used the option of a buffer of fixes, present in the NMEAGPS (NMEAGPS_FIX_MAX on NMEAGPS_cfg.h),
// So if >1 packets arrive without you running this update(), you will lose the first fix that arrived.
// I won't lose time right now managing this, as the main loop is fast enought, so it won't happen. And if it happens, for some temporary freeze
// in the program, it will be only one fix that will be lost.
PmmGps::UpdateRtn PmmGps::update(uint32_t timeMs)
{
    if (!mGpsIsWorking)
        return UpdateRtn::NotWorking;

    if (!mNMEAGPS.available(PMM_GPS_PORT))
        return UpdateRtn::OkNoData;

    mFix = mNMEAGPS.read();
    if (mFix.valid.location)
        mLastLocationTimeMs = timeMs;

    debugPrintFix(Serial, mNMEAGPS, mFix);

    fixToOurType(mFix, mPmmGpsStruct);

    return UpdateRtn::GotFix; // not necessarily coord
}

// All info below on https://github.com/SlashDevin/NeoGPS/blob/master/extras/doc/Data%20Model.md !!
void PmmGps::fixToOurType(gps_fix &mFix, pmmGpsStructType &mPmmGpsStruct)
{
    #ifdef GPS_FIX_LOCATION
        mPmmGpsStruct.latitude        = mFix.latitudeL();
        mPmmGpsStruct.longitude       = mFix.longitudeL();
    #endif

    #ifdef GPS_FIX_ALTITUDE
        mPmmGpsStruct.altitude        = mFix.altitude();
    #endif

    #ifdef GPS_FIX_SATELLITES
        mPmmGpsStruct.satellites      = mFix.satellites;
    #endif

    #ifdef GPS_FIX_HEADING
        mPmmGpsStruct.headingDegree   = mFix.heading();
    #endif

    #ifdef GPS_FIX_SPEED
        mPmmGpsStruct.upSpeed         = mFix.velocity_down    / (-100.0);  // As NeoGps outputs this in cm/s, we divide by 100 to get in m/s.
        mFix.calculateNorthAndEastVelocityFromSpeedAndHeading();
        mPmmGpsStruct.horizontalSpeed = mFix.speed_metersph() /   3600.0;  // In m/s
        #ifdef GPS_FIX_HEADING
            mPmmGpsStruct.northSpeed  = mFix.velocity_north   /    100.0;  // As NeoGps outputs this in cm/s, we divide by 100 to get in m/s.
            mPmmGpsStruct.eastSpeed   = mFix.velocity_east    /    100.0;  // As NeoGps outputs this in cm/s, we divide by 100 to get in m/s.
        #endif
    #endif


    // if firstDate
    // save/tlm like "original_launch": "2007-03-21T01:10:00.000Z" (https://docs.spacexdata.com/?version=latest), on system millis X
    // option to change UTC
}

void PmmGps::debugPrintFix(Print &Serial, const NMEAGPS &mNMEAGPS, const gps_fix &mFix)
{
    #if PMM_DEBUG && PMM_DEBUG_MORE && PMM_GPS_DEBUG_MORE
        gpsDebugMorePrintf("GPS updated. trace_all(): ")
        gpsDebugMorePrintf("Status,UTC Date/Time,Lat,Lon,Hdg,Spd,Alt,Sats,Rx ok,Rx err,Rx chars\n");
        trace_all(Serial, mNMEAGPS, mFix);
    #endif
}

double PmmGps::distanceToInMeters(int32_t targetLatitude, int32_t targetLongitude) {
    NeoGPS::Location_t targetLoc(targetLatitude, targetLongitude);
    return mFix.location.DistanceKm(targetLoc) * 1000;
}

float PmmGps::bearingToInDegrees(int32_t targetLatitude, int32_t targetLongitude) {
    NeoGPS::Location_t targetLoc(targetLatitude, targetLongitude);
    return mFix.location.BearingToDegrees(targetLoc);
}

pmmGpsStructType* PmmGps::getGpsStructPtr() { return &mPmmGpsStruct;}
pmmGpsStructType  PmmGps::getGpsStruct   () { return  mPmmGpsStruct;}
gps_fix*          PmmGps::getFixPtr      () { return &mFix         ;}
gps_fix           PmmGps::getFix         () { return  mFix         ;}

uint32_t  PmmGps::getLastLocationTimeMs()    { return  mLastLocationTimeMs; }
uint32_t* PmmGps::getLastLocationTimeMsPtr() { return &mLastLocationTimeMs; }