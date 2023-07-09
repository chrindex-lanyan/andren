
#pragma once

#include <string>
#include <string_view>

static_assert(__cplusplus >= 202002L);

extern std::string base64_encode     (std::string const& s, bool url = false);
extern std::string base64_encode_pem (std::string const& s);
extern std::string base64_encode_mime(std::string const& s);

extern std::string base64_decode(std::string const& s, bool remove_linebreaks = false);
extern std::string base64_encode(unsigned char const*, size_t len, bool url = false);

extern std::string base64_encode     (std::string_view s, bool url = false);
extern std::string base64_encode_pem (std::string_view s);
extern std::string base64_encode_mime(std::string_view s);

extern std::string base64_decode(std::string_view s, bool remove_linebreaks = false);


