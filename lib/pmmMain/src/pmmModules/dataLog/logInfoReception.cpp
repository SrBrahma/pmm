/* PmmModuleDataLog.cpp
 * Defines the Package Log (MLOG) and the Package Log Information (MLIN).
 *
 * By Henrique Bruno Fantauzzi de Almeida (aka SrBrahma) - Minerva Rockets, UFRJ, Rio de Janeiro - Brazil */

#include "crc.h"
#include "pmmModules/dataLog/dataLog.h"



// Received Package Log Info Package
int PmmModuleDataLog::receivedLogInfo(receivedPacketAllInfoStructType* packetInfo)
{

    uint16_t tempPackageCrc;
    unsigned packetId;

// 1) If the packet size is smaller than the packet header length, it's invalid
    if (packetInfo->payloadLength < PMM_PORT_LOG_INFO_HEADER_LENGTH)
        return 1;


// 2) Test the CRC, to see if the packet is valid.
    if (((packetInfo->payload[PMM_PORT_LOG_INFO_INDEX_CRC_PACKET_MSB] << 8) | (packetInfo->payload[PMM_PORT_LOG_INFO_INDEX_CRC_PACKET_LSB]))
                                  != crc16(packetInfo->payload + PMM_PORT_LOG_INFO_INDEX_CRC_PACKET_MSB + 1, packetInfo->payloadLength - 2))
        return 2;

    tempPackageCrc = (packetInfo->payload[PMM_PORT_LOG_INFO_INDEX_CRC_PACKAGE_MSB] << 8) | packetInfo->payload[PMM_PORT_LOG_INFO_INDEX_CRC_PACKAGE_LSB];


// 3) If is the first packet ever received
    if (!mLogInfoTelemetryNumberPackets)
    {
        memset(mLogInfoTelemetryArrayLengths, 0, PMM_PORT_LOG_INFO_MAX_PACKETS);
        mLogInfoTelemetryNumberPackets = (packetInfo->payload[PMM_PORT_LOG_INFO_INDEX_OF_Y_PACKETS]); // Only get the 4 least significant bits, and sum 1!
        // If is the first packet or if changed the entirePackageCrc, reset some parameters
    }

// 4) Get the packetId from the received packet
    packetId = packetInfo->payload[PMM_PORT_LOG_INFO_INDEX_PACKET_X];

    // Copies the received array
    memcpy(mLogInfoTelemetryArray[packetId], packetInfo->payload, packetInfo->payloadLength);

    mLogInfoTelemetryArrayLengths[packetId] = packetInfo->payloadLength;

    for (packetId = 0; packetId < mLogInfoTelemetryNumberPackets; packetId ++)
    {
        if (mLogInfoTelemetryArrayLengths == 0) // Test if any length is 0. If it is, a packet haven't been adquired yet.
            return; //  Leave the function. It has done it's job for now!
    }

    // If every packet has a length, all packets have been successfully been received.
    unitePackageInfoPackets(); // Packets of the world, unite!
}



void PmmModuleDataLog::unitePackageInfoPackets()
{
    unsigned packetCounter;
    unsigned payloadLength;
    uint16_t logInfoRawArrayIndex = 0; // unsigned or uint32_t was giving me a "warning: comparison between signed and unsigned integer expressions [-Wsign-compare]". why?
    unsigned stringSizeWithNullChar;
    unsigned variableCounter;

    mLogInfoRawPayloadArrayLength = 0;

// 1) Copies all the packets into the big raw array
    for (packetCounter = 0; packetCounter < mLogInfoTelemetryNumberPackets; packetCounter ++)
    {
        payloadLength = mLogInfoTelemetryArrayLengths[packetCounter] - PMM_PORT_LOG_INFO_HEADER_LENGTH;
        // Copies the telemetry array to the raw array. Skips the headers in the telemetry packet.
        memcpy(mPackageLogInfoRawArray + mLogInfoRawPayloadArrayLength,
               mLogInfoTelemetryArray[packetCounter] + PMM_PORT_LOG_INFO_HEADER_LENGTH,
               payloadLength);

        // Increases the raw array length by the copied telemetry array length.
        mLogInfoRawPayloadArrayLength += payloadLength;
    }



// 2) Now extracts all the info from the raw array.

    // 2.1) First get the number of variables
    mLogNumberVariables = mPackageLogInfoRawArray[0];
    
    // 2.2) Get the variable types and sizes
    for (variableCounter = 0; variableCounter < mLogNumberVariables; variableCounter++)
    {
        if (variableCounter % 2) // If is odd (if rest is 1)
        {
            mVariableTypeArray[variableCounter] = mPackageLogInfoRawArray[logInfoRawArrayIndex] & 0xF; // Get the 4 Least significant bits
        }
        else // Is even (if rest is 0). This will happen first
        {
            logInfoRawArrayIndex++;
            mVariableTypeArray[variableCounter] = mPackageLogInfoRawArray[logInfoRawArrayIndex] >> 4; // Get the 4 Most significant bits
        }
        mVariableSizeArray[variableCounter] = variableTypeToVariableSize(mVariableTypeArray[variableCounter]);
    }

    // 2.3) Now get the strings of the variables
    logInfoRawArrayIndex++;
    for (variableCounter = 0; logInfoRawArrayIndex < mLogInfoRawPayloadArrayLength - 1; variableCounter++)
    {
        stringSizeWithNullChar = strlen((char*)&mPackageLogInfoRawArray[logInfoRawArrayIndex]) + 1; // Includes '\0'.
        memcpy(mVariableNameArray[variableCounter], &mPackageLogInfoRawArray[logInfoRawArrayIndex], stringSizeWithNullChar);
        logInfoRawArrayIndex += stringSizeWithNullChar;
    }

    // Finished!
}
