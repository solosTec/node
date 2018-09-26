/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_MASTER_CLIENT_H
#define NODE_MASTER_CLIENT_H

#include <cyng/async/mux.h>
#include <cyng/log.h>
#include <cyng/store/db.h>
#include <cyng/table/key.hpp>
#include <cyng/vm/context.h>
#include <cyng/io/serializer.h>
#include <atomic>
#include <boost/random.hpp>
#include <boost/uuid/random_generator.hpp>

namespace node 
{
	std::pair<cyng::table::key_list_t, std::uint16_t> collect_matching_targets(const cyng::store::table* tbl_target
		, boost::uuids::uuid
		, std::string const& name);

	std::tuple<std::uint32_t, std::string, boost::uuids::uuid> get_source_channel(const cyng::store::table* tbl_session, boost::uuids::uuid);
	std::size_t remove_targets_by_tag(cyng::store::table* tbl_target, boost::uuids::uuid);
	cyng::table::key_list_t get_targets_by_peer(cyng::store::table const* tbl_target, boost::uuids::uuid);
	cyng::table::key_list_t get_channels_by_peer(cyng::store::table const* tbl_channel, boost::uuids::uuid);

	class client
	{
		friend class connection;

	public:
		client(cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, cyng::store::db&
			, std::atomic<std::uint64_t>& global_configuration
			, boost::filesystem::path stat_dir);

		client(client const&) = delete;
		client& operator=(client const&) = delete;

		void req_login(cyng::context& ctx
			, boost::uuids::uuid	//	[0] remote client tag
			, boost::uuids::uuid	//	[1] peer tag
			, std::uint64_t			//	[2] sequence number
			, std::string			//	[3] name
			, std::string			//	[4] pwd/credential
			, std::string			//	[5] authorization scheme
			, cyng::param_map_t const&		//	[6] bag)
			, cyng::object session);

		void req_close(cyng::context& ctx
			, boost::uuids::uuid		//	[0] remote client tag
			, boost::uuids::uuid	//	[1] peer tag
			, std::uint64_t			//	[2] sequence number
			, boost::uuids::uuid self
			, bool);

		void req_open_connection(cyng::context& ctx
			, boost::uuids::uuid	//	[0] remote client tag
			, boost::uuids::uuid	//	[1] peer tag
			, std::uint64_t			//	[2] sequence number
			, std::string			//	[3] number
			, cyng::param_map_t const&		//	[4] bag)
			, cyng::object self);

		void res_open_connection(cyng::context& ctx
			, boost::uuids::uuid		//	[0] origin client tag
			, boost::uuids::uuid	//	[1] peer tag
			, std::uint64_t			//	[2] sequence number
			, bool					//	[3] success
			, cyng::param_map_t const&		//	[4] options
			, cyng::param_map_t const&);		//	[5] bag);

		void res_close_connection(cyng::context& ctx
			, boost::uuids::uuid	//	[0] origin client tag
			, boost::uuids::uuid	//	[1] peer tag
			, std::uint64_t			//	[2] sequence number
			, bool					//	[3] success
			, cyng::param_map_t const&		//	[4] options
			, cyng::param_map_t const&);		//	[5] bag

		void req_transmit_data(cyng::context& ctx
			, boost::uuids::uuid		//	[0] origin client tag
			, boost::uuids::uuid		//	[1] peer tag
			, std::uint64_t			//	[2] sequence number
			, cyng::param_map_t const&		//	[5] bag
			, cyng::object);	//	data

		void req_close_connection(cyng::context& ctx
			, boost::uuids::uuid	//	[0] remote client tag
			, boost::uuids::uuid	//	[1] peer tag
			, bool shutdown			//	[2] shutdown mode
			, std::uint64_t			//	[3] sequence number
			, cyng::param_map_t const&);		//	[4] bag)

		void req_open_push_channel(cyng::context& ctx
			, boost::uuids::uuid		//	[0] remote client tag
			, boost::uuids::uuid		//	[1] peer tag
			, std::uint64_t			//	[2] sequence number
			, std::string			//	[3] target name
			, std::string			//	[4] device name
			, std::string			//	[5] device number
			, std::string			//	[6] device software version
			, std::string			//	[7] device id
			, std::chrono::seconds	//	[8] timeout
			, cyng::param_map_t const&		//	[9] bag
			, boost::uuids::uuid);	//	[10] local session tag

		void req_close_push_channel(cyng::context& ctx
			, boost::uuids::uuid tag
			, boost::uuids::uuid peer
			, std::uint64_t seq			//	[2] sequence number
			, std::uint32_t channel		//	[3] channel id
			, cyng::param_map_t const& bag);

