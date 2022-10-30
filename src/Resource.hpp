#pragma once

#include <string>

#include <Windows.h>

#include "../vc2017/resource.h"

class Resource
{
public:
    struct Parameters
    {
        std::size_t size_bytes = 0;
        void* ptr = nullptr;
    };

private:
    HRSRC hResource = nullptr;
    HGLOBAL hMemory = nullptr;

    Parameters p;

public:
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

    Parameters Data()
    {
        return p;
    }
};