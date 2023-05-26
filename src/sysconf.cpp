//
// Created by simon on 22.02.23.
//

#include <climits>


#include <concurrentfw/sysconf.hpp>

namespace ConcurrentFW
{

size_t cache_line()
{
    static const size_t value = sysconf<size_t>(_SC_LEVEL1_DCACHE_LINESIZE);
    return value;
}

size_t page_size()
{
    static const size_t value = sysconf<size_t>(_SC_PAGESIZE);
    return value;
}

}  // namespace ConcurrentFW
