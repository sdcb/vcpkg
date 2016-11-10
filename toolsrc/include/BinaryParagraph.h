#pragma once

#include <unordered_map>
#include "SourceParagraph.h"
#include "package_spec.h"

namespace vcpkg
{
    struct BinaryParagraph
    {
        BinaryParagraph();
        explicit BinaryParagraph(std::unordered_map<std::string, std::string> fields);
        BinaryParagraph(const SourceParagraph& spgh, const triplet& target_triplet);

        std::string displayname() const;

        std::string fullstem() const;

        std::string dir() const;

        package_spec spec;
        std::string version;
        std::string description;
        std::string maintainer;
        std::vector<std::string> depends;
    };

    std::ostream& operator<<(std::ostream& os, const BinaryParagraph& pgh);
}
