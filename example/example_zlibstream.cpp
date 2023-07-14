

#include "../include/andren.hh"

#include <iostream>

std::string source1 = "This is the first part of data.";
std::string source2 = "This is the second part of data.";

std::string cdata;  // compressed data
std::string ddata;  // decompressed data
int chunk_size = 4; // internal buffer size

using namespace chrindex::andren::base;

int test_compress()
{
    int ret = 0;
    ZStreamCompress zstream(chunk_size);

    ret = zstream.compress(source1);
    assert(ret == 0);
    ret = zstream.compress(source2);
    assert(ret == 0);
    cdata = zstream.finished();

    std::cout << "压缩后(" << cdata.size() << "):" << cdata << std::endl;

    return 0;
}

int test_decompress()
{
    int ret = 0;
    ZStreamDecompress zstream(chunk_size);
    std::string data1, data2;

    data1.insert(data1.begin(), cdata.begin(), cdata.begin() + 6);
    data2.insert(data2.begin(), cdata.begin() + 6, cdata.end());

    ret = zstream.decompress(data1);
    assert(ret == 0);
    ret = zstream.decompress(data2);
    assert(ret == 0);
    ddata = zstream.finished();

    std::cout << "解压后(" << ddata.size() << "):" << ddata << std::endl;

    return 0;
}

const char *sourceFilePath1 = "data1.txt";
const char *sourceFilePath2 = "data2.txt";
const char *compressedFilePath = "data.txt.7z";

int testArchive()
{
    int ret =0;
    uint64_t timeMsec, usedMsec;
    std::vector<std::string> outList;

    ///
    {
        File file1, file2;

        file1.open(sourceFilePath1, O_CREAT | O_RDWR,
                   S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        file2.open(sourceFilePath2, O_CREAT | O_RDWR,
                   S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

        file1.writeByBuf("hello world.\nI am lihua.\n");
        file2.writeByBuf("hello lihua.\nI am xiaohong.\n");
    }

    ///
    {
        Archive compress(ArchiveType::COMPRESS, compressedFilePath);
        timeMsec = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

        // 压缩数据为7z格式
        ret = compress.compress(sourceFilePath1);
        assert(ret ==0);
        ret = compress.compress(sourceFilePath2);
        assert(ret ==0);

        usedMsec = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        usedMsec = usedMsec - timeMsec;

        printf("compress used Msec %lu .\n", usedMsec);
    }

    ///
    {
        Archive decompress(ArchiveType::DECOMPRESS, compressedFilePath);
        timeMsec = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

        // 解压文件
        ret = decompress.decompress("./decompress/", outList);
        assert(ret ==0);

        usedMsec = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        usedMsec = usedMsec - timeMsec;

        printf("decompress Used Msec %lu .\n", usedMsec);
    }

    for (auto &path : outList)
    {
        printf("decompress \"%s\".\n", path.c_str());
    }

    return 0;
}

int main(int argc, char **argv)
{
    test_compress();
    test_decompress();

    testArchive();

    return 0;
}
