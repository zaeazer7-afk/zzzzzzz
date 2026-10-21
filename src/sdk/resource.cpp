#include "resource.h"

#include <Windows.h>

namespace fx
{
ResourceImpl* Resource::get_impl()
{
    return (ResourceImpl*)&((uint64_t*)this)[0];
}
}
