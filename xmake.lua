add_rules("mode.debug", "mode.release")

set_languages("cxx20")
add_cxxflags("-Wall ")

main_project_src = "./"

--- third partition 
third_part_dir = "./third-part/"

-- example test
example_src = "./example/"

--- main src pattern 

main_cpp_src_pattern =  main_project_src .. "./src/**.cpp"
main_c_src_pattern =  main_project_src .. "./src/**.c"

main_c_head_pattern = main_project_src .. "./src/**.h"
main_cpp_head_pattern = main_project_src .. "./src/**.hh"
main_ext_head_pattern = main_project_src .. "./include/**.h"

--- third part pattern
base64_lib_src_pattern = third_part_dir .. "./base64_src/base64.cpp"
llhttp_lib_src_pattern = third_part_dir .. "./llhttp_src/generate_src/*.c"

base64_lib_head_pattern = third_part_dir .. "./base64_src/base64.h"
llhttp_lib_head_pattern = third_part_dir .. "./llhttp_src/generate_src/*.h"
nlohmannJson_lib_hea_pattern = third_part_dir .. "./nlohmann_json/*.hpp"

--- example pattern
example_2x_pattern = example_src .. "./example_2xHeap.cpp"
example_4x_pattern = example_src .. "./example_4xHeap.cpp"
example_timer_pattern = example_src .. "./example_timer.cpp"
example_signal_pattern = example_src .. "./example_signal.cpp"
example_pipe_pattern = example_src .. "./example_pipe.cpp"
example_shmutex_pattern = example_src .. "./example_shmutex.cpp"
example_shmem_pattern = example_src .. "./example_shmem.cpp"
example_zlibstream_pattern = example_src .. "./example_zlibstream.cpp"
example_threadpool_pattern = example_src .. "./example_threadpool.cpp"
example_mysql_statement_pattern = example_src .. "./example_mysql_statement.cpp"
example_pgsql_pattern = example_src .. "./example_pgsql.cpp"
example_DBuffer_pattern = example_src .. "./example_DBuffer.cpp"
example_socket_pattern = example_src .. "./example_socket.cpp"
example_ssl_pattern = example_src .. "./example_ssl.cpp"
example_taskdistributor_pattern = example_src .. "./example_taskdistributor.cpp"
example_epoll_tcpserver_pattern = example_src .. "./example_epoll_tcpserver.cpp"
example_asyncfile_pattern = example_src .. "./example_afio.cpp"
example_grpc_pattern = example_src .. "./example_grpc.cpp"
example_sslstream_pattern = example_src .. "./example_sslstream.cpp"
example_https2_pattern = example_src .. "./example_http2.cpp"
example_unixdomain_pattern = example_src .. "./example_unixdomain_socket.cpp"
example_freelockshmem_pattern = example_src .. "./example_freelockshmem.cpp"
example_detachtcpserver_pattern = example_src .. "./example_detachtcpserver.cpp"
example_io_file_pattern = example_src .. "./example_io_file.cpp"
example_coro_runner_pattern = example_src .. "./example_coro_runner.cpp"



