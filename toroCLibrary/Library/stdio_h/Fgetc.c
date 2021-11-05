/*++

    toro C Library
    https://github.com/KilianKegel/toro-C-Library#toro-c-library-formerly-known-as-torito-c-library

    Copyright (c) 2017-2021, Kilian Kegel. All rights reserved.
    SPDX-License-Identifier: GNU General Public License v3.0

Module Name:

    Fgetc.c

Abstract:

    Implementation of the Standard C function.
    Read a character from a stream.

Author:

    Kilian Kegel

--*/
#include <stdio.h>

/** fgetc
Synopsis
    #include <stdio.h>
    int fgetc(FILE *stream);
Description
    https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fgetc-fgetwc?view=msvc-160&viewFallbackFrom=vs-2019
    If the end-of-file indicator for the input stream pointed to by stream is not set and a
    next character is present, the fgetc function obtains that character as an unsigned
    char converted to an int and advances the associated file position indicator for the
    stream (if defined).
Returns
    If the end-of-file indicator for the stream is set, or if the stream is at end-of-file, the endof-
    file indicator for the stream is set and the fgetc function returns EOF. Otherwise, the
    fgetc function returns the next character from the input stream pointed to by stream.
    If a read error occurs, the error indicator for the stream is set and the fgetc function
    returns EOF.

    @param[in] FILE *stream

    @retval character from the input stream.

    @retval EOF on error.

**/
int fgetc(FILE* stream) {
    unsigned int nRet = (size_t)EOF;
    char c;

    do {

        if (1 != fread(&c, 1, 1, stream))
            break;
        nRet = (unsigned char)c;

    } while (0);

    return nRet;
}