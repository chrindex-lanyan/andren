

#include "../include/andren.hh"

#include <iostream>

std::string source1 = "This is the first part of data.";
std::string source2 = "This is the second part of data.";

std::string cdata; // compressed data
std::string ddata; // decompressed data
int chunk_size = 4; // internal buffer size 

using namespace chrindex::andren::base;

int test_compress()
{
    int ret = 0;
    ZStreamCompress zstream(chunk_size);

    ret = zstream.compress(source1); assert(ret == 0);
    ret = zstream.compress(source2); assert(ret == 0);
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

    ret = zstream.decompress(data1); assert(ret == 0);
    ret = zstream.decompress(data2); assert(ret == 0);
    ddata = zstream.finished();

    std::cout << "解压后(" << ddata.size() << "):" << ddata << std::endl;

    return 0;
}

int main(int argc, char **argv)
{
    test_compress();
    test_decompress();

    return 0;
}
