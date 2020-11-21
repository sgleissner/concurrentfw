/*
 * concurrentfw/version.h
 *
 * (C) 2017-2020 by Simon Gleissner <simon@gleissner.de>, http://concurrentfw.de
 *
 * This file is distributed under the ISC license, see file LICENSE.
 */

#ifndef CONCURRENTFW_VERSION_H_
#define CONCURRENTFW_VERSION_H_

#include <cstdint>

#include <concurrentfw/generated_config.h>	// get generated constants from configure.ac

namespace ConcurrentFW
{
	namespace VersionParser
	{
		static constexpr uint16_t parse_int(const char* const version, const uint16_t version_int)
		{
			return (version[0]<'0' || version[0]>'9') ? version_int :
					parse_int(&version[1], (version_int*10)+(version[0]-'0'));
		}

		static constexpr uint16_t search_int(const char* const version, const int part)
		{
			return (part==0 || version[0]=='\0')
						? parse_int(version,0)
						: search_int(&version[1], version[0]=='.' ? part-1 : part);
		}
	} // namespace ConcurrentFW::VersionParser

	enum Version : uint16_t
	{
		MAJOR    = VersionParser::search_int(CONCURRENTFW_PACKAGE_VERSION,0),
		MINOR    = VersionParser::search_int(CONCURRENTFW_PACKAGE_VERSION,1),
		REVISION = VersionParser::search_int(CONCURRENTFW_PACKAGE_VERSION,2)
	};

}	// namespace ConcurrentFW

#endif /* CONCURRENTFW_VERSION_H_ */
