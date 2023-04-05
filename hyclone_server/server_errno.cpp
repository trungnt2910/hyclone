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
        default:
            throw std::system_error(ec);
    }
}