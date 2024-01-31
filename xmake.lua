add_rules("mode.debug", "mode.release")

set_kind("static")

package("zycore")
    add_deps("cmake")

    set_sourcedir(path.join(os.scriptdir(), "3rd/zycore"))

    on_install(function (package)
        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=OFF")
        table.insert(configs, "-DZYCORE_BUILD_SHARED_LIB=OFF")
        import("package.tools.cmake").install(package, configs)
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            #include <Zycore/Comparison.h>
            #include <Zycore/Vector.h>
            void test() {
                ZyanVector vector;
                ZyanU16 buffer[32];
            }
        ]]}))
    end)
package_end()

package("zydis")
    add_deps("cmake", "zycore")

    set_sourcedir(path.join(os.scriptdir(), "3rd/zydis"))

    on_load(function (package)
        package:add("deps", "zycore")
    end)

    on_install(function (package)
        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=OFF")
        table.insert(configs, "-DZYDIS_BUILD_SHARED_LIB=OFF")
        table.insert(configs, "-DZYAN_ZYCORE_PATH=" .. path.join(os.scriptdir(), "3rd/zycore"))
        import("package.tools.cmake").install(package, configs)
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            #include <Zydis/Zydis.h>
            #include <Zycore/LibC.h>
            void test() {
                ZyanU8 encoded_instruction[ZYDIS_MAX_INSTRUCTION_LENGTH];
                ZyanUSize encoded_length = sizeof(encoded_instruction);
            }
        ]]}, {configs = {languages = "c++11"}}))
    end)
package_end()


package("fmt")
    add_deps("cmake")
    set_sourcedir(path.join(os.scriptdir(), "3rd/fmt"))

    on_install(function (package)
        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=OFF")
        import("package.tools.cmake").install(package, configs)
    end)
package_end()

package("spdlog")
    add_deps("cmake", "fmt")
    set_sourcedir(path.join(os.scriptdir(), "3rd/spdlog"))

    on_load(function (package)
        package:add("deps", "fmt")
    end)

    on_install(function (package)
        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=OFF")
        import("package.tools.cmake").install(package, configs)
    end)
package_end()




add_requires("spdlog")


target("pal-plugin-loader")
    set_kind("shared")

    set_languages("c17", "cxx20")

    add_defines("ZYCORE_STATIC_BUILD")
    add_defines("ZYDIS_STATIC_BUILD")
    add_defines("WASM_API_EXTERN=inline")

    add_packages("spdlog")

    if is_os("windows") then
        add_syslinks("ws2_32.lib")
        add_syslinks("ntdll.lib")
        add_syslinks("advapi32.lib")
        add_syslinks("userenv.lib")
        add_syslinks("bcrypt.lib")
        add_syslinks("ole32.lib")
        add_syslinks("shell32.lib")
        -- ��� /bigobj �ͺ����ض�����ı���ѡ��
        add_cxflags("/bigobj", "/wd4369", {force = true})
        add_cxxflags("/bigobj", "/wd4369", {force = true})
    end

    add_includedirs(path.join(os.scriptdir(), "include/sdk/sdk"))
    add_includedirs(path.join(os.scriptdir(), "include/sdk"))
    add_includedirs(path.join(os.scriptdir(), "include"))
    add_includedirs("C:/boost")  -- �滻Ϊ���Boost��װ·��


    if is_os("windows") then
        add_includedirs(path.join(os.scriptdir(), "include/os/windows/sdk"))
        add_files("src/os/windows/*.cpp")
    end

    add_files("src/*.cpp")
    add_files("src/sdk/*.cpp")




    

