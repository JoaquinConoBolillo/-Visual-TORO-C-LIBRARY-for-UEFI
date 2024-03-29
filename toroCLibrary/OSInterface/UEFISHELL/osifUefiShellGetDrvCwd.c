/*++

    toro C Library
    https://github.com/KilianKegel/toro-C-Library#toro-c-library-formerly-known-as-torito-c-library

    Copyright (c) 2017-2022, Kilian Kegel. All rights reserved.
    SPDX-License-Identifier: GNU General Public License v3.0

Module Name:

    _osifUefiShellGetDrvCwd.c

Abstract:

    OS interface (osif) get current working directory UEFI Shell

Author:

    Kilian Kegel

--*/
#define OS_EFI
#include <Base.h>
#include <CdeServices.h>
#include <Protocol\Shell.h>
#include <wchar.h>

extern EFI_SHELL_PROTOCOL* pEfiShellProtocol;
extern CDESYSTEMVOLUMES gCdeSystemVolumes;

/**

Synopsis
    #include <CdeServices.h>
    char* _osifUefiShellGetDrvCwd(IN CDE_APP_IF* pCdeAppIf, IN OUT char* pstrDrvCwdBuf);
Description
    Write file
Paramters
    IN CDE_APP_IF* pCdeAppIf    : application interface
    IN OUT char* pstrDrvCwdBuf  : buffer to hold directory name as narrow string
Returns
    SUCCESS: pointer to current working directory
    FAILURE: NULL

**/
char* _osifUefiShellGetDrvCwd(IN CDE_APP_IF* pCdeAppIf, IN OUT char* pstrDrvCwdBuf)
{
    wchar_t* pwcsPool;
    char* pRet = NULL;
    char*    pstrIntern = (void*)&pCdeAppIf->szTmpBuf[0];                   // access buffer via char* type
    wchar_t* pwcsIntern = (void*)&pCdeAppIf->szTmpBuf[0];                   // access buffer via wchar_t* type
    wchar_t wc;
    int i = 0;

    do
    {
        //pwcsPool = (wchar_t*)pEfiShellProtocol->GetCurDir(NULL == pstrDrvCwdBuf ? NULL : (const wchar_t*) &wcsDrive[0]);
        
        pwcsPool = (wchar_t*)pEfiShellProtocol->GetCurDir(NULL);

        CDEMOFINE((MFNINF(1) "pwcsPool %p\n", pwcsPool));

        if (NULL != pwcsPool)
        {
            #if CDE_DIRECTORYNAME_LEN > CDE_TMPBUF_WCHAR_LEN
                #error CDE_TMPBUF_WCHAR_LEN TOO SMALL
            #endif// CDE_DIRECTORYNAME_LEN > CDE_TMPBUF_WCHAR_LEN
            
            //wcsncpy(pwcsIntern, pwcsPool, CDE_DIRECTORYNAME_LEN);
            //pwcsIntern[CDE_DIRECTORYNAME_LEN] = L'\0';

            //while ( wc = pwcsIntern[i], 
            //        pstrIntern[i++] = (char)wc, 
            //        L'\0' != wc
            //    );
            //
            //pRet = pstrIntern;
            while ( wc = pwcsPool[i],
                pstrDrvCwdBuf[i++] = (char)wc,
                    L'\0' != wc
                );
            
            pRet = pstrDrvCwdBuf;

    }

    } while (0);

    return pRet;
}