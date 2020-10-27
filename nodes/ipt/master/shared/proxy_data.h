/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_MASTER_PROXY_DATA_H
#define NODE_IPT_MASTER_PROXY_DATA_H

#include <smf/sml/protocol/generator.h>

#include <cyng/intrinsics/buffer.h>
#include <cyng/object.h>
#include <cyng/intrinsics/sets.h>
#include <cyng/dom/reader.h>

#include <queue>

#include <boost/uuid/uuid.hpp>

namespace node
{
	namespace proxy
	{
		/**
		 *	cluster communication
		 */
		struct cluster_data {

			cluster_data(
				boost::uuids::uuid,		//	[0] ident tag (target session)
				boost::uuids::uuid,		//	[1] source tag
				std::uint64_t,			//	[2] cluster seq
				boost::uuids::uuid,		//	[3] ws tag (origin)
				cyng::vector_t			//	[6] TGateway/Device PK
			);

			/**
			 * deep copy
			 */
			cluster_data copy() const;

			boost::uuids::uuid const tag_;		//!<	ident tag (target)
			boost::uuids::uuid const source_;	//!<	source tag
			std::uint64_t const seq_;			//!<	cluster seq
			boost::uuids::uuid const origin_;	//!<	ws tag (origin)
			cyng::vector_t const gw_;			//!<	TGateway/TDevice PK
		};

		/**
		 *	SML parameters
		 */
		struct sml_data {

			sml_data(
				sml::message_e,	//	[4] SML message type
				cyng::buffer_t,	//	[8] server id
				std::string,	//	[9] name
				std::string		//	[10] pwd
			);

			/**
			 * deep copy
			 */
			sml_data copy() const;

			/**
			 * replace server id with meter id
			 */
			sml_data(sml_data const&, cyng::buffer_t const&);

			/**
			 * @return true if message is of specified type
			 */
			bool is_msg_type(sml::message_e) const;

			/**
			 * Turns the message type string into the SML message type code
			 */
			sml::message_e const msg_code_;		//!<	SML message type
			cyng::buffer_t const srv_;			//!<	server id
			std::string const name_;			//!<	name
			std::string const pwd_;				//!<	pwd
		};

	}

	/**
	 * Contains all data to process an incoming response
	 * from an gateway.
	 */
	class reply_data
	{
	public:
		reply_data(
			proxy::cluster_data&&,
			proxy::sml_data&&,
			sml::obis_path_t path,
			bool					//	job - true if running as job
		);

		/**
		 * deep copy
		 */
		reply_data copy() const;

		/**
		 * Copy constructor
		 */
		reply_data(reply_data const&) = default;
		reply_data(reply_data&&) noexcept = default;

		proxy::cluster_data const& get_cluster_data() const;
		proxy::sml_data const& get_sml_data() const;

		/**
		 *@return OBIS path
		 */
		sml::obis_path_t const& get_path() const;

		/**
		 * @return true if a recursive query is running
		 */
		bool is_job() const;

	protected:
		//
		//	cluster communication
		//
		proxy::cluster_data const cluster_data_;

		//
		//	SML parameters
		//
		proxy::sml_data const sml_data_;

		/**
		 * the root SML path
		 */
		sml::obis_path_t const path_;

		bool const job_;	//!<	recursive query
	};


	namespace proxy
	{
		/**
		 * list of transaction id / message pairs
		 */
		//using messages_t = std::vector < cyng::tuple_t >;

		/**
		 * Define an output queue to store all data to send responses
		 * to SML requests back.
		 * 
		 * This map stores the relation between a transaction ID and the reply data
		 */
		using output_map = std::map<std::string, reply_data>;

		/**
		 * After conversion of an incoming SML request all collected data stored in this
		 * structure - ready to be placed in an input or output queue.
		 * 
		 * Only single SML message bodies supported. If multi SML message body support
		 * is requested replace the trx string with a vector or a list of strings
		 * with all transaction IDs
		 */
		struct data {
			data(sml::messages_t&&, reply_data&&, std::string const&);
			data(sml::messages_t&&, reply_data&&, std::initializer_list<std::string>);

			sml::messages_t const messages_;
			reply_data const reply_;
			std::vector<std::string> const trxs_;
		};

		/**
		 * Define an input queue to store all incoming SML requests
		 */
		using input_queue = std::queue< data >;

		data make_data(sml::messages_t&&, std::string const& trx, reply_data&&);
		data make_data(sml::messages_t&&, std::string const& trx, cluster_data&&,
			sml_data&&,
			sml::obis_path_t path,
			bool);
		data make_data(sml::messages_t&&, std::initializer_list<std::string> trx, cluster_data&&,
			sml_data&&,
			sml::obis_path_t path,
			bool);
	}

	/**
	 * Convert obj to string and parse it as an server ID
	 */
	cyng::object read_server_id(cyng::object obj);
	cyng::object parse_server_id(cyng::object obj);

	/**
	 * generate get proc parameter request messages
	 */
	CYNG_ATTR_NODISCARD proxy::data finalize_get_proc_parameter_request(
		proxy::cluster_data&&,
		proxy::sml_data&&,
		sml::obis_path_t&& path,		//	OBIS path
		cyng::tuple_reader const& reader,
		bool cache_enabled);

	CYNG_ATTR_NODISCARD proxy::data finalize_get_proc_parameter_request(
		proxy::cluster_data&&,
		proxy::sml_data&&,
		sml::obis code,		//	OBIS root code
		cyng::tuple_t const& params,
		bool cache_enabled);

