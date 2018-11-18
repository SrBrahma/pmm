#include <string.h>
#include "pmmSd/pmmSd.h"
#include "pmmSd/pmmSdSafeLog.h"

// These are files which are:
// 1) Pre-allocated with a length of X blocks - (use the kibibytesToBlocksAmount() or the mebibytesToBlocksAmount() functions to !)
//  The pre-allocation will also erase previous garbages. So, the blocks will be filled with zeroes or ones, deppending on the flash memory vendor.
// 2) Separated in parts. Each part, will have the length of the previous item.
// 3) Each write, have the same length -- maybe for now?
// 4)

// It won't use SdFatSdioEX as it uses multi-block writing, and by doing it, we lose the control of the safety system.

// By having the backup blocks always ahead of the current block instead of a fixed place for them, we distribute the SD


PmmSdSafeLog::PmmSdSafeLog(PmmSd *pmmSd, uint16_t defaulBlocksAllocationPerPart) // To use multiple buffers
{
    mPmmSd = pmmSd;
    mDefaultBlocksAllocationPerPart = defaulBlocksAllocationPerPart;
}


void PmmSdSafeLog::initSafeLogStatusStruct(safeLogStatusStructType * statusStruct, uint8_t groupLength, uint16_t blocksPerPart)
{
    statusStruct->currentBlock           = 0;
    statusStruct->freeBlocksAfterCurrent = 0;
    statusStruct->groupLength            = groupLength;
    statusStruct->currentPart            = 0;
    
    statusStruct->currentPositionInBlock = 0;

    if (blocksPerPart == 0)
        statusStruct->blocksPerPart      = mDefaultBlocksAllocationPerPart;
    else
        statusStruct->blocksPerPart      = blocksPerPart;
}


// The statusStruct must be initSafeLogStatusStruct() before.
int PmmSdSafeLog::write(uint8_t data[], char dirFullRelativePath[], safeLogStatusStructType* statusStruct)
{
    unsigned bytesRemaining = statusStruct->groupLength;
    uint32_t endBlock;

    // 1) Is this the first time running for this statusStruct?
    if (statusStruct->currentBlock == 0)
    {
        mPmmSd->allocateFilePart(dirFullRelativePath, PMM_SD_SAFE_LOG_EXTENSION, statusStruct->currentPart, statusStruct->blocksPerPart, &(statusStruct->currentBlock), &endBlock);
        statusStruct->freeBlocksAfterCurrent = endBlock - statusStruct->currentBlock;
    }

    // 2) Was there any previous data written on the actual block?
    else if (statusStruct->currentPositionInBlock > 0)
        
    //actualPart;
    //blocksPerPart;
    actualPositionBlock : 9;

    // Allocate new file part
    statusStruct->currentPart++;
    mPmmSd->allocateFilePart(dirFullRelativePath, PMM_SD_SAFE_LOG_EXTENSION, statusStruct->currentPart, statusStruct->blocksPerPart, &(statusStruct->currentBlock), &endBlock);

    statusStruct->freeBlocksAfterCurrent = endBlock - statusStruct->currentBlock;

}