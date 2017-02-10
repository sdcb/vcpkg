#include "pch.h"
#include "expected.h"
#include "vcpkg_paths.h"
#include "metrics.h"
#include "vcpkg_System.h"
#include "package_spec.h"

namespace vcpkg
{
    expected<vcpkg_paths> vcpkg_paths::create(const fs::path& vcpkg_root_dir)
    {
        std::error_code ec;
        const fs::path canonical_vcpkg_root_dir = fs::canonical(vcpkg_root_dir, ec);
        if (ec)
        {
            return ec;
        }

        vcpkg_paths paths;
        paths.root = canonical_vcpkg_root_dir;

        if (paths.root.empty())
        {
            System::println(System::color::error, "Invalid vcpkg root directory: %s", paths.root.string());
            TrackProperty("error", "Invalid vcpkg root directory");
            exit(EXIT_FAILURE);
        }

        paths.packages = paths.root / "packages";
        paths.buildtrees = paths.root / "buildtrees";
        paths.downloads = paths.root / "downloads";
        paths.ports = paths.root / "ports";
        paths.installed = paths.root / "installed";
        paths.triplets = paths.root / "triplets";
        paths.scripts = paths.root / "scripts";

        paths.buildsystems = paths.scripts / "buildsystems";
        paths.buildsystems_msbuild_targets = paths.buildsystems / "msbuild" / "vcpkg.targets";

        paths.vcpkg_dir = paths.installed / "vcpkg";
        paths.vcpkg_dir_status_file = paths.vcpkg_dir / "status";
        paths.vcpkg_dir_info = paths.vcpkg_dir / "info";
        paths.vcpkg_dir_updates = paths.vcpkg_dir / "updates";

        paths.ports_cmake = paths.scripts / "ports.cmake";
        return paths;
    }

    fs::path vcpkg_paths::package_dir(const package_spec& spec) const
    {
        return this->packages / spec.dir();
    }

    fs::path vcpkg_paths::port_dir(const package_spec& spec) const
    {
        return this->ports / spec.name();
    }

    fs::path vcpkg_paths::build_info_file_path(const package_spec& spec) const
    {
        return this->package_dir(spec) / "BUILD_INFO";
    }

    fs::path vcpkg_paths::listfile_path(const BinaryParagraph& pgh) const
    {
        return this->vcpkg_dir_info / (pgh.fullstem() + ".list");
    }

    bool vcpkg_paths::is_valid_triplet(const triplet& t) const
    {
        auto it = fs::directory_iterator(this->triplets);
        for (; it != fs::directory_iterator(); ++it)
        {
            std::string triplet_file_name = it->path().stem().generic_u8string();
            if (t.canonical_name() == triplet_file_name) // TODO: fuzzy compare
            {
                //t.value = triplet_file_name; // NOTE: uncomment when implementing fuzzy compare
                return true;
            }
        }
        return false;
    }
}
