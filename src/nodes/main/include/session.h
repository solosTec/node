/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MAIN_SESSION_H
#define SMF_MAIN_SESSION_H

#include <cyng/log/logger.h>
#include <cyng/io/parser/parser.h>
#include <cyng/vm/proxy.h>
#include <cyng/obj/intrinsics/pid.h>
#include <cyng/store/slot_interface.h>

#include <memory>
#include <array>

namespace smf {

	class server;
	class session : public std::enable_shared_from_this<session>
	{
		class slot : public cyng::slot_interface {
		public:

			slot(session*);

			virtual bool forward(cyng::table const*
				, cyng::key_t const&
				, cyng::data_t const&
				, std::uint64_t
				, boost::uuids::uuid) override;

			virtual bool forward(cyng::table const* tbl
				, cyng::key_t const& key
				, cyng::attr_t const& attr
				, std::uint64_t gen
				, boost::uuids::uuid tag) override;

			virtual bool forward(cyng::table const* tbl
				, cyng::key_t const& key
				, boost::uuids::uuid tag) override;

			virtual bool forward(cyng::table const*
				, boost::uuids::uuid) override;

			virtual bool forward(cyng::table const*
				, bool) override;


		private:
			session* sp_;
		};

	public:
		session(boost::asio::ip::tcp::socket socket, server*, cyng::logger);
		~session();

		void start();
		void stop();

		/**
		 * @return tag of remote session
		 */
		boost::uuids::uuid get_peer() const;

	private:
		cyng::vm_proxy init_vm(server*);
		void do_read();
		void do_write();
		void handle_write(const boost::system::error_code& ec);

		void cluster_login(std::string
			, std::string
			, cyng::pid
			, std::string node
			, boost::uuids::uuid tag
			, cyng::version v);

		void db_req_subscribe(std::string, boost::uuids::uuid tag);

		void db_req_insert(std::string const& table_name
			, cyng::key_t key
			, cyng::data_t data
			, std::uint64_t generation
			, boost::uuids::uuid);

		/**
		 * table merge()
		 */
		void db_req_update(std::string const& table_name
			, cyng::key_t key
			, cyng::param_map_t
			, boost::uuids::uuid);

		/**
		 * table erase()
		 */
		void db_req_remove(std::string const& table_name
			, cyng::key_t key
			, boost::uuids::uuid source);

		/**
		 * table clear()
		 */
		void db_req_clear(std::string const& table_name
			, boost::uuids::uuid source);

		/**
		 * pty login
		 */
		void pty_login(boost::uuids::uuid
			, std::string
			, std::string
			, boost::asio::ip::tcp::endpoint
			, std::string);

		//
		//	generate VM channel functions
		//

		//	"cluster.req.login"
		static std::function<void(std::string
			, std::string
			, cyng::pid
			, std::string
			, boost::uuids::uuid
			, cyng::version)> 
		get_vm_func_cluster_req_login(session*); 

		//	"db.req.subscribe"
		static std::function<void(std::string
			, boost::uuids::uuid tag)> 
		get_vm_func_db_req_subscribe(session*); 

		//	"db.req.insert"
		static std::function<void(std::string
			, cyng::key_t
			, cyng::data_t
			, std::uint64_t
			, boost::uuids::uuid)> 
		get_vm_func_db_req_insert(session*);

		//	"db.req.update" aka merge()
		static std::function<void(std::string
			, cyng::key_t
			, cyng::param_map_t
			, boost::uuids::uuid)> 
		get_vm_func_db_req_update(session*);

		//	"db.req.remove"
		static std::function<void(std::string
			, cyng::key_t
			, boost::uuids::uuid)> 
		get_vm_func_db_req_remove(session*);

		//	"db.req.clear"
		static std::function<void(std::string
			, boost::uuids::uuid)> 
		get_vm_func_db_req_clear(session*);

		static std::function<void(boost::uuids::uuid
			, std::string
			, std::string
			, boost::asio::ip::tcp::endpoint
			, std::string)>
		get_vm_func_pty_login(session*);

		static std::function<void(boost::uuids::uuid
			, std::string)>
		get_vm_func_pty_connect(server*);

	private:
		boost::asio::ip::tcp::socket socket_;
		server* srvp_;
		cyng::logger logger_;

		/**
		 * Buffer for incoming data.
		 */
		std::array<char, 2048>	buffer_;

		/**
		 * Buffer for outgoing data.
		 */
		std::deque<cyng::buffer_t>	buffer_write_;

		cyng::vm_proxy	vm_;
		cyng::io::parser parser_;

		cyng::slot_ptr	slot_;

		/**
		 * remote session tag
		 */
		boost::uuids::uuid peer_;
		std::string protocol_layer_;
	};

}

#endif
