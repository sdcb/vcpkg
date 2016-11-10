#include "vcpkg_Commands.h"
#include "vcpkg.h"
#include <fstream>
#include "vcpkg_Environment.h"
#include "metrics.h"
#include "vcpkg_Files.h"
#include "post_build_lint.h"
#include "vcpkg_System.h"
#include "vcpkg_Dependencies.h"
#include "vcpkg_Input.h"
#include "vcpkg_Maps.h"
#include "Paragraphs.h"
#include "vcpkg_info.h"

namespace vcpkg
{
    static void create_binary_control_file(const vcpkg_paths& paths, const SourceParagraph& source_paragraph, const triplet& target_triplet)
    {
        auto bpgh = BinaryParagraph(source_paragraph, target_triplet);
        const fs::path binary_control_file = paths.packages / bpgh.dir() / "CONTROL";
        std::ofstream(binary_control_file) << bpgh;
    }

    static void build_internal(const package_spec& spec, const vcpkg_paths& paths, const fs::path& port_dir)
    {
        auto pghs = Paragraphs::get_paragraphs(port_dir / "CONTROL");
        Checks::check_exit(pghs.size() == 1, "Error: invalid control file");
        SourceParagraph source_paragraph(pghs[0]);

        const fs::path ports_cmake_script_path = paths.ports_cmake;
        auto&& target_triplet = spec.target_triplet();
        const std::wstring command = Strings::wformat(LR"("%%VS140COMNTOOLS%%..\..\VC\vcvarsall.bat" %s && cmake -DCMD=BUILD -DPORT=%s -DTARGET_TRIPLET=%s "-DCURRENT_PORT_DIR=%s/." -P "%s")",
                                                      Strings::utf8_to_utf16(target_triplet.architecture()),
                                                      Strings::utf8_to_utf16(spec.name()),
                                                      Strings::utf8_to_utf16(target_triplet.canonical_name()),
                                                      port_dir.generic_wstring(),
                                                      ports_cmake_script_path.generic_wstring());

        System::Stopwatch2 timer;
        timer.start();
        int return_code = System::cmd_execute(command);
        timer.stop();
        TrackMetric("buildtimeus-" + to_string(spec), timer.microseconds());

        if (return_code != 0)
        {
            System::println(System::color::error, "Error: building package %s failed", to_string(spec));
            System::println("Please ensure sure you're using the latest portfiles with `vcpkg update`, then\n"
                            "submit an issue at https://github.com/Microsoft/vcpkg/issues including:\n"
                            "  Package: %s\n"
                            "  Vcpkg version: %s\n"
                            "\n"
                            "Additionally, attach any relevant sections from the log files above."
                            , to_string(spec), Info::version());
            TrackProperty("error", "build failed");
            TrackProperty("build_error", to_string(spec));
            exit(EXIT_FAILURE);
        }

        perform_all_checks(spec, paths);

        create_binary_control_file(paths, source_paragraph, target_triplet);

        // const fs::path port_buildtrees_dir = paths.buildtrees / spec.name;
        // delete_directory(port_buildtrees_dir);
    }

    static void build_internal(const package_spec& spec, const vcpkg_paths& paths)
    {
        return build_internal(spec, paths, paths.ports / spec.name());
    }

