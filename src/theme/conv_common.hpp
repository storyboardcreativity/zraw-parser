#pragma once

#include "Resource.hpp"

// color palette
#define LIGHT        nana::color(138, 138, 138)
#define WHITE        nana::color(255, 255, 255)
#define BLACK        nana::color(0, 0, 0)
#define HIGHLIGHT    nana::color(186, 186, 186)

// font

#define FONT_SIZE    9
#define FONT_NAME                "Segoe UI"

#define FONT0_SIZE   9
#define FONT0_NAME               "Consolas"

// button image

Resource g_res_vsbuttons(IDR_BINARY7, L"BINARY");

// icon

Resource g_res_icon(IDR_BINARY6, L"BINARY");

// logo

Resource g_res_about_logo(IDR_BINARY5, L"BINARY");

//

#define COLOR0        {45, 45, 48, 1}            // Facade background
#define COLOR0_NANA    nana::color(45, 45, 48, 1)

#define COLOR1        {241, 241, 241, 1}        // Font
#define COLOR1_NANA    nana::color(241, 241, 241, 1)

#define COLOR2        {202, 81, 0, 1}            // Debug (orange)
#define COLOR2_NANA    nana::color(202, 81, 0, 1)

#define COLOR3        {0, 122, 204, 1}        // Normal (blue)
#define COLOR3_NANA    nana::color(0, 122, 204, 1)

#define COLOR4        {37, 37, 38, 1}            // Textbox background
#define COLOR4_NANA    nana::color(37, 37, 38, 1)

#define COLOR5        {28, 151, 234, 1}        // Highlighted (blue)
#define COLOR5_NANA    nana::color(28, 151, 234, 1)

#define COLOR6        {153, 153, 153, 1}        // Font (form title, active)
#define COLOR6_NANA    nana::color(153, 153, 153, 1)

#define COLOR7        {208, 208, 208, 1}        // Font (secondary, darker)
#define COLOR7_NANA    nana::color(208, 208, 208, 1)

#define COLOR8        {230, 49, 33, 1}        // Red (failed)
#define COLOR8_NANA    nana::color(230, 49, 33, 1)

#define COLOR9        {141, 210, 138, 1}        // Green (ok)
#define COLOR9_NANA    nana::color(141, 210, 138, 1)

#define COLOR10        {63, 63, 70, 1}            // Listbox item selected
#define COLOR10_NANA    nana::color(63, 63, 70, 1)

#define COLOR11        {30, 30, 30, 1}            // Textbox background (darker)
#define COLOR11_NANA    nana::color(30, 30, 30, 1)

#define COLOR_PR0    {90, 126, 143, 1}        // Progressbar fgcolor
#define COLOR_PR0_NANA    nana::color(90, 126, 143, 1)

#define COLOR_PR1    {117, 170, 196, 1}        // Progressbar fgcolor (border)
#define COLOR_PR1_NANA    nana::color(117, 170, 196, 1)

#define COLOR12        {117, 170, 196, 1}        // Font (listbox category header)
#define COLOR12_NANA    nana::color(117, 170, 196, 1)
