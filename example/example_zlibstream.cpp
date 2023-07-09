

#include "../include/andren.hh"

#include <iostream>

std::string source1 = "This is the first part of data.";
std::string source2 = "This is the second part of data.";

std::string cdata;
std::string ddata;


using namespace chrindex::andren::base;

int test_compress()
{
    int ret = 0;
    ZStreamCompress zstream(4);

    ret = zstream.compress(source1);
    ret = zstream.compress(source2);

    assert(ret == 0);

    cdata = zstream.finished();

    std::cout << "压缩后(" << cdata.size() << "):" << cdata << std::endl;

    return 0;
}

int test_decompress()
{

    return 0;
}

int main(int argc, char **argv)
{
    test_compress();
    test_decompress();

    return 0;
}
