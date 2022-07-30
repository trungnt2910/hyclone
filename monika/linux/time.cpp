#include <cstddef>

#include "BeDefs.h"
#include "export.h"
#include "extended_commpage.h"
#include "haiku_errors.h"

extern "C"
{

// Fills the time zone offset in seconds in the field _timezoneOffset,
// and the name of the timezone in the buffer by name.
status_t MONIKA_EXPORT _kern_get_timezone(int32 *_timezoneOffset, char *name, size_t nameLength)
{
    if (GET_HOSTCALLS()->system_timezone(_timezoneOffset, name, nameLength) < 0)
    {
        return B_ERROR;
    }

    return B_OK;
}

}