﻿#pragma once

#include <stdint.h>
#include <string>

#include "noncopyable.hpp"

namespace chrindex ::andren::base
{
    class ZStreamCompressPrivate;
    class ZStreamDecompressPrivate;

    class ZStreamCompress : public noncopyable
    {
    public :
        ZStreamCompress(uint32_t chunkSize = 4);
        ~ZStreamCompress();
        ZStreamCompress(ZStreamCompress&& _);
        ZStreamCompress & operator=(ZStreamCompress&& _);

        /// @brief z_stream is valid or not.
        /// @return  true => valid 
        bool isValid() const ;

        /// @brief finish compress and return data
        /// @return compress data
        std::string finished();
        
        /// @brief append data in internal buffer to compress.
        /// @param data user data
        /// @return ok => zero ; valid z_stream => -1 ; data error => -2
        int compress(std::string const & data);

    private:
        ZStreamCompressPrivate *m_handle;
    };

    class ZStreamDecompress : public noncopyable
    {
    public :
        ZStreamDecompress(uint32_t chunkSize = 4);
        ~ZStreamDecompress();
        ZStreamDecompress(ZStreamDecompress&& _);
        ZStreamDecompress & operator=(ZStreamDecompress&& _);

        /// @brief z_stream is valid or not.
        /// @return  true => valid 
        bool isValid() const ;

        /// @brief finish decompress and return data.
        /// @return decompress data
        std::string finished();

        /// @brief append compressed data in internal buffer to decompress.
        /// @param data compressed data
        /// @return ok => zero ; valid z_stream => -1 ; data error => -2
        int decompress(std::string const & data);

    private:
        ZStreamDecompressPrivate *m_handle;
    };


}




