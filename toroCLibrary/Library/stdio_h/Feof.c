/*++

    toro C Library
    https://github.com/KilianKegel/toro-C-Library#toro-c-library-formerly-known-as-torito-c-library

    Copyright (c) 2017-2021, Kilian Kegel. All rights reserved.
    SPDX-License-Identifier: GNU General Public License v3.0

Module Name:

    Feof.c

Abstract:

    Implementation of the Standard C function.
    Tests for end-of-file on a stream.

Author:

    Kilian Kegel

--*/
#include <stdio.h>
#include <CdeServices.h>

extern unsigned char __cdeIsFilePointer(void* stream);

/**

Synopsis
    #include <stdio.h>
    int feof(FILE *stream);
Description
    https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/feof?view=msvc-160&viewFallbackFrom=vs-2019
    The feof function tests the end-of-file indicator for the stream pointed to by stream.
Returns
    The feof function returns nonzero if and only if the end-of-file indicator is set for
    stream.

    @param[in] FILE *stream

    @retval EOF ? 1 : 0

**/
int feof(FILE* stream) {

    CDEFILE* pCdeFile = (CDEFILE*)stream;

    return __cdeIsFilePointer(pCdeFile) ? (int)pCdeFile->fEof : 0;
}
