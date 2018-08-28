/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_SML_BUS_SERIALIZER_H
#define NODE_SML_BUS_SERIALIZER_H

#include <NODE_project_info.h>
#include <cyng/vm/controller.h>
#include <cyng/io/serializer.h>
#include <boost/asio.hpp>

namespace node
{
	namespace sml
	{
		class serializer
		{
		public:
			serializer(boost::asio::ip::tcp::socket& s, cyng::controller& vm);

		private:
			boost::asio::streambuf buffer_;
			std::ostream ostream_;

		};
	}
}

#endif
