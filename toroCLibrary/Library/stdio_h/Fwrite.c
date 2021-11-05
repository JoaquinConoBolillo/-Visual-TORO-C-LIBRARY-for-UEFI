/*++

    toro C Library
    https://github.com/KilianKegel/toro-C-Library#toro-c-library-formerly-known-as-torito-c-library

    Copyright (c) 2017-2021, Kilian Kegel. All rights reserved.
    SPDX-License-Identifier: GNU General Public License v3.0

Module Name:

    Fwrite.c

Abstract:

    Implementation of the Standard C function.
    Writes data to a stream.
    CTRL-Z, and text vs. binary mode is supported to be Microsoft C Library compatible

Author:

    Kilian Kegel

--*/
#include <uefi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <CdeServices.h>

extern unsigned char __cdeIsFilePointer(void* stream);

/**
Synopsis

    #include <stdio.h
    size_t fwrite(const void* ptr, size_t size, size_t nelem, FILE* stream);
Description
    https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fwrite?view=msvc-160&viewFallbackFrom=vs-2019
    The fwrite function writes, from the array pointed to by ptr, up to nmemb elements
    whose size is specified by size, to the stream pointed to by stream. For each object,
    size calls are made to the fputc function, taking the values (in order) from an array of
    unsigned char exactly overlaying the object. The file position indicator for the
    stream (if defined) is advanced by the number of characters successfully written. If an
    error occurs, the resulting value of the file position indicator for the stream is
    indeterminate.
Paramters
    https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fwrite?view=msvc-160#parameters
Returns
    https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fwrite?view=msvc-160#return-value
    The fwrite function returns the number of elements successfully written, which will be
    less than nmemb only if a write error is encountered. If size or nmemb is zero,
    fwrite returns zero and the state of the stream remains unchanged

**/
size_t fwrite(const void* ptr, size_t size, size_t nelem, FILE* stream) {
    CDEFILE* pCdeFile = (CDEFILE*)stream;
    size_t nRet = 0, requested, provided;
    size_t lastnum;
    CDE_APP_IF* pCdeAppIf = __cdeGetAppIf();
    unsigned char fposoosync = TRUE; //file position out of sync
    unsigned char flushbuf = ptr == NULL && size == (size_t)EOF && nelem == 0;
    unsigned char fCRinserted;/*carriage return*/

//    CDEMOFINE((MFNINF(1) "ptr %p, size %d,nelem %d, fp %p\n",ptr,(int)size,(int)nelem,stream));

    if (__cdeIsFilePointer(stream))
    {
        //fpos_t fpos = pCdeFile->bpos; //memorize file position that is destroyed by a possible fwrite(NULL,EOF,0,stream)

        if (O_APPEND == (pCdeFile->openmode & O_APPEND)) {
            //CDEMOFINE((MFNINF(1) "append\n"));
            pCdeFile->bpos = CDE_FPOS_SEEKEND; // force END OF FILE position
            pCdeFile->fEof = TRUE;
        }

        if (NULL == pCdeFile->Buffer) {
            pCdeFile->Buffer = malloc(BUFSIZ);
            pCdeFile->bsiz = BUFSIZ;
        }

        for (provided = 0, requested = size * nelem, lastnum = 0, fCRinserted = FALSE; (flushbuf || provided < requested); flushbuf = FALSE)
        {

            for (/* provided = 0 , requested = size * nelem */; provided < requested && pCdeFile->bidx < pCdeFile->bsiz; /* do nothing */)
            {
                pCdeFile->Buffer[pCdeFile->bidx] = ((char*)ptr)[provided];
                pCdeFile->bidx++;
                //
                // NOTE: If the buffer of a readonly-file is written, pCdeFile->bvld must not be updated
                //
                if (O_RDONLY != (pCdeFile->openmode & O_RDWRMSK)) {
                    pCdeFile->bvld++;   //??? += pCdeFile->bvld < pCdeFile->bsiz ? 1 : 0;
                }
                pCdeFile->bdirty = TRUE;

                if (pCdeFile->openmode & O_TEXT) {
                    if (('\n' == pCdeFile->Buffer[pCdeFile->bidx - 1]) && (FALSE == fCRinserted)) {
                        pCdeFile->Buffer[pCdeFile->bidx - 1] = '\r';
                        fCRinserted = TRUE;
                        continue;
                    }
                }

                fCRinserted = FALSE;

                provided++;
            }

            if (O_RDONLY == (pCdeFile->openmode & O_RDWRMSK)) {
                //
                // NOTE: If the buffer of a readonly-file is written, it only fails if:
                //
                if (pCdeFile->bidx >= pCdeFile->bvld || (pCdeFile->bdirty && !pCdeFile->bclean)/*pCdeFile->bidx == 1*/
                /*
                    if (bdirty AND NOT bclean) , it was 0 at the beginning == buffer was empty == writing to a empty
                    read only buffer == no one did fgetc() before to be ungetc()
                */) {
                //DONT ADD ERROR TO ERRNO
                //CDEMOFINE((MFNINF(1) "write protection\n"));
                    pCdeFile->fErr = TRUE;      // set the error flag
                    provided = 0;               // return 0 bytes written
                    break;                      // 
                }
            }

            if ((flushbuf || pCdeFile->bidx >= pCdeFile->bsiz) && pCdeFile->bvld != 0/*don't write 0 bytes*/)
            {
                if (fposoosync) {
                    pCdeAppIf->pCdeServices->pFsetpos(pCdeAppIf, pCdeFile, &pCdeFile->bpos);
                    fposoosync = FALSE;
                }

                lastnum = pCdeFile->bclean ? pCdeFile->bvld : pCdeAppIf->pCdeServices->pFwrite(pCdeAppIf, pCdeFile->Buffer, pCdeFile->bvld, pCdeFile); // don't write if buffer is clean (already read)
                if (0) {
                    int loci;
                    for (loci = 0; loci < pCdeFile->bsiz; loci++)pCdeFile->Buffer[loci] = 0xFF;
                }

                //CDEFAULT((CDEFINE_ERROR lastnum == 0, "%s\n",strefierror(efierrno)));

                //CDEASSERT(lastnum != 0);
                //TODO:
                // errorhandling, ftell etc
                pCdeFile->bpos += lastnum;
                pCdeFile->bidx = 0;
                pCdeFile->bvld = 0;//lastnum;
                pCdeFile->bdirty = FALSE;
                pCdeFile->bclean = FALSE;

            }
        }
        nRet = provided / (size == 0 ? 1 : size/*don't divide by zero*/);
        
        //
        // speed up buffer flushing, skip for redirected stdout/stderr
        //
        if (nRet) 
            if ((stream == (FILE*)CDE_STDOUT) || (stream == (FILE*)CDE_STDERR))
                if (0 == (O_CDEREDIR & pCdeFile->openmode)) // skip flushing if file is redirected to speed up
                    fwrite(NULL, (size_t)EOF, 0, stream);   // flush the write buffer on each byte
    }

    return nRet;
}