#!/bin/sh

sudo apt update

### GCC/GDB
sudo apt install gcc gdb g++ clangd

### third-part library
sudo apt install libssl-dev zlib1g-dev libhiredis-dev libarchive-dev libpq-dev libmysqlclient-dev uuid-dev libgrpc-dev libgrpc++-dev libfmt-dev libnghttp2-dev libnghttp3-dev liburing-dev

    # add_links("ssl") -- OpenSSL
    # add_links("crypto") -- OpenSSL need
    # add_links("hiredis") -- Hiredis library (A minimalistic C client library for the Redis database)
    # add_links("z") -- libz (GnuZip library)
    # add_links("archive") -- libarchive (Compress And Decompress)
    # add_links("pq") -- libpq-dev (C application programmer's interface to PostgreSQL)
    # add_links("mysqlclient") --libmysqlclient (A mysql client library for C development)
    # add_links("uuid") -- uuid-dev (uuid library)
    # add_links("grpc") -- libgrpc-dev (grpc library)
    # add_links("grpc++") -- libgrpc++-dev (grpc++ library)
    # add_links("fmt") -- string format library
    # add_links("nghttp2") -- libnghttp2 library
    # add_links("nghttp3") -- libnghttp3 library
    # add_links("uring")  -- liburing