	/**
	 * generate set proc parameter request messages
	 */
	CYNG_ATTR_NODISCARD proxy::data finalize_set_proc_parameter_request(
		proxy::cluster_data&&,
		proxy::sml_data&&,
		sml::obis code,		//	OBIS root code
		cyng::tuple_t const& params,
		bool cache_enabled);

	CYNG_ATTR_NODISCARD proxy::data finalize_set_proc_parameter_request(
		proxy::cluster_data&&,
		proxy::sml_data&&,
		sml::obis_path_t&& path,		//	OBIS path
		cyng::tuple_reader const& reader,
		bool cache_enabled);

	CYNG_ATTR_NODISCARD proxy::data finalize_get_profile_list_request_op_log(
		proxy::cluster_data&& cd,
		proxy::sml_data&& sd,
		sml::obis_path_t&& path,		//	OBIS path
		std::uint32_t hours,
		bool cache_enabled);

	CYNG_ATTR_NODISCARD proxy::data finalize_get_profile_list_request(
		proxy::cluster_data&&,
		proxy::sml_data&&,
		sml::obis code,		//	OBIS root code
		cyng::tuple_t const& params,
		bool cache_enabled);

	CYNG_ATTR_NODISCARD proxy::data finalize_get_list_request(
		proxy::cluster_data&&,
		proxy::sml_data&&,
		sml::obis code,		//	OBIS root code
		cyng::tuple_t const& params,
		bool cache_enabled);

	CYNG_ATTR_NODISCARD proxy::data finalize_get_list_request_current_data_record(
		proxy::cluster_data&&,
		proxy::sml_data&&,
		sml::obis_path_t&& path,		//	OBIS path
		cyng::tuple_reader const& reader,
		bool cache_enabled);

	/**
	 * preprocessing and cleanup of proxy data
	 */
	CYNG_ATTR_NODISCARD proxy::data make_proxy_data(boost::uuids::uuid tag,		//	[0] ident tag (target)
		boost::uuids::uuid source,	//	[1] source tag
		std::uint64_t seq,			//	[2] cluster seq
		boost::uuids::uuid origin,	//	[3] ws tag (origin)

		std::string const& channel,		//	[4] channel (SML message type)
		sml::obis code,					//	[5] OBIS root code
		cyng::vector_t const& gw,		//	[6] TGateway/TDevice PK
		cyng::tuple_t const& params,	//	[7] parameters

		cyng::buffer_t const& srv_id,	//	[8] server id
		std::string const& name,		//	[9] name
		std::string const& pwd,			//	[10] pwd
		bool cache_enabled				//	[11] use cache
	);

	/**
	 * @return MAC of system or random MAC if data not available
	 */
	cyng::mac48 get_mac();

	//
	//	generator functions
	//

	/**
	 * set IP-T communicatin parameters
	 *
	 * @return SML message
	 */
	CYNG_ATTR_NODISCARD cyng::tuple_t set_proc_parameter_ipt_param(cyng::tuple_t&& msg
		, cyng::buffer_t const& server_id
		, std::uint8_t idx
		, cyng::tuple_t&& tpl);

	CYNG_ATTR_NODISCARD cyng::tuple_t set_proc_parameter_activate_device(cyng::tuple_t&& msg
		, cyng::buffer_t const& server_id
		, std::uint8_t idx
		, std::string&& meter);

	CYNG_ATTR_NODISCARD cyng::tuple_t set_proc_parameter_deactivate_device(cyng::tuple_t&& msg
		, cyng::buffer_t const& server_id
		, std::uint8_t idx
		, std::string&& meter);

	CYNG_ATTR_NODISCARD cyng::tuple_t set_proc_parameter_delete_device(cyng::tuple_t&& msg
		, cyng::buffer_t const& server_id
		, std::uint8_t idx
		, std::string&& meter);

	CYNG_ATTR_NODISCARD cyng::tuple_t set_proc_parameter_sensor_params(cyng::tuple_t&& msg
		, cyng::buffer_t const& server_id
		, std::string&& user
		, std::string&& pwd
		, std::string&& pubKey
		, std::string&& aesKey);

	CYNG_ATTR_NODISCARD cyng::tuple_t set_proc_parameter_broker(cyng::tuple_t&& msg
		, cyng::buffer_t const& server_id
		, cyng::vector_t&&);

	/**
	 * set wireless M-Bus protocol type
	 * <ul>
	 * <li>0 = T-Mode</li>
	 * <li>1 = S-Mode</li>
	 * <li>2 = S/T-Automatik (wechselnd)</li>
	 * <li>3 = S/T-Automatik  (Parallelbetrieb)</li>
	 * </ul>
	 *
	 * @return transaction ID
	 */
	CYNG_ATTR_NODISCARD cyng::tuple_t set_proc_parameter_if_wmbus(cyng::tuple_t&& msg
		, cyng::buffer_t const& server_id
		, cyng::tuple_t&& tpl);

	CYNG_ATTR_NODISCARD cyng::tuple_t set_proc_parameter_if_1107(cyng::tuple_t&& msg
		, cyng::buffer_t const& server_id
		, cyng::tuple_t&& tpl);

	CYNG_ATTR_NODISCARD cyng::tuple_t set_proc_parameter_hardware_port(cyng::tuple_t&& msg
		, cyng::buffer_t const& server_id
		, std::string
		, std::uint8_t idx
		, cyng::tuple_t);
}


#endif

