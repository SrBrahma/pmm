/* PmmPortLog.cpp
 * Defines the Package Log (MLOG) and the Package Log Information (MLIN).
 *
 * By Henrique Bruno Fantauzzi de Almeida (aka SrBrahma) - Minerva Rockets, UFRJ, Rio de Janeiro - Brazil */

#include "pmmTelemetryPorts/pmmPortLog.h"
#include <crc16.h>
#include <pmmConsts.h>
#include <pmmTelemetry.h>



#define PMM_PORT_LOG_INDEX_DATA 1
// 0  is MLIN String CRC
// 1+ is data (PMM_PORT_LOG_INDEX_DATA)



const PROGMEM char PMM_TELEMETRY_ALTITUDE_DEFAULT_STRING[] =    {"altitude(m)"};
const PROGMEM char PMM_TELEMETRY_GPS_LAT_DEFAULT_STRING[] =     {"gpsLongitude"};
const PROGMEM char PMM_TELEMETRY_GPS_LON_DEFAULT_STRING[] =     {"gpsLatitude"};



PmmPortLog::PmmPortLog()
{
}



int PmmPortLog::init(PmmTelemetry* pmmTelemetry)
{
    mPmmTelemetry = pmmTelemetry;

    mPackageLogSizeInBytes = 0;
    mLogNumberOfVariables = 0;
    mPackageLogInfoNumberOfPackets = 0; // For receptor.

    const PROGMEM char* logInfoPackageCrcStr = "logInfoPackageCrc"; // It isn't actually used. But will leave it for the future. (CMON 11 BYTES AT PROGMEM IS NOTHING)
    addCustomVariable(logInfoPackageCrcStr, PMM_TELEMETRY_TYPE_UINT16, &mLogInfoPackageCrc); // Package Log Info CRC

    return 0;
}



uint8_t PmmPortLog::variableTypeToVariableSize(uint8_t variableType)
{
    switch (variableType)
    {
        case PMM_TELEMETRY_TYPE_UINT8:
            return 1;
        case PMM_TELEMETRY_TYPE_INT8:
            return 1;
        case PMM_TELEMETRY_TYPE_UINT16:
            return 2;
        case PMM_TELEMETRY_TYPE_INT16:
            return 2;
        case PMM_TELEMETRY_TYPE_UINT32:
            return 4;
        case PMM_TELEMETRY_TYPE_INT32:
            return 4;
        case PMM_TELEMETRY_TYPE_FLOAT:
            return 4;
        case PMM_TELEMETRY_TYPE_UINT64:
            return 8;
        case PMM_TELEMETRY_TYPE_INT64:
            return 8;
        case PMM_TELEMETRY_TYPE_DOUBLE:
            return 8;
        default:    // Maybe will avoid internal crashes?
            PMM_DEBUG_PRINT("PmmPort #1: Invalid variable type to size!");
            return 1;
    }
}



void PmmPortLog::includeVariableInPackage(const char *variableName, uint8_t variableType, void *variableAddress)
{
    uint8_t varSize = variableTypeToVariableSize(variableType);
    if (mLogNumberOfVariables >= PMM_PORT_LOG_NUMBER_VARIABLES)
    {
        #if PMM_DEBUG_SERIAL
            Serial.print("PmmPort #2: Failed to add the variable \"");
            Serial.print(variableName);
            Serial.print("\". Exceeds the maximum number of variables in the Package Log.\n");
        #endif
        return;
    }
    if ((mPackageLogSizeInBytes + varSize) >= PMM_TELEMETRY_MAX_PAYLOAD_LENGTH)
    {
        #if PMM_DEBUG_SERIAL
            Serial.print("PmmPort #3: Failed to add the variable \"");
            Serial.print(variableName);
            Serial.print("\". Exceeds the maximum payload length (tried to be ");
            Serial.print((mPackageLogSizeInBytes + varSize));
            Serial.print(", maximum is ");
            Serial.print(PMM_TELEMETRY_MAX_PAYLOAD_LENGTH);
            Serial.print(".\n");
        #endif
        return;
    }

    mVariableNameArray[mLogNumberOfVariables] = (char*) variableName; // Typecast from (const char*) to (char*)
    mVariableTypeArray[mLogNumberOfVariables] = variableType;
    mVariableSizeArray[mLogNumberOfVariables] = varSize;
    mVariableAddressArray[mLogNumberOfVariables] = (uint8_t*) variableAddress;
    mLogNumberOfVariables ++;
    mPackageLogSizeInBytes += varSize;

    if (mLogNumberOfVariables > PMM_PORT_LOG_INDEX_DATA) // yeah it's right. It isn't actually necessary, just skip a few useless function calls.
    {
        updatePackageLogInfoRaw();
        updatePackageLogInfoInTelemetryFormat();
    }
}

