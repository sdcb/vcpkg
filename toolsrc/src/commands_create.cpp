#include "pch.h"
#include "vcpkg_Commands.h"
#include "vcpkg_System.h"
#include "vcpkg_Environment.h"
#include "vcpkg_Files.h"
#include "vcpkg_Input.h"

namespace vcpkg::Commands::Create
{
    void perform_and_exit(const vcpkg_cmd_arguments& args, const vcpkg_paths& paths)
    {
        static const std::string example = Commands::Help::create_example_string(R"###(create zlib2 http://zlib.net/zlib128.zip "zlib128-2.zip")###");
        args.check_max_arg_count(3, example);
        args.check_min_arg_count(2, example);

        const std::string port_name = args.command_arguments.at(0);
        Environment::ensure_utilities_on_path(paths);

        // Space OR define the FILENAME with proper spacing
        std::wstring custom_filename = L" ";
        if (args.command_arguments.size() >= 3)
        {
            const std::string& zip_file_name = args.command_arguments.at(2);
            Checks::check_exit(!Files::has_invalid_chars_for_filesystem(zip_file_name),
                               R"(Filename cannot contain invalid chars %s, but was %s)",
                               Files::FILESYSTEM_INVALID_CHARACTERS, zip_file_name);
            custom_filename = Strings::wformat(LR"( -DFILENAME="%s" )", Strings::utf8_to_utf16(zip_file_name));
        }

        const std::wstring cmdline = Strings::wformat(LR"(cmake -DCMD=CREATE -DPORT=%s -DURL=%s%s-P "%s")",
                                                      Strings::utf8_to_utf16(port_name),
                                                      Strings::utf8_to_utf16(args.command_arguments.at(1)),
                                                      custom_filename,
                                                      paths.ports_cmake.generic_wstring());

        exit(System::cmd_execute(cmdline));
    }
}
