#pragma once

#define ZRAW_PARSER_VERSION_MAJOR 0
#define ZRAW_PARSER_VERSION_MINOR 7
#define ZRAW_PARSER_VERSION_PATCH 1

#define __ZRAW_STR_HELPER(x) #x
#define __ZRAW_STR(x) __ZRAW_STR_HELPER(x)

#define ZRAW_PARSER_VERSION_STRING __ZRAW_STR(ZRAW_PARSER_VERSION_MAJOR)"."__ZRAW_STR(ZRAW_PARSER_VERSION_MINOR)"."__ZRAW_STR(ZRAW_PARSER_VERSION_PATCH)" alpha"