void PmmPortLog::includeArrayInPackage(const char **variableName, uint8_t arrayType, void *arrayAddress, uint8_t arraySize)
{
    uint8_t counter;
    for (counter = 0; counter < arraySize; counter++)
        includeVariableInPackage(*variableName++, arrayType, (uint8_t*) arrayAddress + (variableTypeToVariableSize(arrayType) * counter));
}



void PmmPortLog::addPackageBasicInfo(uint32_t* packageIdPtr, uint32_t* packageTimeMsPtr)
{
    const PROGMEM char* packageIdString = "packageId";
    const PROGMEM char* packageTimeString = "packageTime(ms)";
    includeVariableInPackage(packageIdString, PMM_TELEMETRY_TYPE_UINT32, packageIdPtr);
    includeVariableInPackage(packageTimeString, PMM_TELEMETRY_TYPE_UINT32, packageTimeMsPtr);
}

void PmmPortLog::addMagnetometer(void* array)
{
    const PROGMEM char* arrayString[3] = {"magnetometerX(uT)", "magnetometerY(uT)", "magnetometerZ(uT)"};
    includeArrayInPackage(arrayString, PMM_TELEMETRY_TYPE_FLOAT, array, 3);
}

void PmmPortLog::addGyroscope(void* array)
{
    const PROGMEM char* arrayString[3] = {"gyroscopeX(degree/s)", "gyroscopeY(degree/s)", "gyroscopeZ(degree/s)"};
    includeArrayInPackage(arrayString, PMM_TELEMETRY_TYPE_FLOAT, array, 3);
}

void PmmPortLog::addAccelerometer(void* array)
{
    const PROGMEM char* arrayString[3] = {"accelerometerX(g)", "accelerometerY(g)", "accelerometerZ(g)"};
    includeArrayInPackage(arrayString, PMM_TELEMETRY_TYPE_FLOAT, array, 3);
}

void PmmPortLog::addBarometer(void* barometer)
{
    const PROGMEM char* barometerPressureString = "barometerPressure(hPa)";
    includeVariableInPackage(barometerPressureString, PMM_TELEMETRY_TYPE_FLOAT, barometer);
}

void PmmPortLog::addAltitudeBarometer(void* altitudePressure)
{
    const PROGMEM char* barometerAltitudeString = "barometerAltitude(m)";
    includeVariableInPackage(barometerAltitudeString, PMM_TELEMETRY_TYPE_FLOAT, altitudePressure);
}

void PmmPortLog::addThermometer(void* thermometerPtr)
{
    const PROGMEM char* thermometerString = "temperature(C)";
    includeVariableInPackage(thermometerString, PMM_TELEMETRY_TYPE_FLOAT, thermometerPtr);
}

void PmmPortLog::addImu(pmmImuStructType *pmmImuStructPtr)
{
    addAccelerometer(pmmImuStructPtr->accelerometerArray);
    addGyroscope(pmmImuStructPtr->gyroscopeArray);
    addMagnetometer(pmmImuStructPtr->magnetometerArray);

    addBarometer(&pmmImuStructPtr->pressure);
    addAltitudeBarometer(&pmmImuStructPtr->altitudePressure);
    addThermometer(&pmmImuStructPtr->temperature);
}

