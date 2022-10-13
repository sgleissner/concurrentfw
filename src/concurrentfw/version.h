/*
 * concurrentfw/version.h
 *
 * (C) 2017-2022 by Simon Gleissner <simon@gleissner.de>, http://concurrentfw.de
 *
 * This file is distributed under the ISC license, see file LICENSE.
 */

#ifndef CONCURRENTFW_VERSION_H_
#define CONCURRENTFW_VERSION_H_

#include <cstdint>

#include <concurrentfw/generated_config.h>	// gets generated constants from CMakeLists.txt

namespace ConcurrentFW
{

struct Version
{
	static constexpr uint16_t MAJOR {CONCURRENTFW_VERSION_MAJOR};
	static constexpr uint16_t MINOR {CONCURRENTFW_VERSION_MINOR};
	static constexpr uint16_t PATCH {CONCURRENTFW_VERSION_PATCH};
	// CONCURRENTFW_VERSION_TWEAK not used
};

}  // namespace ConcurrentFW

#endif /* CONCURRENTFW_VERSION_H_ */
