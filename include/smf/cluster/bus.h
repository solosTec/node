/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_CLUSTER_BUS_H
#define SMF_CLUSTER_BUS_H


#include <smf/cluster/config.h>

#include <cyng/log/logger.h>
#include <cyng/io/parser/parser.h>
#include <cyng/obj/intrinsics/buffer.h>
#include <cyng/vm/proxy.h>
#include <cyng/vm/vm_fwd.h>
#include <cyng/store/key.hpp>

namespace smf
{
	/**
	 * The bus interface defines the requirements of any kind of cluster
	 * member that acts as a client.
	 */
	class bus_interface {
	public:
		virtual cyng::mesh* get_fabric() = 0;

		virtual void on_login(bool) = 0;

		virtual void db_res_insert(std::string
			, cyng::key_t  key
			, cyng::data_t  data
			, std::uint64_t gen
			, boost::uuids::uuid tag) = 0;

		virtual void db_res_trx(std::string
			, bool) = 0;

		virtual void db_res_update(std::string
			, cyng::key_t key
			, cyng::attr_t attr
			, std::uint64_t gen
			, boost::uuids::uuid tag) = 0;

		virtual void db_res_remove(std::string
			, cyng::key_t key
			, boost::uuids::uuid tag) = 0;

		virtual void db_res_clear(std::string
			, boost::uuids::uuid tag) = 0;

	};

	/**
	 * The TCP/IP behaviour is modelled after the async_tcp_client.cpp" example
	 * in the asio C++11 example folder (libs/asio/example/cpp11/timeouts/async_tcp_client.cpp)
	 */
	class bus
	{
		enum class state {
			START,
			WAIT,
			CONNECTED,
			STOPPED,
		} state_;
	public:
		bus(boost::asio::io_context& ctx
			, cyng::logger
			, toggle::server_vec_t&& cfg
			, std::string const& node_name
			, boost::uuids::uuid tag
			, bus_interface* bip);

		void start();
		void stop();

		boost::uuids::uuid get_tag() const;

		constexpr bool is_connected() const {
			return state_ == state::CONNECTED;
		}

		//
		//	cluster client functions
		//
		void req_subscribe(std::string table_name);
		void req_db_insert(std::string const& table_name
			, cyng::key_t  key
			, cyng::data_t  data
			, std::uint64_t generation);

		/**
		 * triggers a merge() on the receiver side
		 */
		void req_db_update(std::string const&
			, cyng::key_t  key
			, cyng::param_map_t data);

		void req_db_remove(std::string const&
			, cyng::key_t);

		void req_db_clear(std::string const&);

		//	"pty.req.login"
		void pty_login(std::string name
			, std::string pwd
			, boost::uuids::uuid tag
			, std::string data_layer
			, boost::asio::ip::tcp::endpoint ep);

		//	"pty.connect"
		void pty_connect(std::string msisdn);

		//void pty_reg_target(std::string name);

	private:
		void reset();
		void connect(boost::asio::ip::tcp::resolver::results_type endpoints);
		void start_connect(boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter);
		void handle_connect(boost::system::error_code const&,
			boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter);
		void do_read();
		void do_write();
		void handle_read(boost::system::error_code const&, std::size_t n);
		void handle_write(boost::system::error_code const&);
		void check_deadline(boost::system::error_code const&);

		constexpr bool is_stopped() const {
			return state_ == state::STOPPED;
		}

		/**
		 * insert external functions
		 */
		cyng::vm_proxy init_vm(bus_interface*);

		//
		//	generate VM channel functions
		//

		static std::function<void(bool)> 
		get_vm_func_on_login(bus_interface*);

		//	subscribe (1)
		static std::function<void(std::string
			, cyng::key_t
			, cyng::data_t
			, std::uint64_t
			, boost::uuids::uuid )> 
		get_vm_func_db_res_insert(bus_interface*);

		//	trx (2)
		static std::function<void(std::string
			, bool)>
		get_vm_func_db_res_trx(bus_interface*);

		//	update (3)
		static std::function<void(std::string
			, cyng::key_t
			, cyng::attr_t
			, std::uint64_t
			, boost::uuids::uuid)> 
		get_vm_func_db_res_update(bus_interface*);

		//	remove (4)
		static std::function<void(std::string
			, cyng::key_t key
			, boost::uuids::uuid)> 
		get_vm_func_db_res_remove(bus_interface*);

		//	clear (5)
		static std::function<void(std::string
			, boost::uuids::uuid)> 
		get_vm_func_db_res_clear(bus_interface*);

	private:
		boost::asio::io_context& ctx_;
		cyng::logger logger_;
		toggle const tgl_;
		std::string const node_name_;
		boost::uuids::uuid const tag_;
		bus_interface* bip_;

		boost::asio::ip::tcp::resolver::results_type endpoints_;
		boost::asio::ip::tcp::socket socket_;
		boost::asio::steady_timer timer_;
		std::deque<cyng::buffer_t>	buffer_write_;
		std::array<char, 2048>	input_buffer_;
		cyng::vm_proxy	vm_;
		cyng::io::parser parser_;
	};
}


#endif
