/*++

    toro C Library
    https://github.com/KilianKegel/toro-C-Library#toro-c-library-formerly-known-as-torito-c-library

    Copyright (c) 2017-2021, Kilian Kegel. All rights reserved.
    SPDX-License-Identifier: GNU General Public License v3.0

Module Name:

    osifUefiDxeEntryPoint.c

Abstract:

    CRT0 for UEFI DXE in a non-EDK build environment

Author:

    Kilian Kegel

--*/
//
// block the original tianocore/UEFI DriverEntryPoint.c
//
#define _DONT_USE_EDK2_MODULEENTRYPOINT_OVERRIDE_   // block _cdeCRT0UefiDxeEDK() from beeing included
#define _DEBUG_LIB_C_H_                             // block DebugLib.c from beeing included
#define _DRIVER_ENTRY_POINT_C_H_                    // block DriverEntryPoint.c from beeing included
//
// include the entire .C file but with _cdeCRT0UefiDxeEDK() and _cdeCRT0UefiDxeEDKINT3() disabled by switches above
//
#include "osifUefiDxeEntryPointEDK.c"

EFI_HANDLE               _cdegImageHandle;
EFI_SYSTEM_TABLE*        _cdegST;
EFI_BOOT_SERVICES*       _cdegBS;
EFI_RUNTIME_SERVICES*    _cdegRT;

/** _cdeCRT0UefiDxe() - CRT0 for DXE in a non-EDK build environment

Synopsis
    #include <CdeServices.h>
    EFI_STATUS EFIAPI _cdeCRT0UefiDxe(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE* SystemTable);
Description
    C runtime initialization.
Parameters
    IN EFI_HANDLE        ImageHandle
    IN EFI_SYSTEM_TABLE* SystemTable
Returns
    Status
**/
EFI_STATUS EFIAPI _cdeCRT0UefiDxe/*for non-EDK build environment*/(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE* SystemTable)
{
    //
    // initialize EDK global variables USUALLY provided by UefiBootServicesTableLib.lib and UefiRuntimeServicesTableLib.lib
    //
    _cdegImageHandle = ImageHandle;
    _cdegST = SystemTable;
    _cdegBS = SystemTable->BootServices;
    _cdegRT = SystemTable->RuntimeServices;

    EFI_STATUS Status = EFI_SUCCESS;
    do {

        //
        // get the CdeServices
        //
        Status = SystemTable->BootServices->LocateProtocol(&_gCdeDxeProtocolGuid, NULL, &CdeAppIfDxe.pCdeServices);
        if (EFI_SUCCESS != Status)
        {
            __cdeFatalCdeServicesNotAvailDXE(SystemTable);
            break;//do{}while(0);
        }

        CdeAppIfDxe.DriverParm.BsDriverParm.ImageHandle = ImageHandle;
        CdeAppIfDxe.DriverParm.BsDriverParm.pSystemTable = SystemTable;

    } while (0);

    return _MainEntryPointDxe(ImageHandle, SystemTable);
}