void PmmPortLog::addGps(pmmGpsStructType* pmmGpsStruct)
{
    #ifdef GPS_FIX_LOCATION
        const PROGMEM char* gpsLatitudeString = "gpsLatitude";
        const PROGMEM char* gpsLongitudeString = "gpsLongitude";
        includeVariableInPackage(gpsLatitudeString, PMM_TELEMETRY_TYPE_FLOAT, &(pmmGpsStruct->latitude));
        includeVariableInPackage(gpsLongitudeString, PMM_TELEMETRY_TYPE_FLOAT, &(pmmGpsStruct->longitude));
    #endif

    #ifdef GPS_FIX_ALTITUDE
        const PROGMEM char* gpsAltitudeString = "gpsAltitude(m)";
        includeVariableInPackage(gpsAltitudeString, PMM_TELEMETRY_TYPE_FLOAT, &(pmmGpsStruct->altitude));
    #endif

    #ifdef GPS_FIX_SATELLITES
        const PROGMEM char* gpsSatellitesString = "gpsSatellites";
        includeVariableInPackage(gpsSatellitesString, PMM_TELEMETRY_TYPE_UINT8, &(pmmGpsStruct->satellites));
    #endif
    /*
    #ifdef GPS_FIX_SPEED
        const PROGMEM char* gpsHorizontalSpeedString = "gpsHorSpeed(m/s)";
        const PROGMEM char* gpsNorthSpeedString = "gpsNorthSpeed(m/s)";
        const PROGMEM char* gpsEastSpeedString = "gpsEastSpeed(m/s)";
        const PROGMEM char* gpsHeadingDegreeString = "gpsHeadingDegree";
        includeVariableInPackage(gpsHorizontalSpeedString, PMM_TELEMETRY_TYPE_FLOAT, &(pmmGpsStruct->horizontalSpeed));
        includeVariableInPackage(gpsNorthSpeedString, PMM_TELEMETRY_TYPE_FLOAT, &(pmmGpsStruct->northSpeed));
        includeVariableInPackage(gpsEastSpeedString, PMM_TELEMETRY_TYPE_FLOAT, &(pmmGpsStruct->eastSpeed));
        includeVariableInPackage(gpsHeadingDegreeString, PMM_TELEMETRY_TYPE_FLOAT, &(pmmGpsStruct->headingDegree));

        #ifdef GPS_FIX_ALTITUDE
            const PROGMEM char* gpsUpSpeedString = "gpsSpeedUp(m/s)";
            includeVariableInPackage(gpsUpSpeedString, PMM_TELEMETRY_TYPE_FLOAT, &pmmGpsStruct->upSpeed);
        #endif
    #endif*/
}

void PmmPortLog::addCustomVariable(const char* variableName, uint8_t variableType, void* variableAddress)
{
    includeVariableInPackage(variableName, variableType, variableAddress);
}



// Log Info Package in Telemetry format (MLIN)
void PmmPortLog::updatePackageLogInfoRaw()
{
    uint8_t variableCounter;
    uint8_t stringLengthWithNullChar;

    mPackageLogInfoRawArray[0] = mLogNumberOfVariables;
    mPackageLogInfoRawArrayLength = 1;

    // 1) Add the variable types
    for (variableCounter = 0; variableCounter < mLogNumberOfVariables; variableCounter++)
    {
        // 1.1) If is odd (if rest is 1)
        if (variableCounter % 2)
        {
            mPackageLogInfoRawArray[mPackageLogInfoRawArrayLength] |= mVariableTypeArray[variableCounter]; // Add it on the right
            mPackageLogInfoRawArrayLength++;
        }

        // 1.2) Is even (rest is 0). As it happens first than the odd option, no logical OR is needed.
        else
            mPackageLogInfoRawArray[mPackageLogInfoRawArrayLength] = mVariableTypeArray[variableCounter] << 4; // Shift Left 4 positions to add to the left
    }

    // 1.3) If for loop ended on a even number (and now the variable is odd due to the final increment that made it >= mLogNumberOfVariables)
    if (variableCounter % 2)
        mPackageLogInfoRawArrayLength++; // As this variable only increased in odd numbers.


    // 2) Now add the length of the following strings, and the strings of each variable
    for (variableCounter = 0; variableCounter < mLogNumberOfVariables; variableCounter++)
    {

        // 2.1) Adds the strings, with the null-char
        stringLengthWithNullChar = strlen(mVariableNameArray[variableCounter]) + 1; // Doesn't count the '\0'.
        memcpy(&mPackageLogInfoRawArray[mPackageLogInfoRawArrayLength], mVariableNameArray[variableCounter], stringLengthWithNullChar);
        mPackageLogInfoRawArrayLength += stringLengthWithNullChar;
    }

    // 3) Calculate the total number of packets.
    mPackageLogInfoNumberOfPackets = ceil(mPackageLogInfoRawArrayLength / (float) PMM_PORT_LOG_INFO_MAX_PAYLOAD_LENGTH);
    // This is different to PMM_PORT_LOG_INFO_MAX_PACKETS, as the macro is the maximum number of packets, and this variable is the actual maximum
    // number of packets. This one varies with the actual contents in MLIN Package.
}



