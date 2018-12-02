#include <string.h>
#include "pmmConsts.h"

#include "pmmSd/pmmSd.h"
#include "pmmSd/pmmSdAllocation.h"
#include "pmmSd/pmmSdSafeLog.h"

// These are files which are:
// 1) Pre-allocated with a length of X KiB
//  The pre-allocation will also erase previous garbages. So, the blocks will be filled with zeroes or ones, deppending on the flash memory vendor.
// 2) Separated in parts. Each part, will have the length of the previous item.
// 3) Each write, have the same length -- maybe for now?
// 4)

// It won't use SdFatSdioEX as it uses multi-block writing, and by doing it, we lose the control of the safety system.

// By having the backup blocks always ahead of the current block instead of a fixed place for them, we distribute the SD


PmmSdSafeLog::PmmSdSafeLog(PmmSd* pmmSd, unsigned defaulBlocksAllocationPerPart)
    : PmmSdAllocation(pmmSd->getSdFatPtr(), defaulBlocksAllocationPerPart)
{
    // These 3 exists as I an confused of which option to use. This must be improved later.
    mPmmSd    = pmmSd;
    mSdFat    = pmmSd->getSdFatPtr();
    mSdioCard = pmmSd->getCardPtr();
}



// READ: The statusStruct must be initSafeLogStatusStruct() before.
// The maximum groupLength is 255, and the code below is written with that in mind, and assuming the block size is 512 bytes.
// On the future it can be improved. Not hard.

