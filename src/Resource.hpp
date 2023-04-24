#pragma once

#include <string>

#ifdef _WIN32
    #include <Windows.h>
    #include "../vc2017/resource.h"
#elif __APPLE__
    #include <fstream>
    #include <mach-o/dyld.h>
#else
    #error !!! resources for Linux are not implemented yet!
#endif

class Resource
{
public:
    struct Parameters
    {
        std::size_t size_bytes = 0;
        void* ptr = nullptr;
    };

private:

#ifdef _WIN32
    HRSRC hResource = nullptr;
    HGLOBAL hMemory = nullptr;
#endif

    Parameters p;

public:
#ifdef _WIN32
    Resource(int resource_id, const std::wstring &resource_class, bool is_font = false)
    {
        hResource = FindResource(nullptr, MAKEINTRESOURCEW(resource_id), resource_class.c_str());
        hMemory = LoadResource(nullptr, hResource);

        p.size_bytes = SizeofResource(nullptr, hResource);
        p.ptr = LockResource(hMemory);

        if (is_font)
        {
            DWORD nFonts;
            ::AddFontMemResourceEx(p.ptr, p.size_bytes, NULL, &nFonts);
        }
    }
#elif __APPLE__
    Resource(int resource_id, const std::string relative_path, bool is_font = false)
    {
        char buf [PATH_MAX];
        uint32_t bufsize = PATH_MAX;
        if(_NSGetExecutablePath(buf, &bufsize))
            return;
            
        auto path = std::string(buf);
        path = path.substr(0, path.find_last_of("\\/"));

        std::ifstream input(path + "/" + relative_path, std::ios::binary);
        std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(input), {});

        p.size_bytes = buffer.size();
        p.ptr = new uint8_t[p.size_bytes];

        memcpy(p.ptr, buffer.data(), buffer.size());
    }
#else
    #error !!! resources for Linux are not implemented yet!
#endif
    
    Parameters Data()
    {
        return p;
    }
};