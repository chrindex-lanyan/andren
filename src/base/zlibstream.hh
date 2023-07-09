#pragma once

#include <stdint.h>
#include <string>

#include "noncopyable.hpp"

namespace chrindex ::andren::base
{
    class ZStreamCompressPrivate;

    class ZStreamCompress : public noncopyable
    {
    public :
        ZStreamCompress(uint32_t chunkSize = 4);
        ~ZStreamCompress();
        ZStreamCompress(ZStreamCompress&& _);
        ZStreamCompress & operator=(ZStreamCompress&& _);

        bool isValid() const ;

        std::string finished();

        int compress(std::string const & data);

    private:

        ZStreamCompressPrivate *m_handle;

    };

    class ZStreamDecompressPrivate;

    class ZStreamDecompress : public noncopyable
    {
    public :
        ZStreamDecompress(uint32_t chunkSize = 4);
        ~ZStreamDecompress();
        ZStreamDecompress(ZStreamDecompress&& _);
        ZStreamDecompress & operator=(ZStreamDecompress&& _);

        bool isValid() const ;

        std::string finished();

        int decompress(std::string const & data);

    private:

        ZStreamDecompressPrivate *m_handle;

    };


}




