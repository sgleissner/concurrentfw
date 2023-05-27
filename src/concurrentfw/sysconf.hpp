/*
 * concurrentfw/sysconf.hpp
 *
 * (C) 2023 by Simon Gleissner <simon@gleissner.de>, http://concurrentfw.de
 *
 * This file is distributed under the MIT license, see file LICENSE.
 */

#pragma once
#ifndef CONCURRENTFW_SYSCONF_HPP
#define CONCURRENTFW_SYSCONF_HPP

#include <cstddef>
#include <system_error>

#include <unistd.h>

namespace ConcurrentFW
{

template<typename T>
T sysconf(int sysconf_name)
{
    errno = 0;
    long result = ::sysconf(sysconf_name);
    if ((result == -1) && (errno != 0))
        throw std::system_error(errno, std::system_category(), "error in sysconf()");
    return static_cast<T>(result);
}

size_t cache_line();
size_t page_size();

}  // namespace ConcurrentFW

#endif  // CONCURRENTFW_SYSCONF_HPP
