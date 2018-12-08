/* pmmTelemetry.h
 *
 * By Henrique Bruno Fantauzzi de Almeida (aka SrBrahma) - Minerva Rockets, UFRJ, Rio de Janeiro - Brazil */

#ifndef PMM_TELEMETRY_h
#define PMM_TELEMETRY_h



// General defines

#include <RH_RF95.h>                            // Our current RF module!

  // For errors reporting and health status

#include "pmmTelemetry/pmmTelemetryProtocols.h" // For the PMM_NEO_PROTOCOL_HEADER_LENGTH define


#define PMM_TELEMETRY_MAX_PACKET_TOTAL_LENGTH   255 // Max LoRa Packet Size!

#define PMM_NEO_PROTOCOL_MAX_PAYLOAD_LENGTH     (PMM_TELEMETRY_MAX_PACKET_TOTAL_LENGTH - PMM_NEO_PROTOCOL_HEADER_LENGTH)

#define PMM_TELEMETRY_MAX_PAYLOAD_LENGTH        PMM_NEO_PROTOCOL_MAX_PAYLOAD_LENGTH // Since this is currently the only protocol.

#define PMM_TELEMETRY_QUEUE_LENGTH              8


class PmmTelemetry
{

    typedef enum
    {
        PMM_TELEMETRY_QUEUE_PRIORITY_HIGH,
        PMM_TELEMETRY_QUEUE_PRIORITY_NORMAL,
        PMM_TELEMETRY_QUEUE_PRIORITY_LOW
    } pmmTelemetryQueuePrioritiesType;

    typedef struct
    {
        toBeSentTelemetryPacketInfoStructType protocolsContentStructArray[PMM_TELEMETRY_QUEUE_LENGTH];
        uint8_t*  payloadArray[PMM_TELEMETRY_QUEUE_LENGTH];
        uint8_t   lengthArray [PMM_TELEMETRY_QUEUE_LENGTH];
        uint8_t   actualIndex;
        uint8_t   remainingItemsOnQueue; // How many items on this queue not sent yet.
    } pmmTelemetryQueueStructType;



public:

    PmmTelemetry();

    int init();

    int updateReception();
    int updateTransmission();

    int addSendToQueue(uint8_t dataArray[], uint8_t totalByteSize, toBeSentTelemetryPacketInfoStructType protocolsContentStruct, pmmTelemetryQueuePrioritiesType priority);

    uint8_t* getReceivedPacketArray();
    
    receivedPacketAllInfoStructType* getReceivedPacketStatusStructPtr();

private:

    RH_RF95  mRf95;

    unsigned mTelemetryIsWorking;

    receivedPacketAllInfoStructType*           mReceivedPacketAllInfoStructPtr;
    receivedPacketPhysicalLayerInfoStructType* mReceivedPacketPhysicalLayerInfoStructPtr;

    uint8_t  mReceivedPacket[PMM_TELEMETRY_MAX_PACKET_TOTAL_LENGTH];

    pmmTelemetryQueueStructType mHighPriorityQueueStruct;
    pmmTelemetryQueueStructType mNormalPriorityQueueStruct;
    pmmTelemetryQueueStructType mLowPriorityQueueStruct;

    int tryToAddToQueue(pmmTelemetryQueuePrioritiesType priority, pmmTelemetryQueueStructType *pmmTelemetryQueueStructPtr);

};


#endif
