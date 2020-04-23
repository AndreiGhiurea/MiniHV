#include "mzpe.h"
#include "vmguest.h"
#include "data.h"
#include "paging_tables.h"

STATUS MzpeFindExport(
    _In_ QWORD ImageBase,
    _In_ CHAR* ExportName,
    _Out_ QWORD* ExportAddress
)
{
    STATUS status;
    IMAGE_DATA_DIRECTORY dir;
    PVOID imageBaseVa = NULL;
    PVOID imageBasePa = NULL;
    BOOLEAN exportFound = FALSE;

    if (0 == ImageBase)
    {
        return STATUS_INVALID_PARAMETER1;
    }

    if (NULL == ExportName)
    {
        return STATUS_INVALID_PARAMETER2;
    }

    if (0 == ExportAddress)
    {
        return STATUS_INVALID_PARAMETER3;
    }

    status = STATUS_SUCCESS;

    status = GuestVAToHostVA(ImageBase, &imageBasePa, &imageBaseVa);
    if (!SUCCEEDED(status))
    {
        LOGL("GuestVAToHostVa failed with status: 0x%x\n", status);
        return STATUS_UNSUCCESSFUL;
    }

    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)imageBaseVa;

    dir = ((IMAGE_NT_HEADERS32*)((BYTE*)dosHeader + dosHeader->e_lfanew))->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];

    if (0 != dir.Size)
    {
        WORD* index = NULL;
        PVOID indexPa = NULL;
        DWORD* exportNamePointerTable = NULL;
        PVOID exportNamePointerTablePa = NULL;
        DWORD* addrOfFuncs = NULL;
        PVOID addrOfFuncsPa = NULL;
        DWORD numberOfPages = 0;

        PIMAGE_EXPORT_DIRECTORY exportDir = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)ImageBase + dir.VirtualAddress);
        PVOID exportDirPa = NULL;

        status = GuestVAToHostVA((QWORD)ImageBase + dir.VirtualAddress, &exportDirPa, &exportDir);
        if (!SUCCEEDED(status))
        {
            LOGL("GuestVAToHostVA failed for address 0x%X with status: 0x%x\n", exportDir, status);
            goto _cleanup_and_exit;
        }

        // DWORD noOfNames = exportDir->NumberOfNames;
        DWORD noOfFunctions = exportDir->NumberOfFunctions;

        numberOfPages = (noOfFunctions * sizeof(DWORD)) / PAGE_SIZE;
        if ((noOfFunctions * sizeof(DWORD)) % PAGE_SIZE)
        {
            numberOfPages++;
        }

        for (int i = numberOfPages - 1; i >= 0; i--)
        {
            exportNamePointerTable = (DWORD*)((BYTE*)ImageBase + exportDir->AddressOfNames + i * PAGE_SIZE);
            status = GuestVAToHostVA((QWORD)exportNamePointerTable, &exportNamePointerTablePa, &exportNamePointerTable);
            if (!SUCCEEDED(status))
            {
                LOGL("GuestVAToHostVA failed for address 0x%X with status: 0x%x\n", exportNamePointerTable, status);
                goto _cleanup_and_exit;
            }

            index = (WORD*)((BYTE*)ImageBase + exportDir->AddressOfNameOrdinals + i * PAGE_SIZE);
            status = GuestVAToHostVA((QWORD)index, &indexPa, &index);
            if (!SUCCEEDED(status))
            {
                LOGL("GuestVAToHostVA failed for address 0x%X with status: 0x%x\n", index, status);
                goto _cleanup_and_exit;
            }

            addrOfFuncs = (DWORD*)((BYTE*)ImageBase + exportDir->AddressOfFunctions + i * PAGE_SIZE);
            status = GuestVAToHostVA((QWORD)addrOfFuncs, &addrOfFuncsPa, &addrOfFuncs);
            if (!SUCCEEDED(status))
            {
                LOGL("GuestVAToHostVA failed for address 0x%X with status: 0x%x\n", addrOfFuncs, status);
                goto _cleanup_and_exit;
            }
        }

        if (NULL == exportNamePointerTable || NULL == index)
        {
            return STATUS_UNSUCCESSFUL;
        }

        for (DWORD j = 0; j < noOfFunctions; ++j)
        {
            CHAR* nameAddr = (CHAR*)((BYTE*)ImageBase + exportNamePointerTable[j]);
            PVOID nameAddrPa = NULL;
            status = GuestVAToHostVA((QWORD)nameAddr, &nameAddrPa, &nameAddr);
            if (!SUCCEEDED(status))
            {
                LOGL("GuestVAToHostVA failed for address 0x%X with status: 0x%x\n", nameAddr, status);
                continue;
            }

            if (0 == strcmp(nameAddr, ExportName))
            {
                *ExportAddress = (QWORD)((BYTE*)ImageBase + addrOfFuncs[index[j]]);
                LOGL("Found %s at address 0x%X\n", ExportName, *ExportAddress);
                exportFound = TRUE;
            }

            if (NULL != nameAddrPa)
            {
                status = UnmapMemory(nameAddrPa, PAGE_SIZE);
                if (!SUCCEEDED(status))
                {
                    LOGL("UnmapMemory failed with status: 0x%x\n", status);
                }
            }

            if (exportFound)
            {
                break;
            }
        }

        _cleanup_and_exit:

        for (DWORD i = 0; i < numberOfPages; i++)
        {
            if (NULL != exportNamePointerTablePa)
            {
                status = UnmapMemory((BYTE*)exportNamePointerTablePa + i * PAGE_SIZE, PAGE_SIZE);
                if (!SUCCEEDED(status))
                {
                    LOGL("UnmapMemory failed with status: 0x%x\n", status);
                }
            }

            if (NULL != indexPa)
            {
                status = UnmapMemory((BYTE*)indexPa + i * PAGE_SIZE, PAGE_SIZE);
                if (!SUCCEEDED(status))
                {
                    LOGL("UnmapMemory failed with status: 0x%x\n", status);
                }
            }

            if (NULL != addrOfFuncsPa)
            {
                status = UnmapMemory((BYTE*)addrOfFuncsPa + i * PAGE_SIZE, PAGE_SIZE);
                if (!SUCCEEDED(status))
                {
                    LOGL("UnmapMemory failed with status: 0x%x\n", status);
                }
            }
        }

        status = UnmapMemory((BYTE*)exportDirPa, PAGE_SIZE);
        if (!SUCCEEDED(status))
        {
            LOGL("UnmapMemory failed with status: 0x%x\n", status);
        }
    }

    if (NULL != imageBasePa)
    {
        status = UnmapMemory(imageBasePa, PAGE_SIZE);
        if (!SUCCEEDED(status))
        {
            LOGL("UnmapMemory failed with status: 0x%x\n", status);
        }
    }

    if (exportFound)
    {
        return STATUS_SUCCESS;
    }

    return STATUS_INTRO_EXPORT_NOT_FOUND;
}

