#include "BinaryParagraph.h"
#include "vcpkglib_helpers.h"
#include "vcpkg_Checks.h"

using namespace vcpkg::details;

namespace vcpkg
{
    //
    namespace BinaryParagraphRequiredField
    {
        static const std::string PACKAGE = "Package";
        static const std::string VERSION = "Version";
        static const std::string ARCHITECTURE = "Architecture";
        static const std::string MULTI_ARCH = "Multi-Arch";
    }

    namespace BinaryParagraphOptionalField
    {
        static const std::string DESCRIPTION = "Description";
        static const std::string MAINTAINER = "Maintainer";
        static const std::string DEPENDS = "Depends";
    }

    static const std::vector<std::string>& get_list_of_valid_fields()
    {
        static const std::vector<std::string> valid_fields =
        {
            BinaryParagraphRequiredField::PACKAGE,
            BinaryParagraphRequiredField::VERSION,
            BinaryParagraphRequiredField::ARCHITECTURE,

            BinaryParagraphOptionalField::DESCRIPTION,
            BinaryParagraphOptionalField::MAINTAINER,
            BinaryParagraphOptionalField::DEPENDS
        };

        return valid_fields;
    }

    BinaryParagraph::BinaryParagraph() = default;

    BinaryParagraph::BinaryParagraph(std::unordered_map<std::string, std::string> fields)
    {
        const std::string name = details::remove_required_field(&fields, BinaryParagraphRequiredField::PACKAGE);
        const std::string architecture = details::remove_required_field(&fields, BinaryParagraphRequiredField::ARCHITECTURE);
        const triplet target_triplet = triplet::from_canonical_name(architecture);

        this->spec = package_spec::from_name_and_triplet(name, target_triplet).get_or_throw();
        this->version = details::remove_required_field(&fields, BinaryParagraphRequiredField::VERSION);

        this->description = details::remove_optional_field(&fields, BinaryParagraphOptionalField::DESCRIPTION);
        this->maintainer = details::remove_optional_field(&fields, BinaryParagraphOptionalField::MAINTAINER);

        std::string multi_arch = details::remove_required_field(&fields, BinaryParagraphRequiredField::MULTI_ARCH);
        Checks::check_exit(multi_arch == "same", "Multi-Arch must be 'same' but was %s", multi_arch);

        std::string deps = details::remove_optional_field(&fields, BinaryParagraphOptionalField::DEPENDS);
        this->depends = parse_depends(deps);
    }

    BinaryParagraph::BinaryParagraph(const SourceParagraph& spgh, const triplet& target_triplet)
    {
        this->spec = package_spec::from_name_and_triplet(spgh.name, target_triplet).get_or_throw();
        this->version = spgh.version;
        this->description = spgh.description;
        this->maintainer = spgh.maintainer;
        this->depends = filter_dependencies(spgh.depends, target_triplet);
    }

    std::string BinaryParagraph::displayname() const
    {
        return Strings::format("%s:%s", this->spec.name(), this->spec.target_triplet());
    }

    std::string BinaryParagraph::dir() const
    {
        return this->spec.dir();
    }

    std::string BinaryParagraph::fullstem() const
    {
        return Strings::format("%s_%s_%s", this->spec.name(), this->version, this->spec.target_triplet());
    }

    std::ostream& operator<<(std::ostream& os, const BinaryParagraph& p)
    {
        os << "Package: " << p.spec.name() << "\n";
        os << "Version: " << p.version << "\n";
        if (!p.depends.empty())
        {
            os << "Depends: " << p.depends.front();

            auto b = p.depends.begin() + 1;
            auto e = p.depends.end();
            for (; b != e; ++b)
            {
                os << ", " << *b;
            }

            os << "\n";
        }
        os << "Architecture: " << p.spec.target_triplet() << "\n";
        os << "Multi-Arch: same\n";
        if (!p.maintainer.empty())
            os << "Maintainer: " << p.maintainer << "\n";
        if (!p.description.empty())
            os << "Description: " << p.description << "\n";
        return os;
    }
}
