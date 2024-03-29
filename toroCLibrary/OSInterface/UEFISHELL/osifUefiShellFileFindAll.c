/*++

    toro C Library
    https://github.com/KilianKegel/toro-C-Library#toro-c-library-formerly-known-as-torito-c-library

    Copyright (c) 2017-2022, Kilian Kegel. All rights reserved.
    SPDX-License-Identifier: GNU General Public License v3.0

Module Name:

    _osifUefiShellFileFindAll.c

Abstract:

    OS interface (osif) to find all files in a directory for UEFI Shell

Author:

    Kilian Kegel

--*/
#define OS_EFI
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <io.h>
#undef NULL
#include <uefi.h>
#include <Protocol\Shell.h>
#include <CdeServices.h>
#define UINT64 unsigned long long
#define UINT8 unsigned char
#define CDE_APP_IF void

#define ELC(x) (sizeof(x)/sizeof(x[0]))  // element count
#define MAX_FILE_NAME_LEN 522 // (20 * (6+5+2))+1) unicode characters from EFI FAT spec (doubled for bytes)
#define FIND_XXXXX_FILE_BUFFER_SIZE (SIZE_OF_EFI_FILE_INFO + MAX_FILE_NAME_LEN)

extern const short* _CdeGetCurDir(IN const short* FileSystemMapping);
extern CDESYSTEMVOLUMES gCdeSystemVolumes;

static CDEFILEINFO* __cdeReadDirectory(IN char* strFileName, OUT int* pcntDirEntries);

