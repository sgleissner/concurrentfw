//
// Created by simon on 22.02.23.
//

#pragma once
#ifndef SYSCONF_HPP
#define SYSCONF_HPP

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
	return static_cast<size_t>(result);
}

size_t cache_line();
size_t page_size();

}  // namespace ConcurrentFW

#endif	// SYSCONF_HPP
