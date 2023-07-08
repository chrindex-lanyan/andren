add_rules("mode.debug", "mode.release")

set_languages("cxx20")
add_cxxflags("-Wall -O2")

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
    add_links("pthread") -- 
    add_links("dl")  -- 
    add_links("ssl") -- open ssl
    add_links("hiredis") -- hiredis client sdk
    add_links("z")
    add_links("rt")
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
    add_links("pthread") -- posix thread (System)
    add_links("dl")  -- dynamic load (System)
    add_links("ssl") -- open ssl (SSL)
    add_links("hiredis") -- hiredis client sdk (Redis Client SDK)
    add_links("z") -- libz (Zip Library)
    --add_links("unistring") -- libunistring (TextCodec)
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
