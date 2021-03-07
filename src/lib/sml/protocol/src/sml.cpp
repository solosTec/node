#include <smf/sml.h>

#ifdef _DEBUG_SML
#include <iostream>
#endif

#include <boost/assert.hpp>

namespace smf {
	namespace sml {

		const char* get_name(sml_type type) {
			switch (type) {
			case sml_type::BINARY:	return "BINARY";
			case sml_type::BOOLEAN:	return "BOOLEAN";
			case sml_type::INTEGER:	return "INTEGER";
			case sml_type::UNSIGNED:	return "UNSIGNED";
			case sml_type::LIST:	return "LIST";
			case sml_type::OPTIONAL:	return "OPTIONAL";
			case sml_type::EOM:	return "EOM";
			case sml_type::RESERVED:	return "RESERVED";
			default:
				break;
			}
			return "unknown";
		}

	}
}