// https://stackoverflow.com/questions/31256206/c-memory-alignment
// readBlock() and writeBlock() uses a local array of 512 bytes, and a memcpy() if it is requested.
// When time is available for me, I will write a function in pmmSd for reading and writing without these slowers, and making sure the blockBuffer is aligned to 4.
int PmmSdSafeLog::write(uint8_t data[], char dirFullRelativePath[], pmmSdAllocationStatusStructType* statusStruct, uint8_t externalBlockBuffer[])
{

    // I didn't want to create this variable and keep changing it, and I didn't want to use (PMM_SD_BLOCK_SIZE - statusStruct->currentPositionInBlock),
    // so I will use a define, which certainly I will regret in the future, but now, is the best option I can think.
    // Edit: Actually, now I think it was a very nice decision.
    #define remainingBytesInThisBlock_macro (PMM_SD_BLOCK_SIZE - statusStruct->currentPositionInBlock)

    unsigned dataBytesRemaining = statusStruct->groupLength;
    unsigned hadWrittenGroupHeader = false;

    uint8_t* blockBuffer = (externalBlockBuffer? externalBlockBuffer : mBlockBuffer);

// 1) Is this the first time running for this statusStruct?
    if (statusStruct->currentBlock == 0)
    {
        statusStruct->currentPositionInBlock = 0; // To make sure it starts reseted.
        // The function below will also change some struct member values. Read the corresponding function definition.
        if (allocateFilePart(dirFullRelativePath, PMM_SD_SAFE_LOG_FILENAME_EXTENSION, statusStruct))
        {
            PMM_DEBUG_ADV_PRINTLN("Error at allocateFilePart(), at First Time!");
            return 1;
        }
    }


// 2) Is the current block full?
    //  The behavior of the (statusStruct->currentPositionInBlock > PMM_SD_BLOCK_SIZE) case is probably horrible. It normally won't happen.
    else if (statusStruct->currentPositionInBlock >= PMM_SD_BLOCK_SIZE)
    {
        if (nextBlockAndAllocIfNeeded(dirFullRelativePath, PMM_SD_SAFE_LOG_FILENAME_EXTENSION, statusStruct))
        {
            PMM_DEBUG_ADV_PRINTLN("Error at nextBlockAndAllocIfNeeded(), at Partial Final Data!");
            return 2;
        }
        statusStruct->currentPositionInBlock = 0;
    }


// 3) If there is something in the actual block, we must first copy it to add the new data.
    else if (statusStruct->currentPositionInBlock > 0)
    {
        if (!externalBlockBuffer)   // Only readBlock if no external buffer given.
            mSdioCard->readBlock(statusStruct->currentBlock, blockBuffer); // For some reason, this function returns an error, but works fine. help?

        // We have two possible cases:
        //    4) If the new data needs another block.              -- Partial Initial Data
        //    5) If the new data fits the current block, entirely. -- Entire Data / Partial Final Data


// 4) Now we will see if the data needs another block. If the conditional is true, we need another block!
        if ((statusStruct->groupLength + 2) > remainingBytesInThisBlock_macro) // +2 for header and footer
        {

            // 4.1) We first add the group header, which will always fit the current block (as the currentPositionInBlock goes from 0 to 511, if >=512,
            //  it was treated on 2).)
            blockBuffer[statusStruct->currentPositionInBlock++] = PMM_SD_ALLOCATION_FLAG_GROUP_BEGIN;
            hadWrittenGroupHeader = true;


            // 4.2) Now we add the data to the current block, if there is available space.
            if (remainingBytesInThisBlock_macro) // Avoids useless calls of memcpy
            {
                memcpy(blockBuffer + statusStruct->currentPositionInBlock, data, remainingBytesInThisBlock_macro);
                dataBytesRemaining -= remainingBytesInThisBlock_macro;


                // 4.3) Write the Partial Initial Data to the SD.
                if(!mSdioCard->writeBlock(statusStruct->currentBlock, blockBuffer))
                {
                    PMM_DEBUG_ADV_PRINTLN("Error at writeBlock(), at Partial Initial Data!");
                    return 1;
                }
            }

            statusStruct->currentPositionInBlock = 0;

            // 4.4) As we filled the current block, we need to move to the next one.
            if (nextBlockAndAllocIfNeeded(dirFullRelativePath, PMM_SD_SAFE_LOG_FILENAME_EXTENSION, statusStruct))
            {
                PMM_DEBUG_ADV_PRINTLN("Error at nextBlockAndAllocIfNeeded(), at Partial Initial Data!");
                return 1;
            }

        } // End of 4).
    } // End of 3).


// 5) Write the block that fits entirely the block / Write the Partial Final Data.

    // 5.1) Write the Written Flag if it is a new block.
    if (statusStruct->currentPositionInBlock == 0)
        blockBuffer[statusStruct->currentPositionInBlock++] = PMM_SD_ALLOCATION_FLAG_BLOCK_WRITTEN;


    // 5.2) Write the Group Begin Flag, if not done already at 3.1).
    if (!hadWrittenGroupHeader)
        blockBuffer[statusStruct->currentPositionInBlock++] = PMM_SD_ALLOCATION_FLAG_GROUP_BEGIN;


    // 5.3) Write the Entire Data, or the Partial Final Data. We check if there is still dataBytesRemaining, as the 3)
    // may only needs to write the Group End Flag.
    if (dataBytesRemaining)
    {
        memcpy(blockBuffer + statusStruct->currentPositionInBlock, data + (statusStruct->groupLength - dataBytesRemaining), dataBytesRemaining);

        statusStruct->currentPositionInBlock += dataBytesRemaining;
        dataBytesRemaining = 0;
    }

    // 5.4) Write the Group End Flag.
    blockBuffer[statusStruct->currentPositionInBlock++] = PMM_SD_ALLOCATION_FLAG_GROUP_END;

    // 5.5) Erase any previous garbage after the written data.
    if (remainingBytesInThisBlock_macro)
        memset(blockBuffer + statusStruct->currentPositionInBlock, 0, remainingBytesInThisBlock_macro);

    // 5.6) Write the Last Valid Block to the SD.
    if(!mSdioCard->writeBlock(statusStruct->currentBlock, blockBuffer))
    {
        PMM_DEBUG_ADV_PRINTLN("Error at writeBlock(), at Last Valid Block!");
        return 1;
    }

// 6) Save the current variables of statusStruct, as they will change during the writting of the backup block, and we want to restore
    // these values after it.
    uint32_t tempCurrentBlock = statusStruct->currentBlock;
    uint32_t tempFreeBlocks   = statusStruct->freeBlocksAfterCurrent;
    uint32_t tempNextPart     = statusStruct->nextFilePart;

    // 6.1) We need two blocks for the Backup Block 0 and Backup Block 1. As in this system we first write in the Backup Block 1
    //  and only then on the Backup Block 0, and it's possible that the Backup Block 1 is on another file part,
    //  we will get the address of the Backup Block 0, write on the Backup Block 1, and then, on the Backup Block 0.
    //    It only happens when freeBlocksAfterCurrent < 2, but the procedure is almost the same if it isn't needed.

    // 6.2) Go to the next block, the Backup Block 0.
    if (nextBlockAndAllocIfNeeded(dirFullRelativePath, PMM_SD_SAFE_LOG_FILENAME_EXTENSION, statusStruct))
    {
        PMM_DEBUG_ADV_PRINTLN("Error at nextBlockAndAllocIfNeeded(), at Backup Block 0!");
        return 1;
    }

    // 6.3) Get the address of this block, the Backup Block 0, as the Backup Block 1 may be on another part.
    uint32_t backupBlock0Address = statusStruct->currentBlock;


    // 6.4) Go to the next block, the Backup Block 1.
    if (nextBlockAndAllocIfNeeded(dirFullRelativePath, PMM_SD_SAFE_LOG_FILENAME_EXTENSION, statusStruct))
    {
        PMM_DEBUG_ADV_PRINTLN("Error at nextBlockAndAllocIfNeeded(), at Backup Block 1!");
        return 1;
    }


    // 6.5) Write the Backup Block 1.
    if(!mSdioCard->writeBlock(statusStruct->currentBlock, blockBuffer))
    {
        PMM_DEBUG_ADV_PRINTLN("Error at writeBlock(), at Backup Block 1!");
        return 1;
    }


    // 6.6) Write the Backup Block 0.
    if(!mSdioCard->writeBlock(backupBlock0Address, blockBuffer))
    {
        PMM_DEBUG_ADV_PRINTLN("Error at writeBlock(), at Backup Block 0!");
        return 1;
    }


    // 6.1.7) Return the previous values
    statusStruct->currentBlock           = tempCurrentBlock;
    statusStruct->freeBlocksAfterCurrent = tempFreeBlocks  ;
    statusStruct->nextFilePart           = tempNextPart    ;
    
    return 0;
}



// Returns by reference the number of actual file parts.
// Returns 0 if no error.
int PmmSdSafeLog::getNumberFileParts(char dirFullRelativePath[], uint8_t* fileParts)
{
    return PmmSdAllocation::getNumberFileParts(dirFullRelativePath, PMM_SD_SAFE_LOG_FILENAME_EXTENSION, fileParts);
}

int PmmSdSafeLog::getFileRange(char dirFullRelativePath[], uint8_t filePart, uint32_t *beginBlock, uint32_t *endBlock)
{
    getFilePartName(mTempFilename, dirFullRelativePath, filePart, PMM_SD_SAFE_LOG_FILENAME_EXTENSION);
    return PmmSdAllocation::getFileRange(mTempFilename, beginBlock, endBlock);
}



const char* PmmSdSafeLog::getFilenameExtension()
{
    return PMM_SD_SAFE_LOG_FILENAME_EXTENSION;
}