/**
Synopsis
    #include <CdeServices.h>
    int _osifUefiShellFileFindAll(CDE_APP_IF* pCdeAppIf, const char* filename, CDEFILE* pCdeFile);
Description
    Find all files in a directory
Paramters
    IN CDE_APP_IF* pCdeAppIf    : application interface
    IN char* pstrDrvPthDirStar  : path of search directory followed appended with "\*"
    IN OUT int* pCountOrError   : OUT number
Returns
    0   : success
    EOF : failure
**/
CDEFILEINFO* _osifUefiShellFileFindAll(IN CDE_APP_IF* pCdeAppIf, IN char* pstrDrvPthDirStar, IN OUT int* pCountOrError)
{
    struct _finddata64i32_t** ppFindData = NULL;
    CDEFILEINFO* pCdeFileInfo = NULL;
    bool fIsFileAbs = false/* absolute path vs. relative path */;
    bool fIsFileDrv = false/* a drive name leads the path e.g. FS0: This is identified by precense of ':' */;
    char strDrive[6], strDrive2[6];
    wchar_t wcsDrive2[6];
    int i;

    //
    // non-malloc()'ed pointers
    //
    char* pColon, * pstrCurDir, * pstrCurDir2, * pstrTargetDir, *pDrive, *pCurPath, * pFilePath;

    //
    // malloc()'ed pointers to be freed at _findfirst return , included in freelist[] are initialized with NULL
    //
    wchar_t* pwcsCurDir = NULL, * pwcsCurDir2 = NULL;
    char* pstrFilePath = malloc(320);                                           // can hold 258 bytes max. length of filename
    void* freelist[] = { pwcsCurDir, pstrFilePath };

    CDEMOFINE((MFNINF(1)    ">>> %s\n", pstrDrvPthDirStar));
    
    if (NULL != pCountOrError)
        *pCountOrError = 0;

    do
    {
        //
        // cut trailing "*" from pstrDrvPthDirStar
        //
        //if (0 == strcmp(pstrDrvPthDirStar,"*"))
        //    strcpy(pstrDrvPthDirStar, ".");
        //else
        pstrDrvPthDirStar[strlen(pstrDrvPthDirStar) - 1] = '\0';
        
        //
        //
        //
        fIsFileDrv = (NULL != (pColon = strchr(pstrDrvPthDirStar, ':')));       // drive name presence is identified by ':'
        fIsFileAbs = pstrDrvPthDirStar[0] == '\\' || (fIsFileDrv ? pColon[1] == '\\' : 0);

        pwcsCurDir = (wchar_t*)_CdeGetCurDir(NULL);
        pstrCurDir = (void*)pwcsCurDir;
        wcstombs(pstrCurDir, pwcsCurDir,(size_t)-1);

        if (fIsFileDrv)                                                         // get current directory of remote drive
        {
            strncpy(strDrive2, pstrDrvPthDirStar, &pColon[1] - pstrDrvPthDirStar);

            mbstowcs(wcsDrive2, strDrive2,(size_t)-1);

            //
            // check if drive name is valid
            //
            if (1)
            {
                for (i = 0; i <gCdeSystemVolumes.nVolumeCount; i++)
                {
                    if (0 == _wcsicmp(wcsDrive2,gCdeSystemVolumes.rgFsVolume[i].rgpVolumeMap[0]))
                        break;
                }

                //CDEMOFINE((MFNINF(i ==gCdeSystemVolumes.nVolumeCount) "Drive %ls %s found\n", wcsDrive2, i ==gCdeSystemVolumes.nVolumeCount ? "NOT" : ""));

                if (i ==gCdeSystemVolumes.nVolumeCount)
                    break;  // break with nRet == -1, drive is not available

            }

            pwcsCurDir2 = (wchar_t*)_CdeGetCurDir(wcsDrive2);
            pstrCurDir2 = (void*)pwcsCurDir2;
            wcstombs(pstrCurDir2, pwcsCurDir2,(size_t)-1);
        }

        pstrTargetDir = fIsFileDrv ? (0 == strncmp(pstrDrvPthDirStar, pstrCurDir, &pColon[1] - pstrDrvPthDirStar) ? pstrCurDir : pstrCurDir2) : pstrCurDir;

        pDrive = fIsFileDrv ? pstrDrvPthDirStar : pstrTargetDir;                         // getting real drive name of the file
        pColon = strchr(pDrive, (wchar_t)':');
        strncpy(strDrive, pDrive, &pColon[1] - pDrive);
        strDrive[&pColon[1] - pDrive] = '\0';

        pCurPath = 1 + strchr(pstrTargetDir, ':');

        pFilePath = (fIsFileDrv ? 1 + strchr(pstrDrvPthDirStar, ':') : pstrDrvPthDirStar);

        CDEMOFINE((MFNINF(1)    "pFilePath %p \"%s\"\n", pFilePath, pFilePath));
        CDEMOFINE((MFNINF(1)    "pCurPath  %p \"%s\"\n", pCurPath, pCurPath));
        CDEMOFINE((MFNINF(1)    "fIsFileAbs %d\n", fIsFileAbs));

        if (fIsFileAbs) {                       // if absolute path ...
            strcpy(pstrFilePath, pFilePath);    // ... just copy the entire path\file
        }
        else {                                  // if relative path...
            strcpy(pstrFilePath, pCurPath);     // ... merge...
            strcat(pstrFilePath, "\\");         // ... the absolute ...
            strcat(pstrFilePath, pFilePath);    // ... path
        }
        //
        // create complete drive + path string
        //
        strcpy(pstrDrvPthDirStar, strDrive);
        strcat(pstrDrvPthDirStar, pstrFilePath);

        CDEMOFINE((MFNINF(true) "SEARCH PATH == %s\n", pstrDrvPthDirStar));

        //
        // read the UEFI directory structure and convert to CDEFILEINFO
        //
        pCdeFileInfo = __cdeReadDirectory(pstrDrvPthDirStar, pCountOrError);

    } while (0);

    CDEMOFINE((MFNINF(1)    "<<< pRet %p\n", pCdeFileInfo));

    for (i = 0; i < (int)ELC(freelist); i++)
        free(freelist[i]);

    return pCdeFileInfo;
}

