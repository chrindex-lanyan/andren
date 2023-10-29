
#include "textcode.h"

#include <stdint.h>
#include <string.h>

/// UTF8 <=> UTF16

int TextUtf8ToUtf16(char const *utf8Str, size_t size_1, char16_t *utf16Str, size_t size_2)
{
    if (size_1 / (sizeof(char16_t)/sizeof(char)) > size_2)
    {
        return -1;
    }

    return 0;
}

int TextUtf16ToUtf8(char16_t const *utf16Str, size_t size_1, char *utf8Str, size_t size_2)
{
    if (size_1 * sizeof(char16_t) > size_2)
    {
        return -1;
    }
    return 0;
}

/// UTF8 <=> UTF32

int TextUtf8ToUtf32(char const *utf8Str, size_t size_1, char32_t *utf32Str, size_t size_2)
{
    if (size_1 / (sizeof(char32_t)/sizeof(char)) > size_2)
    {
        return -1;
    }
    return 0;
}

int TextUtf32ToUtf8(char32_t const *utf32Str, size_t size_1, char *utf8Str, size_t size_2)
{
    if (size_1 * sizeof(char32_t) > size_2)
    {
        return -1;
    }
    return 0;
}

/// UTF16 <=> GBK

int TextUtf16ToGBK(char16_t const *utf16Str, size_t size_1, char *gbkStr, size_t size_2)
{
    if (size_1 * (sizeof(char16_t) / sizeof(char)) > size_2)
    {
        return -1;
    }

    return 0;
}

int TextGBKToUtf16(char const *gbkStr, size_t size_1, char16_t *utf16Str, size_t size_2)
{
    if (size_1 / sizeof(char16_t) > size_2)
    {
        return -1;
    }

    return 0;
}