#ifndef __LIBHPKG_HPKEXCEPTION_H__
#define __LIBHPKG_HPKEXCEPTION_H__

#include <exception>

namespace LibHpkg
{
    class HpkException : public std::exception
    {
    private:
        std::string message;
    public:
        HpkException(const std::string& message)
            : message(message)
        {
        }

        HpkException(const std::string& message, const std::exception& cause)
            : message(message + ": " + cause.what())
        {
        }

        virtual const char* what() const noexcept override
        {
            return message.c_str();
        }
    };
}

#endif // __LIBHPKG_HPKEXCEPTION_H__