void PmmPortLog::updatePackageLogInfoInTelemetryFormat()
{
    // The port format is in pmmPortLog.h

    uint16_t packetLength = 0; // The Package Header default length.
    uint16_t crc16ThisPacket;
    mLogInfoPackageCrc = CRC16_DEFAULT_VALUE;
    uint16_t payloadBytesInThisPacket;
    uint8_t packetCounter;

    // 1) Copies the raw array content and the package header into the packets
    for (packetCounter = 0; packetCounter < mPackageLogInfoNumberOfPackets; packetCounter++)
    {
        packetLength = PMM_PORT_LOG_INFO_HEADER_LENGTH; // The initial length is the default header length

        // This packet size is the total raw size minus the (actual packet * packetPayloadLength).
        // If it is > maximum payload length, it will be equal to the payload length.
        payloadBytesInThisPacket = mPackageLogInfoRawArrayLength - (packetCounter * PMM_PORT_LOG_INFO_MAX_PAYLOAD_LENGTH);
        if (payloadBytesInThisPacket > PMM_PORT_LOG_INFO_MAX_PAYLOAD_LENGTH)
            payloadBytesInThisPacket = PMM_PORT_LOG_INFO_MAX_PAYLOAD_LENGTH;

        packetLength += payloadBytesInThisPacket;

        // Adds the requested packet and the total number of packets - 1.
        mPackageLogInfoTelemetryArray[packetCounter][PMM_PORT_LOG_INFO_INDEX_PACKET_X_OF_Y_MINUS_1] = (packetCounter << 4) | (mPackageLogInfoNumberOfPackets - 1);

        // Now adds the data, which was built on updatePackageLogInfoRaw(). + skips the packet header.
        memcpy(mPackageLogInfoTelemetryArray[packetCounter] + PMM_PORT_LOG_INFO_HEADER_LENGTH, mPackageLogInfoRawArray, payloadBytesInThisPacket);

        // Set the CRC16 of this packet fields as 0 (to calculate the entire packet CRC16 without caring about positions and changes in headers, etc)
        mPackageLogInfoTelemetryArray[packetCounter][PMM_PORT_LOG_INFO_INDEX_LSB_CRC_PACKET] = 0;
        mPackageLogInfoTelemetryArray[packetCounter][PMM_PORT_LOG_INFO_INDEX_MSB_CRC_PACKET] = 0;

        // Set the CRC16 of the entire package to 0.
        mPackageLogInfoTelemetryArray[packetCounter][PMM_PORT_LOG_INFO_INDEX_LSB_CRC_PACKAGE] = 0;
        mPackageLogInfoTelemetryArray[packetCounter][PMM_PORT_LOG_INFO_INDEX_MSB_CRC_PACKAGE] = 0;

        mLogInfoPackageCrc = crc16(mPackageLogInfoTelemetryArray[packetCounter], packetLength, mLogInfoPackageCrc); // The first crc16Package is = CRC16_DEFAULT_VALUE, as stated.
    }

    // 2) Assign the entire package crc16 to all packets.
    for (packetCounter = 0; packetCounter < mPackageLogInfoNumberOfPackets; packetCounter++)
    {
        mPackageLogInfoTelemetryArray[packetCounter][PMM_PORT_LOG_INFO_INDEX_LSB_CRC_PACKAGE] = mLogInfoPackageCrc;        // Little endian!
        mPackageLogInfoTelemetryArray[packetCounter][PMM_PORT_LOG_INFO_INDEX_MSB_CRC_PACKAGE] = mLogInfoPackageCrc >> 8;   //
    }

    // 3) CRC16 of this packet:
    for (packetCounter = 0; packetCounter < mPackageLogInfoNumberOfPackets; packetCounter++)
    {
        crc16ThisPacket = crc16(mPackageLogInfoTelemetryArray[packetCounter], packetLength); // As the temporary CRC16 of this packet is know to be 0,
        //it can do the crc16 of the packet without skipping the crc16 fields

        mPackageLogInfoTelemetryArray[packetCounter][PMM_PORT_LOG_INFO_INDEX_LSB_CRC_PACKET] = crc16ThisPacket;        // Little endian!
        mPackageLogInfoTelemetryArray[packetCounter][PMM_PORT_LOG_INFO_INDEX_MSB_CRC_PACKET] = crc16ThisPacket >> 8;   //

        mPackageLogInfoTelemetryArrayLengths[packetCounter] = packetLength;
    }
}




