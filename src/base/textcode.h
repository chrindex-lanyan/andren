#pragma once

#include <stdint.h>
#include <wchar.h>
#include <string.h>
#include <stddef.h>
#include <uchar.h>

#ifdef __cplusplus 
extern "C"{
#endif


/// UTF8 <=> UTF16

extern int TextUtf8ToUtf16(char const * utf8Str, size_t size_1 , char16_t * utf16Str , size_t size_2 );

extern int TextUtf16ToUtf8(char16_t const * utf16Str, size_t size_1 , char * utf8Str , size_t size_2 );

/// UTF8 <=> UTF32

extern int TextUtf8ToUtf32(char const * utf8Str, size_t size_1 , char32_t * utf32Str , size_t size_2 );

extern int TextUtf32ToUtf8(char32_t const * utf32Str, size_t size_1 , char * utf8Str , size_t size_2 );

/// UTF16 <=> GBK

extern int TextUtf16ToGBK(char16_t const * utf16Str, size_t size_1 , char * gbkStr , size_t size_2 );

extern int TextGBKToUtf16(char const * gbkStr, size_t size_1 , char16_t * utf16Str , size_t size_2 );



#ifdef __cplusplus 
}
#endif
