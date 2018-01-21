/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/cluster/serializer.h>
#include <cyng/vm/generator.h>

namespace node
{
	serializer::serializer(boost::asio::ip::tcp::socket& s, cyng::controller& vm)
		: buffer_()
		, ostream_(&buffer_)
	{
		vm.async_run(cyng::register_function("stream.flush", 0, [this, &s](cyng::context& ctx) {

#ifdef SMF_IO_DEBUG
			//	get content of buffer
			//boost::asio::const_buffer cbuffer(*buffer_.data().begin());
			//const char* p = boost::asio::buffer_cast<const char*>(cbuffer);
			//const std::size_t size = boost::asio::buffer_size(cbuffer);

			//cyy::io::hex_dump hd;
			//hd(std::cerr, p, p + size);
#endif

			boost::system::error_code ec;
			boost::asio::write(s, buffer_, ec);
			ctx.set_register(ec);
		}));

		vm.async_run(cyng::register_function("stream.serialize", 0, [this](cyng::context& ctx) {

			const cyng::vector_t frame = ctx.get_frame();
			for (auto obj : frame)
			{
				//cyng::io::serialize_plain(std::cerr, obj);
				cyng::io::serialize_binary(ostream_, obj);
			}

		}));

	}

}
