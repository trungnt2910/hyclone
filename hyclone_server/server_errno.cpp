#include "haiku_errors.h"
#include "server_errno.h"

int CppToB(const std::error_code& ec)
{
    switch ((std::errc)ec.value())
    {
        case std::errc::no_such_file_or_directory:
            return B_ENTRY_NOT_FOUND;
        case std::errc::permission_denied:
            return B_PERMISSION_DENIED;
        case std::errc::not_a_directory:
            return B_NOT_A_DIRECTORY;
        case std::errc::filename_too_long:
            return B_NAME_TOO_LONG;
        case std::errc::too_many_symbolic_link_levels:
            return B_LINK_LIMIT;
        default:
            throw std::system_error(ec);
    }
}