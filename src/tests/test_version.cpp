/*
 * test_version.cpp
 *
 * (C) 2017-2022 by Simon Gleissner <simon@gleissner.de>, http://concurrentfw.de
 *
 * This file is distributed under the ISC license, see file LICENSE.
 */

#include <catch2/catch_test_macros.hpp>

#include <concurrentfw/version.hpp>

TEST_CASE("check of version number", "[version]")
{
	INFO(
		"version: " << ConcurrentFW::Version::MAJOR << "." << ConcurrentFW::Version::MINOR << "."
					<< ConcurrentFW::Version::PATCH
	);
	CHECK(ConcurrentFW::Version::MAJOR == 0);
	CHECK(ConcurrentFW::Version::MINOR == 0);
	CHECK(ConcurrentFW::Version::PATCH == 4);
}