    void install_command(const vcpkg_cmd_arguments& args, const vcpkg_paths& paths, const triplet& default_target_triplet)
    {
        static const std::string example = create_example_string("install zlib zlib:x64-windows curl boost");
        args.check_min_arg_count(1, example.c_str());
        StatusParagraphs status_db = database_load_check(paths);

        std::vector<package_spec> specs = Input::check_and_get_package_specs(args.command_arguments, default_target_triplet, example.c_str());
        Input::check_triplets(specs, paths);
        std::vector<package_spec> install_plan = Dependencies::create_dependency_ordered_install_plan(paths, specs, status_db);
        Checks::check_exit(!install_plan.empty(), "Install plan cannot be empty");
        std::string specs_string = to_string(install_plan[0]);
        for (size_t i = 1; i < install_plan.size(); ++i)
        {
            specs_string.push_back(',');
            specs_string.append(to_string(install_plan[i]));
        }
        TrackProperty("installplan", specs_string);
        Environment::ensure_utilities_on_path(paths);

        for (const package_spec& spec : install_plan)
        {
            if (status_db.find_installed(spec.name(), spec.target_triplet()) != status_db.end())
            {
                System::println(System::color::success, "Package %s is already installed", spec);
                continue;
            }

            fs::path package_path = paths.package_dir(spec);

            expected<std::string> file_contents = Files::get_contents(package_path / "CONTROL");

            try
            {
                if (file_contents.error_code())
                {
                    build_internal(spec, paths);
                    file_contents = Files::get_contents(package_path / "CONTROL");
                    if (file_contents.error_code())
                    {
                        file_contents.get_or_throw();
                    }
                }

                auto pghs = Paragraphs::parse_paragraphs(file_contents.get_or_throw());
                Checks::check_throw(pghs.size() == 1, "multiple paragraphs in control file");
                install_package(paths, BinaryParagraph(pghs[0]), status_db);
                System::println(System::color::success, "Package %s is installed", spec);
            }
            catch (const std::exception& e)
            {
                System::println(System::color::error, "Error: Could not install package %s: %s", spec, e.what());
                exit(EXIT_FAILURE);
            }
        }

        exit(EXIT_SUCCESS);
    }

    void build_command(const vcpkg_cmd_arguments& args, const vcpkg_paths& paths, const triplet& default_target_triplet)
    {
        static const std::string example = create_example_string("build zlib:x64-windows");

        // Installing multiple packages leads to unintuitive behavior if one of them depends on another.
        // Allowing only 1 package for now.

        args.check_exact_arg_count(1, example.c_str());
        StatusParagraphs status_db = database_load_check(paths);

        const package_spec spec = Input::check_and_get_package_spec(args.command_arguments.at(0), default_target_triplet, example.c_str());
        Input::check_triplet(spec.target_triplet(), paths);

        // Explicitly load and use the portfile's build dependencies when resolving the build command (instead of a cached package's dependencies).
        auto first_level_deps = Dependencies::get_unmet_package_build_dependencies(paths, spec);
        std::vector<package_spec> first_level_deps_specs;
        for (auto&& dep : first_level_deps)
        {
            first_level_deps_specs.push_back(package_spec::from_name_and_triplet(dep, spec.target_triplet()).get_or_throw());
        }

        std::unordered_set<package_spec> unmet_dependencies = Dependencies::get_unmet_dependencies(paths, first_level_deps_specs, status_db);
        if (!unmet_dependencies.empty())
        {
            System::println(System::color::error, "The build command requires all dependencies to be already installed.");
            System::println("The following dependencies are missing:");
            System::println("");
            for (const package_spec& p : unmet_dependencies)
            {
                System::println("    %s", to_string(p));
            }
            System::println("");
            exit(EXIT_FAILURE);
        }

        Environment::ensure_utilities_on_path(paths);
        build_internal(spec, paths);
        exit(EXIT_SUCCESS);
    }

    void build_external_command(const vcpkg_cmd_arguments& args, const vcpkg_paths& paths, const triplet& default_target_triplet)
    {
        static const std::string example = create_example_string(R"(build_external zlib2 C:\path\to\dir\with\controlfile\)");
        args.check_exact_arg_count(2, example.c_str());

        expected<package_spec> current_spec = package_spec::from_string(args.command_arguments[0], default_target_triplet);
        if (auto spec = current_spec.get())
        {
            Input::check_triplet(spec->target_triplet(), paths);
            Environment::ensure_utilities_on_path(paths);
            const fs::path port_dir = args.command_arguments.at(1);
            build_internal(*spec, paths, port_dir);
            exit(EXIT_SUCCESS);
        }

        System::println(System::color::error, "Error: %s: %s", current_spec.error_code().message(), args.command_arguments[0]);
        print_example(Strings::format("%s zlib:x64-windows", args.command).c_str());
        exit(EXIT_FAILURE);
    }
}
