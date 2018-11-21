/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_SESSION_STUB_H
#define NODE_SESSION_STUB_H

#include <smf/cluster/bus.h>
#include <smf/cluster/generator.h>

#include <cyng/log.h>
#include <cyng/vm/generator.h>
#include <cyng/object_cast.hpp>

namespace node 
{
	/**
	 * base class for sessions on cluster clients.
	 */
	class session_stub
	{
	protected:
		using read_buffer_t = std::array<char, NODE::PREFERRED_BUFFER_SIZE>;
		using read_buffer_iterator = read_buffer_t::iterator;
		using read_buffer_const_iterator = read_buffer_t::const_iterator;

		/**
		 * connection socket
		 */
		boost::asio::ip::tcp::socket socket_;

	private:

		/**
		 * Buffer for incoming data.
		 */
		read_buffer_t buffer_;

		/**
		 * session is in shutdown mode
		 */
		bool pending_;

	protected:

		/**
		 * scheduler 
		 */
		cyng::async::mux& mux_;

		/**
		 * The logger instance
		 */
		cyng::logging::log_ptr logger_;

		/**
		 * cluster bus
		 */
		bus::shared_type bus_;

		/**
		 * device specific functions and state
		 */
		cyng::controller vm_;	

	public:
		session_stub(boost::asio::ip::tcp::socket&& socket
			, cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, bus::shared_type bus
			, boost::uuids::uuid tag
			, std::chrono::seconds const& timeout);

		session_stub(session_stub const&) = delete;
		session_stub& operator=(session_stub const&) = delete;

		~session_stub();

		/**
		 * start reading from socket
		 */
		void start();

		/**
		 * close socket
		 */
		void close();

		cyng::controller& vm();
		cyng::controller const& vm() const;

		/**
		 * @return connection specific hash based in internal tag
		 */
		std::size_t hash() const noexcept;

	protected:
		virtual cyng::buffer_t parse(read_buffer_const_iterator, read_buffer_const_iterator) = 0;

		/**
		 * halt VM
		 */
		virtual void shutdown();

	private:
		void stop(boost::system::error_code ec);
		void do_read();
	};
}

//namespace cyng
//{
//	namespace traits
//	{
//
//#if defined(CYNG_LEGACY_MODE_ON)
//		const char type_tag<node::ipt::connection>::name[] = "ipt::connection";
//#endif
//	}	// traits	
//}


#include <cyng/intrinsics/traits.hpp>
//namespace cyng
//{
//	namespace traits
//	{
//		template <typename SESSION, typename SERIALIZER>
//		struct type_tag<node::connection_stub<SESSION, SERIALIZER>>
//		{
//			using type = node::connection_stub<SESSION, SERIALIZER>;
//			using tag = std::integral_constant<std::size_t, PREDEF_CONNECTION>;
//#if defined(CYNG_LEGACY_MODE_ON)
//			const static char name[];
//#else
//			constexpr static char name[] = "connection";
//#endif
//		};
//
//		template <>
//		struct reverse_type < PREDEF_CONNECTION >
//		{
//			//using type = node::connection_stub<SESSION, SERIALIZER>;
//			using type = void;
//		};
//	}
//}

#include <functional>
#include <boost/functional/hash.hpp>

//namespace std
//{
//	template<typename SESSION, typename SERIALIZER>
//	struct hash<node::connection_stub<SESSION, SERIALIZER>>
//	{
//		using argument_type = node::connection_stub<SESSION, SERIALIZER>;
//		inline size_t operator()(argument_type const& conn) const noexcept
//		{
//			return conn.hash();
//		}
//	};
//	template<typename SESSION, typename SERIALIZER>
//	struct equal_to<node::connection_stub<SESSION, SERIALIZER>>
//	{
//		using result_type = bool;
//		using first_argument_type = node::connection_stub<SESSION, SERIALIZER>;
//		using second_argument_type = node::connection_stub<SESSION, SERIALIZER>;
//
//		inline bool operator()(first_argument_type const& conn1, second_argument_type const& conn2) const noexcept
//		{
//			return conn1.hash() == conn2.hash();
//		}
//	};
//	template<typename SESSION, typename SERIALIZER>
//	struct less<node::connection_stub<SESSION, SERIALIZER>>
//	{
//		using result_type = bool;
//		using first_argument_type = node::connection_stub<SESSION, SERIALIZER>;
//		using second_argument_type = node::connection_stub<SESSION, SERIALIZER>;
//
//		inline bool operator()(first_argument_type const& conn1, second_argument_type const& conn2) const noexcept
//		{
//			return conn1.hash() < conn2.hash();
//		}
//	};
//}

#endif
