/* pmmSd.cpp
 *
 * By Henrique Bruno Fantauzzi de Almeida (aka SrBrahma) - Minerva Rockets, UFRJ, Rio de Janeiro - Brazil */

#include <SdFat.h>
#include "pmmConsts.h"                      // For this system name
#include "pmmDebug.h"
#include "pmmSd/pmmSdGeneralFunctions.h"
#include "pmmSd/pmmSd.h"



PmmSd::PmmSd()
    : mSafeLog(getSdFatPtr(), getCardPtr()),
      mSplit(getSdFatPtr())
{
}

int PmmSd::init()
{
    // 1) Initialize the SD
    if (!mSdFat.begin())
    {
        mSdIsWorking = 0;
        PMM_DEBUG_ADV_PRINTLN("Error at mSdFat.begin()!");
        return 1;
    }

    else
    {
        mSdIsWorking = 1;
        PMM_SD_DEBUG_PRINT_MORE("PmmSd: [M] Initialized successfully.");
        return 0;
    }
    mHasCreatedThisSessionDirectory = 0;
}

int PmmSd::init(uint8_t sessionId)
{
    if (init())
    {
        PMM_DEBUG_ADV_PRINTLN("Error at init()!")
        return 1;
    }

    mThisSessionId = sessionId;
    
    if (setPmmCurrentDirectory())
    {
        PMM_DEBUG_ADV_PRINTLN("Error at setPmmCurrentDirectory()!")
        return 2;
    }

    return 0;

}



// Will rename, if exists, a previous Session with the same name, if this is the first time running this function in this Session.
int PmmSd::setPmmCurrentDirectory()
{
    char fullPath[PMM_SD_FILENAME_MAX_LENGTH];
    char fullPathRenameOld[PMM_SD_FILENAME_MAX_LENGTH];
    unsigned oldCounter = 0;

    snprintf(fullPath, PMM_SD_FILENAME_MAX_LENGTH, "%s/%s/Session %02u", PMM_SD_BASE_DIRECTORY, PMM_THIS_NAME_DEFINE, mThisSessionId);

    // 1) Make sure we are at root dir
    if (!mSdFat.chdir())
    {
        PMM_DEBUG_ADV_PRINTLN("Error at chdir() to root!")
        return 1;
    }

    // 2) If is the first time running this function and there is an old dir with the same name as the new one about to be created,
    //    rename the old one.
    if (mSdFat.exists(fullPath))
    {
        if (!mHasCreatedThisSessionDirectory)
        {
            do
            {
                snprintf(fullPathRenameOld, PMM_SD_FILENAME_MAX_LENGTH, "%s/%s/Session %02u old %02u", PMM_SD_BASE_DIRECTORY, PMM_THIS_NAME_DEFINE, mThisSessionId, oldCounter);
                oldCounter++;
            } while (mSdFat.exists(fullPathRenameOld));

            if (!mSdFat.rename(fullPath ,fullPathRenameOld))
            {
                PMM_DEBUG_ADV_PRINTLN("Error at rename()!")
                return 2;
            }
            mHasCreatedThisSessionDirectory = 1;
        }
    }
    
    // 3) Create the new dir
    else if (!mSdFat.mkdir(fullPath))
    {
        PMM_DEBUG_ADV_PRINTLN("Error at mkdir()!")
        return 3;
    }

    // 4) Change to it
    if (!mSdFat.chdir(fullPath))
    {
        PMM_DEBUG_ADV_PRINTLN("Error at chdir() to fullPath!")
        return 4;
    }

    return 0;
}

int PmmSd::setCurrentDirectory(char fullPath[])
{
    if (!fullPath)  // Null address
        return 1;

    mSdFat.chdir();
    mSdFat.mkdir(fullPath);
    mSdFat.chdir(fullPath);
    return 0;
}

int PmmSd::removeDirRecursively(char relativePath[])
{
    // open the dir
    if (mSdFat.exists(relativePath))
    {
        mFile.open(relativePath, O_RDWR);
            //return 1;

        mFile.rmRfStar();
           // return 2;

        mFile.close();
           // return 3;
    }

    return 0;
}


// The functions below are mostly just a call for the original function. But you won't need to use the File variable directly.
// For using it directly (aka using a function not listed here), you can just getFile() from this class.

bool PmmSd::exists(char path[])
{
    return mSdFat.exists(path);
}

// The default open, but this will also automatically create the path.
int PmmSd::createDirsAndOpen(char path[], uint8_t mode)
{
    return ::createDirsAndOpen(&mSdFat, &mFile, path, mode); // https://stackoverflow.com/a/1061630/10247962
}

int PmmSd::open(char path[], uint8_t mode)
{
    return mFile.open(path, mode);
}
int PmmSd::seek(uint32_t position)
{
    return mFile.seek(position);
}
uint32_t PmmSd::size()
{
    return mFile.size();
}
int PmmSd::read(uint8_t buffer[], size_t numberBytes)
{
    if (!buffer)
        return 1;

    return mFile.read(buffer, numberBytes);
}
int PmmSd::write(uint8_t byte)
{
    return mFile.write(byte);
}
int PmmSd::write(uint8_t arrayToWrite[], size_t length)
{
    return mFile.write(arrayToWrite, length);
}
int PmmSd::close()
{
    return mFile.close();
}



int PmmSd::getSelfDirectory(char destination[], uint8_t maxLength, char additionalPath[])
{
    if (!destination)
        return 1;

    // Left expression is always evaluated first! https://stackoverflow.com/a/2456415/10247962
    if (!additionalPath || additionalPath[0] == '\0')
        snprintf(destination, maxLength, "%s", PMM_SD_DIRECTORY_SELF);

    else
        snprintf(destination, maxLength, "%s/%s", PMM_SD_DIRECTORY_SELF, additionalPath);
    
    return 0;
}



int PmmSd::getReceivedDirectory(char destination[], uint8_t maxLength, uint8_t sourceAddress, uint8_t sourceSession, char additionalPath[])
{
    if (!destination)
        return 1;

    // Left expression is always evaluated first! https://stackoverflow.com/a/2456415/10247962
    if (!additionalPath || additionalPath[0] == '\0')
        snprintf(destination, maxLength, "%03u/Session %02u", sourceAddress, sourceSession);

    else
        snprintf(destination, maxLength, "%03u/Session %02u/%s", sourceAddress, sourceSession, additionalPath);
    
    return 0;
}



SdioCard*     PmmSd::getCardPtr()
{
    return mSdFat.card();
}
SdFatSdio*    PmmSd::getSdFatPtr()
{
    return &mSdFat;
}
PmmSdSafeLog* PmmSd::getSafeLog()
{
    return &mSafeLog;
}
PmmSdSplit*   PmmSd::getSplit()
{
    return &mSplit;
}
File*         PmmSd::getFile()
{
    return &mFile;
}

bool PmmSd::getSdIsBusy()
{
    return mSdFat.card()->isBusy();
}