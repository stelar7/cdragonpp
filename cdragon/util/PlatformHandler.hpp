#pragma once
#include <string>
#include <fstream>
#include <cstring>


std::string map_file_to_string(std::string& file);


#define WINDOWS 1
#if WINDOWS
#include "windows.h"

std::string map_file_to_string(std::string& file)
{
    SYSTEM_INFO sysinfo = { 0 };
    GetSystemInfo(&sysinfo);
    std::int32_t maxAllocationSize = sysinfo.dwAllocationGranularity;

    wchar_t* buff = new wchar_t[1024];
    MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, file.c_str(), -1, buff, 1024);
    auto file_handle = CreateFile(buff, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (file_handle == INVALID_HANDLE_VALUE)
    {
        std::cout << GetLastError() << std::endl;
    }

    LARGE_INTEGER sizeHandle;
    GetFileSizeEx(file_handle, &sizeHandle);
    std::int64_t filesize = sizeHandle.QuadPart;

    char* content = new char[static_cast<std::int32_t>(filesize)];

    auto file_mapping = CreateFileMapping(file_handle, 0, PAGE_READONLY, 0, 0, 0);
    if (file_mapping == 0)
    {
        std::cout << GetLastError() << std::endl;
    }

    for (std::int64_t offset = 0; offset < filesize; offset += maxAllocationSize) {
        std::int32_t high = static_cast<std::int32_t>((offset >> 32) & 0xFFFFFFFFul);
        std::int32_t low = static_cast<std::int32_t>(offset & 0xFFFFFFFFul);

        if (offset + maxAllocationSize > filesize)
        {
            maxAllocationSize = static_cast<std::int32_t>(filesize - offset);
        }

        auto memory_address = MapViewOfFile(file_mapping, FILE_MAP_READ, high, low, maxAllocationSize);
        if (memory_address == 0)
        {
            std::cout << GetLastError() << std::endl;
        }

        memcpy(content + offset, memory_address, maxAllocationSize);
    }

    std::string return_value(content, static_cast<std::int32_t>(filesize));


    CloseHandle(file_handle);
    delete buff;
    delete content;

    return return_value;
}


#elif UNIX


#endif