target("andren")
    set_kind("shared")
    ---head list 
    add_headerfiles(main_c_head_pattern)
    add_headerfiles(main_cpp_head_pattern)
    add_headerfiles(main_ext_head_pattern)

    add_headerfiles(base64_lib_head_pattern)
    add_headerfiles(llhttp_lib_head_pattern)
    add_headerfiles(nlohmannJson_lib_hea_pattern)

    -- src list
    add_files(main_cpp_src_pattern)
    add_files(main_c_src_pattern)
    add_files(base64_lib_src_pattern)
    add_files(llhttp_lib_src_pattern)

    -- link lib list
    add_links("pthread") -- posix thread library (System)
    add_links("dl")  -- dynamic linking library (System)
    add_links("ssl") -- OpenSSL
    add_links("crypto") -- OpenSSL need
    add_links("hiredis") -- Hiredis library (A minimalistic C client library for the Redis database)
    add_links("z") -- libz (GnuZip library)
    add_links("archive") -- libarchive (Compress And Decompress)
    add_links("pq") -- libpq-dev (C application programmer's interface to PostgreSQL)
    add_links("mysqlclient") --libmysqlclient (A mysql client library for C development)
    add_links("uuid") -- uuid-dev (uuid library)
    add_links("grpc") -- libgrpc-dev (grpc library)
    add_links("grpc++") -- libgrpc++-dev (grpc++ library)
    add_links("protobuf") -- protobuf c++ (grpc++ need)
    add_links("fmt") -- string format library
    add_links("nghttp2") -- libnghttp2 library
    add_links("nghttp3") -- libnghttp3 library
    add_links("uring")  -- liburing
    -- link lib dir list 
    -- add_linkdirs("your path")

target("andren_a")
    set_kind("static")
    ---head list 
    add_headerfiles(main_c_head_pattern)
    add_headerfiles(main_cpp_head_pattern)
    add_headerfiles(main_ext_head_pattern)

    add_headerfiles(base64_lib_head_pattern)
    add_headerfiles(llhttp_lib_head_pattern)
    add_headerfiles(nlohmannJson_lib_hea_pattern)

    -- src list
    add_files(main_cpp_src_pattern)
    add_files(main_c_src_pattern)
    add_files(base64_lib_src_pattern)
    add_files(llhttp_lib_src_pattern)

    -- link lib list
    add_links("pthread") -- posix thread library (System)
    add_links("dl")  -- dynamic linking library (System)
    add_links("ssl") -- OpenSSL
    add_links("crypto") -- OpenSSL need
    add_links("hiredis") -- Hiredis library (A minimalistic C client library for the Redis database)
    add_links("z") -- libz (GnuZip library)
    add_links("archive") -- libarchive (Compress And Decompress)
    add_links("pq") -- libpq-dev (C application programmer's interface to PostgreSQL)
    add_links("mysqlclient") --libmysqlclient (A mysql client library for C development)
    add_links("uuid") -- uuid-dev (uuid library)
    add_links("grpc") -- libgrpc-dev (grpc library)
    add_links("grpc++") -- libgrpc++-dev (grpc++ library)
    add_links("protobuf") -- protobuf c++ (grpc++ need)
    add_links("fmt") -- string format library
    add_links("nghttp2") -- libnghttp2 library
    add_links("nghttp3") -- libnghttp3 library
    add_links("uring")   -- liburing

    -- link lib dir list 
    -- add_linkdirs("your path")


target("test_2xheap")
    set_kind("binary")
    add_files(example_2x_pattern)
    add_deps("andren", {private = true})

target("test_4xheap")
    set_kind("binary")
    add_files(example_4x_pattern)
    add_deps("andren", {private = true})

target("test_timer")
    set_kind("binary")
    add_files(example_timer_pattern)
    add_deps("andren", {private = true})

target("test_signal")
    set_kind("binary")
    add_files(example_signal_pattern)
    add_deps("andren", {private = true})

target("test_pipe")
    set_kind("binary")
    add_files(example_pipe_pattern)
    add_deps("andren", {private = true})
    
target("test_shmutex")
    set_kind("binary")
    add_files(example_shmutex_pattern)
    add_deps("andren", {private = true})

target("test_shmem")
    set_kind("binary")
    add_files(example_shmem_pattern)
    add_deps("andren", {private = true})

target("test_zlibstream")
    set_kind("binary")
    add_files(example_zlibstream_pattern)
    add_deps("andren", {private = true})
    
target("test_threadpool")
    set_kind("binary")
    add_files(example_threadpool_pattern)
    add_deps("andren", {private = true})
   
target("test_mysql_statement")
    set_kind("binary")
    add_files(example_mysql_statement_pattern)
    add_links("fmt")
    add_deps("andren", {private = true})

target("test_pgsql_statement")
    set_kind("binary")
    add_files(example_pgsql_pattern)
    add_links("fmt")
    add_deps("andren", {private = true})
    
target("test_DBuffer")
    set_kind("binary")
    add_files(example_DBuffer_pattern)
    add_links("fmt")
    add_deps("andren", {private = true})
        
target("test_socket")
    set_kind("binary")
    add_files(example_socket_pattern)
    add_links("fmt")
    add_deps("andren", {private = true})

target("test_ssl_socket")
    set_kind("binary")
    add_files(example_ssl_pattern)
    add_links("fmt")
    add_deps("andren", {private = true})

target("test_taskdistributor")
    set_kind("binary")
    add_files(example_taskdistributor_pattern)
    add_links("fmt")
    add_deps("andren", {private = true})

target("test_epoll_tcpserver")
    set_kind("binary")
    add_files(example_epoll_tcpserver_pattern)
    add_links("fmt")
    add_deps("andren", {private = true})
    
target("test_afio")
    set_kind("binary")
    add_files(example_asyncfile_pattern)
    add_links("fmt")
    add_deps("andren", {private = true})
    
target("test_grpc")
    set_kind("binary")
    add_files(example_src .. "greet.grpc.pb.cc")
    add_files(example_src .. "greet.pb.cc")
    add_files(example_grpc_pattern)
    add_links("fmt")
    add_links("grpc") -- libgrpc-dev (grpc library)
    add_links("grpc++") -- libgrpc++-dev (grpc++ library)
    add_links("protobuf") -- protobuf c++ (grpc++ need)
    add_deps("andren", {private = true})
    
target("test_sslstream")
    set_kind("binary")
    add_files(example_sslstream_pattern)
    add_links("fmt")
    add_deps("andren", {private = true})
    
target("test_https2")
    set_kind("binary")
    add_files(example_https2_pattern)
    add_links("fmt")
    add_deps("andren", {private = true})

target("test_unixdomain")
    set_kind("binary")
    add_files(example_unixdomain_pattern)
    add_links("fmt")
    add_deps("andren", {private = true})

target("test_freelockshmem")
    set_kind("binary")
    add_files(example_freelockshmem_pattern)
    add_links("fmt")
    add_deps("andren", {private = true})
 
target("test_detachtcpserver")
    set_kind("binary")
    add_files(example_detachtcpserver_pattern)
    add_links("fmt")
    add_deps("andren", {private = true})   

target("test_io_file")
    set_kind("binary")
    add_files(example_io_file_pattern)
    add_links("fmt")
    add_deps("andren", {private = true})   

target("test_coro_runner")
    set_kind("binary")
    add_files(example_coro_runner_pattern)
    add_links("fmt")
    add_deps("andren", {private = true})   
    