/* Getters! -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
uint8_t PmmPortLog::getNumberOfVariables()
{
    return mLogNumberOfVariables;
}

uint8_t PmmPortLog::getPackageLogSizeInBytes()
{
    return mPackageLogSizeInBytes;
}

const char** PmmPortLog::getVariableNameArray()  { return (const char**) mVariableNameArray;}
uint8_t* PmmPortLog::getVariableTypeArray()      { return mVariableTypeArray;}
uint8_t* PmmPortLog::getVariableSizeArray()      { return mVariableSizeArray;}
uint8_t** PmmPortLog::getVariableAddressArray()     { return mVariableAddressArray;}







/* Debug! -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

#if PMM_DEBUG_SERIAL
// Note for the 2 functions below:
// There are faster ways to print the debugs, but since it isn't something that is going to be used frequently,
// I (HB :) ) will spend my precious time on other stuffs)

void PmmPortLog::debugPrintLogHeader()
{
    unsigned variableIndex;
    char buffer[512] = {0}; // No static needed, as it is called usually only once.

    // For adding the first variable header to the print
    if (mLogNumberOfVariables > PMM_PORT_LOG_INDEX_DATA)
        snprintf(buffer, 512, "%s", mVariableNameArray[PMM_PORT_LOG_INDEX_DATA]);

    for (variableIndex = PMM_PORT_LOG_INDEX_DATA + 1; variableIndex < mLogNumberOfVariables; variableIndex ++)
    {
        snprintf(buffer, 512, "%s | %s", buffer, mVariableNameArray[variableIndex]);
    }
    Serial.println(buffer);
}

void PmmPortLog::debugPrintLogContent()
{
    unsigned variableIndex;
    static char buffer[512]; // Static for optimization
    buffer[0] = '\0'; // for the snprintf
    for (variableIndex = 0; variableIndex < mLogNumberOfVariables; variableIndex ++)
    {
        switch(mVariableTypeArray[variableIndex])
        {
            case PMM_TELEMETRY_TYPE_FLOAT: // first as it is more common
                snprintf(buffer, 512, "%s%f, ", buffer, *(float*) (mVariableAddressArray[variableIndex]));
                break;
            case PMM_TELEMETRY_TYPE_UINT32:
                snprintf(buffer, 512, "%s%lu, ", buffer, *(uint32_t*) (mVariableAddressArray[variableIndex]));
                break;
            case PMM_TELEMETRY_TYPE_INT32:
                snprintf(buffer, 512, "%s%li, ", buffer, *(int32_t*) (mVariableAddressArray[variableIndex]));
                break;
            case PMM_TELEMETRY_TYPE_UINT8:
                snprintf(buffer, 512, "%s%u, ", buffer, *(uint8_t*) (mVariableAddressArray[variableIndex]));
                break;
            case PMM_TELEMETRY_TYPE_INT8:
                snprintf(buffer, 512, "%s%i, ", buffer, *(int8_t*) (mVariableAddressArray[variableIndex]));
                break;
            case PMM_TELEMETRY_TYPE_UINT16:
                snprintf(buffer, 512, "%s%u, ", buffer, *(uint16_t*) (mVariableAddressArray[variableIndex]));
                break;
            case PMM_TELEMETRY_TYPE_INT16:
                snprintf(buffer, 512, "%s%i, ", buffer, *(int16_t*) (mVariableAddressArray[variableIndex]));
                break;
            case PMM_TELEMETRY_TYPE_UINT64:
                snprintf(buffer, 512, "%s%llu, ", buffer, *(uint64_t*) (mVariableAddressArray[variableIndex]));
                break;
            case PMM_TELEMETRY_TYPE_INT64:
                snprintf(buffer, 512, "%s%lli, ", buffer, *(int64_t*) (mVariableAddressArray[variableIndex]));
                break;
            case PMM_TELEMETRY_TYPE_DOUBLE:
                snprintf(buffer, 512, "%s%f, ", buffer, *(double*) (mVariableAddressArray[variableIndex]));
                break;
            default:    // If none above,
                snprintf(buffer, 512, "%s%s, ", buffer, ">TYPE ERROR HERE!<");
                break;
        } // switch end
    } // for loop end
    Serial.println(buffer);
} // end of function debugPrintLogContent()



#endif