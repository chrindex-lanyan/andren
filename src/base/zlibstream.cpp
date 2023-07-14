

#include <zlib.h>
#include <string>
#include <string.h>
#include <stdint.h>

#include <archive.h>
#include <archive_entry.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "ufile.hh"

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
        ZStreamDecompressPrivate(uint32_t CHUNK_SIZE = 4)
        {
            tmp.resize(CHUNK_SIZE);
            memset(&stream, 0, sizeof(stream));
            if (inflateInit(&stream) != Z_OK)
            {
                m_valid = false;
            }
            else
            {
                m_valid = true;
            }
        }
        ~ZStreamDecompressPrivate()
        {
            finished();
        }

        std::string finished()
        {
            if (!m_valid)
            {
                return {};
            }

            stream.next_in = 0;
            stream.avail_in = 0;
            int ret = inflate(&stream, Z_FINISH);
            ret = inflateEnd(&stream);
            m_valid = false;
            return ret == 0 ? std::move(m_data) : std::string{};
        }

        int decompress(std::string const &data)
        {
            if (m_valid == false && data.size() <= 0)
            {
                return -1;
            }

            stream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(data.c_str()));
            stream.avail_in = data.size();

            while (stream.avail_in > 0)
            {
                stream.next_out = reinterpret_cast<uint8_t *>(&tmp[0]);
                stream.avail_out = tmp.size();
                int ret = inflate(&stream, Z_NO_FLUSH);
                if (ret != Z_OK && ret != Z_STREAM_END)
                {
                    return -2;
                }
                size_t usedSize = tmp.size() - stream.avail_out;
                m_data.append(&tmp[0], usedSize);
            }
            return 0;
        }

        bool isValid() const { return m_valid; }

    private:
        z_stream stream;
        std::string m_data;
        std::string tmp;
        bool m_valid;
    };

    ZStreamDecompress::ZStreamDecompress(uint32_t chunkSize)
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

    Archive::Archive(ArchiveType type, const std::string &filePath, uint32_t bufferSize )
        : type_(type), filePath_(filePath)
    {
        buffer.resize(bufferSize);
        if (type_ == ArchiveType::COMPRESS)
        {
            archive_ = archive_write_new();
            archive_write_set_format_7zip(archive_);
            archive_write_open_filename(archive_, filePath.c_str());
        }
        else if (type_ == ArchiveType::DECOMPRESS)
        {
            archive_ = archive_read_new();
            archive_read_support_format_7zip(archive_);
            archive_read_open_filename(archive_, filePath.c_str(), bufferSize);
        }
    }

    Archive::~Archive()
    {
        if (archive_)
        {
            if (type_ == ArchiveType::COMPRESS)
            {
                archive_write_close(archive_);
                archive_write_free(archive_);
            }
            else if (type_ == ArchiveType::DECOMPRESS)
            {
                archive_read_close(archive_);
                archive_read_free(archive_);
            }
        }
    }

    int Archive::compress(const std::string &filePath)
    {
        if (type_ != ArchiveType::COMPRESS)
        {
            return -1;
        }
        struct stat st;

        if (stat(filePath.c_str(), &st) != 0)
        {
            return -2;
        }

        struct archive_entry *entry = archive_entry_new();
        archive_entry_set_pathname(entry, filePath.c_str());
        archive_entry_set_filetype(entry, AE_IFREG);
        archive_entry_set_perm(entry, 0644);
        archive_entry_set_size(entry, st.st_size);
        archive_write_header(archive_, entry);

        ssize_t rnbytes = 0;
        File file;
        file.open(filePath, O_RDWR);
        while ((rnbytes = file.read(&buffer[0], buffer.size())) > 0)
        {
            archive_write_data(archive_, &buffer[0], rnbytes);
        }

        archive_entry_free(entry);
        return 0;
    }

    int Archive::decompress(std::string const &_targetDir, std::vector<std::string> &outList)
    {
        if (type_ != ArchiveType::DECOMPRESS)
        {
            return -1;
        }
        struct archive_entry *entry;
        std::string entryPath;
        std::string targetDir = _targetDir;
        if (targetDir.size() == 0)
        {
            targetDir = "./";
        }
        if (targetDir[targetDir.size() - 1] != '/')
        {
            targetDir.push_back('/');
        }
        while (archive_read_next_header(archive_, &entry) == ARCHIVE_OK)
        {
            if (archive_entry_size(entry) > 0)
            {
                constexpr int mode =  0775 ;
                entryPath = targetDir + archive_entry_pathname(entry);
                int ret = createDirectories(entryPath,mode);
                if (ret)
                {
                    return -2;
                }
                File file;
                file.open(entryPath, O_CREAT | O_RDWR, mode );
                if (file.handle() <= 0)
                {
                    return -3;
                }
                outList.push_back(std::move(entryPath));
                size_t bytesRead;
                while ((bytesRead = archive_read_data(archive_, &buffer[0], buffer.size())) > 0)
                {
                    file.write(&buffer[0], bytesRead);
                }
            }
            archive_read_data_skip(archive_);
        }

        return 0;
    }

    int Archive::createDirectories(const std::string &path,  int mode)
    {
        return File::createDirectories(path,mode,true);
    }

}
