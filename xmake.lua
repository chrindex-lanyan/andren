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
example_src_pattern = example_src .. "./*.cpp"


target("andren")
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

    -- example src list
    add_files(example_src_pattern)

    -- link lib list
    add_links("pthread") -- 
    add_links("dl")  -- 
    add_links("ssl") -- open ssl
    add_links("hiredis") -- hiredis client sdk

    -- link lib dir list 
    -- add_linkdirs("your path")
