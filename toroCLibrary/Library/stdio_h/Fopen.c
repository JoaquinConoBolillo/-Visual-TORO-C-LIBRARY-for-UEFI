/*++

    toro C Library
    https://github.com/KilianKegel/toro-C-Library#toro-c-library-formerly-known-as-torito-c-library

    Copyright (c) 2017-2022, Kilian Kegel. All rights reserved.
    SPDX-License-Identifier: GNU General Public License v3.0

Module Name:

    Fopen.c

Abstract:

    Implementation of the Standard C function.
    Opens a file.

Author:

    Kilian Kegel

--*/
//#undef NCDETRACE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CdeServices.h>

extern void* __cdeGetIOBuffer(unsigned i);

/** fopen
Synopsis

    #include <stdio.h>
    FILE *fopen(const char *filename,const char *mode);

Description

    https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fopen-wfopen?view=msvc-160&viewFallbackFrom=vs-2019

Parameters

    https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fopen-wfopen?view=msvc-160#parameters

Returns

    https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fopen-wfopen?view=msvc-160#return-value

**/
FILE* fopen(const char* filename, const char* mode) {

    char rgModeCopy[16], szModeNoSpace[16], * pc = NULL;
    const char szDelims[] = { " \tt" };
    unsigned char TODO = 1;
    CDEFILE* pCdeFile = 0;
    CDE_APP_IF* pCdeAppIf = __cdeGetAppIf();
    int i = 0;;
    wchar_t* pwcsFileName = malloc(sizeof(wchar_t) * (1 + strlen(filename))); // allocate space to expand the filename to wide character

    do {/*1. dowhile(0)*/

        CDETRACE((TRCINF(1) "parm filename -> \"%s\", mode \"%s\"\n", filename, mode));

        //
        // sanity check
        //
        if (NULL == pCdeAppIf->pCdeServices->pFopen) {
            CDETRACE((TRCERR(1) "NULL == pCdeAppIf->pCdeServices->pFopen\n"));
            return NULL;
        }
        //
        // expand filename to wide character
        //
        i = 0;
        while ('\0' != (pwcsFileName[i] = filename[i++]));

        CDETRACE((TRCINF(1) "pwcsFileName -> %S\n", pwcsFileName));

        //
        // ----- remove spaces from mode string, remove t (text), that is not Standard C
        //
        strncpy(rgModeCopy, mode, 15);                                          //copy to rw memory (/Gy[-] separate functions for linker)
        rgModeCopy[15] = '\0';                                                  //set termination

        if (0 == _stricmp(rgModeCopy, "ctrwaxb"))
        {
            strcpy(szModeNoSpace, rgModeCopy);
        }
        else {
            szModeNoSpace[0] = '\0';                                                //init space removed copy of mode
            pc = strtok((void*)&rgModeCopy[0], &szDelims[0]);
            do {
                if (NULL == pc)
                    break;
                strcpy(&szModeNoSpace[strlen(szModeNoSpace)], pc);
                pc = strtok(NULL, &szDelims[0]);
            } while (1);
        }

        CDETRACE((TRCINF(1) "szModeNoSpace: %s\n",szModeNoSpace));

        if (TODO) {
            //TODO: add filename separation into drive, (relative) pathname, filename etc.
            //20160806 assumed, that filename ist filename only
            //wchar_t *pWcsCurDir,*pWcsCurDrv={L"."},*pWcsTmp;
    //
    // ----- find free CdeFile slot
    //

            for (i = 0, pCdeFile = __cdeGetIOBuffer(0)/*&_iob[0]*/; NULL != pCdeFile; pCdeFile = __cdeGetIOBuffer(++i)) {
                if (FALSE == pCdeFile->fRsv) {

                    memset(pCdeFile, 0, sizeof(CDEFILE));

                    pCdeFile->fRsv = TRUE;
                    //  pCdeFile->pwcsFileDrive = pWcsCurDrv;
                    //  //TODO: pCdeFile[i].pwcsFilePath =
                    //  pCdeFile->pRootProtocol = i == CDE4WIN_NA ? NULL : gCdeSystemVolumes.rgFsVolume[i].pRootProtocol;
                    break;
                }
            }

            if (i == pCdeAppIf->cIob) {
                CDETRACE((TRCERR(1) "no free CDEFILE slot found\n"));
                //no free CDEFILE slot found
                //TODO: add error "errno" here
                break;/*1. dowhile(0)*/
            }

            //
            // open the file, for POSIX open()/Microsoft _open() additionally check existance/presence of requested file
            //  NOTE:   The "existance check" is an additional step that is required for POSIX open()/Microsoft _open() support
            //          in UEFI. With that additional step it is easier to satisfy the flag matrix
            //              a) nonexisting files
            //              b) existing files r/w 
            //              c) existing files r/o
            //          The flag matrix contains all combinations of O_CREATE, O_APPEND, O_TRUNC, O_WRONLY and O_RDWR
            //
            CDEFILE CdeFileTmp, * pCdeFileTmp = memset(&CdeFileTmp, 0, sizeof(CDEFILE));

            pCdeFileTmp->emufp = pCdeAppIf->pCdeServices->pFopen(pCdeAppIf, pwcsFileName, "r", 0/*no matter 0 or 1*/, pCdeFileTmp); // check existance of file

            CDETRACE((TRCERR(NULL == pCdeFileTmp->emufp) "NULL == pCdeFileTmp->emufp\n"));

            if (NULL != pCdeFileTmp->emufp)
                pCdeAppIf->pCdeServices->pFclose(pCdeAppIf, pCdeFileTmp);       // instantly close file


            pCdeFile->emufp = pCdeAppIf->pCdeServices->pFopen(
                pCdeAppIf,
                pwcsFileName,
                szModeNoSpace,
                NULL != pCdeFileTmp->emufp,  /* 1 == file present, 0 == file not present*/
                pCdeFile);// get emulation file pointer, that is the Windows FP (CDE4WIN) or pCdeFile or NULL in error case

            CDETRACE((TRCERR(NULL == pCdeFileTmp->emufp) "NULL == pCdeFileTmp->emufp\n"));

            if (pCdeFile->emufp == NULL)
                pCdeFile->fRsv = 0;
        }

    } while (0)/*1. dowhile(0)*/;

    free(pwcsFileName);

    return pCdeFile->emufp == NULL ? NULL : (FILE*)pCdeFile;
}