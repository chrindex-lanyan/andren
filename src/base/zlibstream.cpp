

#include <zlib.h>
#include <string>
#include <string.h>
#include <stdint.h>

#include "zlibstream.hh"

namespace chrindex ::andren::base
{

    class ZStreamCompressPrivate
    {
    public:
        ZStreamCompressPrivate(uint32_t CHUNK_SIZE = 8)
        {
            tmp.resize(CHUNK_SIZE);
            memset(&stream, 0, sizeof(stream));
            if (deflateInit(&stream, Z_DEFAULT_COMPRESSION) != Z_OK)
            {
                m_valid = false;
            }
            else
            {
                m_valid = true;
            }
        }

        ~ZStreamCompressPrivate()
        {
            finished();
        }

        bool isValid() const { return m_valid; }

        std::string finished()
        {
            if (!m_valid)
            {
                return {};
            }
            int ret = 0;
            do
            {
                stream.next_out = reinterpret_cast<uint8_t *>(&tmp[0]);
                stream.avail_out = tmp.size();
                ret = deflate(&stream, Z_FINISH);

                size_t usedSize = tmp.size() - stream.avail_out;
                m_data.append(&tmp[0], usedSize);
            } while (ret != Z_STREAM_END);

            ret = deflateEnd(&stream);
            m_valid = false;
            return std::move(m_data);
        }

        int compress(std::string const &data)
        {
            if (m_valid == false)
            {
                return -1;
            }

            stream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(data.c_str()));
            stream.avail_in = data.size();

            while (stream.avail_in > 0)
            {
                stream.next_out = reinterpret_cast<uint8_t *>(&tmp[0]);
                stream.avail_out = tmp.size();

                if (deflate(&stream, Z_NO_FLUSH) != Z_OK)
                {
                    return -2;
                }
                size_t usedSize = tmp.size() - stream.avail_out;
                m_data.append(&tmp[0], usedSize);
            }
            return 0;
        }

    private:
        z_stream stream;
        std::string m_data;
        std::string tmp;
        bool m_valid;
    };

    ZStreamCompress::ZStreamCompress(uint32_t chunkSize)
    {
        m_handle = new ZStreamCompressPrivate(chunkSize);
    }
    ZStreamCompress::~ZStreamCompress()
    {
        delete m_handle;
    }

    ZStreamCompress::ZStreamCompress(ZStreamCompress &&_)
    {
        m_handle = _.m_handle;
        _.m_handle = 0;
    }

    ZStreamCompress &ZStreamCompress::operator=(ZStreamCompress &&_)
    {
        this->~ZStreamCompress();
        m_handle = _.m_handle;
        _.m_handle = 0;
        return *this;
    }

    bool ZStreamCompress::isValid() const
    {
        return m_handle && m_handle->isValid();
    }

    std::string ZStreamCompress::finished()
    {
        return m_handle->finished();
    }

    int ZStreamCompress::compress(std::string const &data)
    {
        return m_handle->compress(data);
    }

    class ZStreamDecompressPrivate
    {
    public:
        ZStreamDecompressPrivate(uint32_t chunkSize = 4)
        {
            m_valid = false;
        }
        ~ZStreamDecompressPrivate() {}
        ZStreamDecompressPrivate(ZStreamDecompressPrivate &&_) {}
        ZStreamDecompressPrivate &operator=(ZStreamDecompressPrivate &&_) { return *this; }

        bool isValid() const { return m_valid; }

        std::string finished() { return m_data; }

        int decompress(std::string const &data) { return 0; }

    private:
        z_stream stream;
        std::string m_data;
        std::string tmp;
        bool m_valid;
    };

    ZStreamDecompress::ZStreamDecompress(uint32_t chunkSize )
    {
        m_handle = new ZStreamDecompressPrivate(chunkSize);
    }
    ZStreamDecompress::~ZStreamDecompress()
    {
        delete m_handle;
    }
    ZStreamDecompress::ZStreamDecompress(ZStreamDecompress &&_)
    {
        m_handle = _.m_handle;
        _.m_handle = 0;
    }

    ZStreamDecompress &ZStreamDecompress::operator=(ZStreamDecompress &&_)
    {
        this->~ZStreamDecompress();
        m_handle = _.m_handle;
        _.m_handle = 0;
        return *this;
    }

    bool ZStreamDecompress::isValid() const
    {
        return m_handle && m_handle->isValid();
    }

    std::string ZStreamDecompress::finished()
    {
        return m_handle->finished();
    }

    int ZStreamDecompress::decompress(std::string const &data)
    {
        return m_handle->decompress(data);
    }

}
