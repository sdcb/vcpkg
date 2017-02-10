#include "pch.h"
#include "package_spec.h"
#include "package_spec_parse_result.h"

namespace vcpkg
{
    const char* package_spec_parse_result_category_impl::name() const noexcept
    {
        return "package_spec_parse_result";
    }

    std::string package_spec_parse_result_category_impl::message(int ev) const noexcept
    {
        switch (static_cast<package_spec_parse_result>(ev))
        {
            case package_spec_parse_result::SUCCESS:
                return "OK";
            case package_spec_parse_result::TOO_MANY_COLONS:
                return "Too many colons";
            case package_spec_parse_result::INVALID_CHARACTERS:
                return "Contains invalid characters. Only alphanumeric lowercase ASCII characters and dashes are allowed";
            default:
                Checks::unreachable();
        }
    }

    const std::error_category& package_spec_parse_result_category()
    {
        static package_spec_parse_result_category_impl instance;
        return instance;
    }

    std::error_code make_error_code(package_spec_parse_result e)
    {
        return std::error_code(static_cast<int>(e), package_spec_parse_result_category());
    }

    package_spec_parse_result to_package_spec_parse_result(int i)
    {
        return static_cast<package_spec_parse_result>(i);
    }

    package_spec_parse_result to_package_spec_parse_result(std::error_code ec)
    {
        return to_package_spec_parse_result(ec.value());
    }
}