		void req_transfer_pushdata(cyng::context& ctx
			, boost::uuids::uuid		//	[0] origin client tag
			, boost::uuids::uuid		//	[1] peer tag
			, std::uint64_t			//	[2] sequence number
			, std::uint32_t			//	[3] channel
			, std::uint32_t			//	[4] source
			, cyng::param_map_t const&		//	[5] bag
			, cyng::object);			//	data

		void req_register_push_target(cyng::context& ctx
			, boost::uuids::uuid		//	[0] remote client tag
			, boost::uuids::uuid		//	[1] remote peer tag
			, std::uint64_t			//	[2] sequence number
			, std::string			//	[3] target name
			, cyng::param_map_t const&
			, boost::uuids::uuid);	//	[5] local session tag

		void req_deregister_push_target(cyng::context& ctx
			, boost::uuids::uuid	//	[0] remote client tag
			, boost::uuids::uuid	//	[1] remote peer tag
			, std::uint64_t			//	[2] sequence number
			, std::string			//	[3] target name
			, cyng::param_map_t const&
			, boost::uuids::uuid);	//	[5] local session tag

		void update_attr(cyng::context& ctx
			, boost::uuids::uuid	//	[0] origin client tag
			, boost::uuids::uuid	//	[1] peer tag
			, std::uint64_t			//	[2] sequence number
			, std::string			//	[3] name
			, cyng::object			//	[4] value
			, cyng::param_map_t);

		bool set_connection_auto_login(cyng::object);
		bool set_connection_auto_enabled(cyng::object);
		bool set_connection_superseed(cyng::object);

		/**
		 * Turn generating time series on or off
		 *
		 * @return previous value
		 */
		bool set_generate_time_series(cyng::object);

		/**
		 * @return true if generating time series is on.
		 */
		bool is_generate_time_series() const;

		void set_class(std::string const&);
		bool open_stat(std::ofstream&, std::string const& account);

		template <typename T>
		void write_stat(boost::uuids::uuid tag, std::string const& account, std::string const& evt, T&& value)
		{
			if (is_generate_time_series())
			{

				std::ofstream of;
				if (open_stat(of, account))
				{

					//
					//	want to use stream operator from namespace cyng
					// 	cyng::operator<<(os, rec.tp_);
					//
					using cyng::operator<<;
					of
						<< std::chrono::system_clock::now()
						<< ';'
						<< tag
						<< ';'
						<< '"'
						<< evt
						<< '"'
						<< ';'
						<< value
						<< std::endl
						;
				}
				else {
					//
					//	want to use stream operator from namespace cyng
					// 	cyng::operator<<(os, rec.tp_);
					//
					using cyng::operator<<;
					//CYNG_LOG_TRACE(logger_, "session.write.stat "
					//	<< tag
					//	<< ';'
					//	<< '"'
					//	<< evt
					//	<< '"'
					//	<< ';'
					//	<< value);
				}
			}
		}

	private:
		void req_open_push_channel_empty(cyng::context& ctx
			, boost::uuids::uuid tag
			, std::uint64_t seq
			, cyng::param_map_t const& bag);

		bool check_auth_state(cyng::store::table*, boost::uuids::uuid);
		bool check_online_state(cyng::context& ctx, cyng::store::table*, std::string const&);
		std::tuple<bool, bool, boost::uuids::uuid, std::uint32_t> test_credential(cyng::context& ctx, const cyng::store::table*, std::string const&, std::string const&);

		cyng::table::key_list_t get_clients_by_peer(const cyng::store::table* tbl_session, boost::uuids::uuid);

		bool create_channel(cyng::context& ctx
			, cyng::store::table* tbl_channel
			, const cyng::store::table* tbl_session
			, cyng::store::table* tbl_msg
			, std::string const& name
			, std::uint32_t source_channel
			, std::uint32_t channel
			, cyng::table::record const& target_rec
			, boost::uuids::uuid tag
			, boost::uuids::uuid self
			, std::chrono::seconds timeout
			, std::uint16_t max_packet_size
			, std::size_t count);

	private:
		cyng::async::mux& mux_;
		cyng::logging::log_ptr logger_;
		cyng::store::db& db_;
		std::atomic<std::uint64_t>& global_configuration_;
		const boost::filesystem::path stat_dir_;

		/**
		 * transport layer
		 */
		std::string node_class_;

		boost::random::mt19937 rng_;
		boost::random::uniform_int_distribution<std::uint32_t> distribution_;
		boost::uuids::random_generator uuid_gen_;

	};

}



#endif