static CDEFILEINFO* __cdeReadDirectory(IN char* strFileName, OUT int* pcntDirEntries)
{
    FILE* fp = fopen(strFileName, "r+b");
    size_t sizeFileInfo = 1024;
    EFI_FILE_INFO* FileInfo = malloc(sizeFileInfo);
    size_t sizeCdeFileInfo = sizeof(CDEFILEINFO);               // this is the initial empty structure
    CDEFILEINFO* pCdeFileInfo = malloc(sizeCdeFileInfo);
    CDEFILEINFO* pCdeFileInfoEnd = NULL;
    CDEFILEINFO* pCdeFileInfoFF = NULL;
    CDEFILEINFO* pRet = NULL;
    int cntDirEntries = 0;                                      // count directory entries
    size_t sizeFileName;

    CDEMOFINE((MFNINF(true) "%p strFileName %s\n", fp, strFileName));

    pCdeFileInfo->time_write = -1LL;

    if (NULL != fp) do
    {
        do {
            CDEMOFINE((MFNFAT(NULL == fp) "fp --> %p\n", fp));

            while (
                sizeFileInfo = 1024,
                sizeFileInfo = fread(FileInfo, 1, sizeFileInfo, fp),
                0 != sizeFileInfo
                )
            {
                //
                // fill "struct tm"-compatible fields
                //
                struct tm timetm = {
                    .tm_sec = FileInfo->ModificationTime.Second,
                    .tm_min = FileInfo->ModificationTime.Minute,
                    .tm_hour = FileInfo->ModificationTime.Hour,
                    .tm_mday = FileInfo->ModificationTime.Day,
                    .tm_mon = FileInfo->ModificationTime.Month,
                    .tm_year = FileInfo->ModificationTime.Year - 1900,
                    .tm_wday = 0,
                    .tm_yday = 0,
                    .tm_isdst = 0
                };
            
                //
                // verify, that data read are truely the "directory" content and not a "file" content
                //
                if (FIND_XXXXX_FILE_BUFFER_SIZE < sizeFileInfo
                    || FileInfo->PhysicalSize % 512
                    || FileInfo->Attribute & ~EFI_FILE_VALID_ATTR
                    )
                {
                    //printf("ERROR: not a directory\n");
                    break;
                }
                fseek(fp, (long)sizeFileInfo, SEEK_SET);

                //CDEMOFINE((MFNINF(true) "---> size %zd, Att:%016zX, Size:%016zX, PhysSize:%016zX %ls\n", sizeFileInfo, FileInfo->Attribute, FileInfo->FileSize, FileInfo->PhysicalSize, FileInfo->FileName));

                cntDirEntries++;

                sizeFileName = 1 + wcslen(FileInfo->FileName);
                sizeCdeFileInfo += sizeFileName;
                sizeCdeFileInfo += sizeof(CDEFILEINFO);

                pCdeFileInfo = realloc(pCdeFileInfo, sizeCdeFileInfo);
                //
                // get pointer to last entry (one before 0xFF/termination structure)
                //
                pCdeFileInfoEnd = (void*)((char*)&pCdeFileInfo[0]
                    + sizeCdeFileInfo
                    - sizeof(CDEFILEINFO)   /*end 0xFF struct */
                    - sizeof(CDEFILEINFO)   /* size of new struct portion*/
                    - sizeFileName)         /* size of the new file name portion*/;
                
                pCdeFileInfoFF = (void*)((char*)&pCdeFileInfo[0]        // get pointer to termination struct
                    + sizeCdeFileInfo
                    - sizeof(CDEFILEINFO))  /*end 0xFF struct */;

                pCdeFileInfoFF->time_write = -1LL;  // write termination signature 0xFFFFFFFFFFFFFFFF (0xFF)

                //
                // fill/copy CdeFileInfo structure elements from EFI_FILE_INFO struct elements
                //
                pCdeFileInfoEnd->attrib = (uint8_t)FileInfo->Attribute;                     // copy attributes
                pCdeFileInfoEnd->size = (_fsize_t)FileInfo->FileSize;                       // copy file size
                wcstombs(&pCdeFileInfoEnd->strFileName[0], FileInfo->FileName,(size_t)-1);  // copy/convert filename
                pCdeFileInfoEnd->time_write = mktime(&timetm);                              // convert EFI_TIME to time_t
 
                if (NULL != pcntDirEntries)
                    *pcntDirEntries++;

                CDEMOFINE((MFNINF(true) "---> size %zd, Att:%016zX, Size:%016zX, time:%016llX, %s\n",
                    pCdeFileInfoEnd->size,
                    pCdeFileInfoEnd->attrib,
                    pCdeFileInfoEnd->strFileName,
                    pCdeFileInfoEnd->time_write,
                    pCdeFileInfoEnd->strFileName));

            }
        } while (0);
    } while (0);
    return pCdeFileInfo;
}