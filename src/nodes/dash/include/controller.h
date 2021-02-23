/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/controller_base.h>
#include <smf/cluster/config.h>

 namespace smf {

	class controller : public config::controller_base
	{
	public:
		controller(config::startup const&);

	protected:
		cyng::vector_t create_default_config(std::chrono::system_clock::time_point&& now
			, std::filesystem::path&& tmp
			, std::filesystem::path&& cwd) override;
		void run(cyng::controller&, cyng::logger, cyng::object const& cfg) override;

	private:
		cyng::param_t create_server_spec(std::filesystem::path const& root);
		cyng::param_t create_cluster_spec();

		void join_cluster(cyng::controller&
			, cyng::logger
			, boost::uuids::uuid
			, toggle);

		void start_listener(cyng::controller&
			, cyng::logger
			, boost::uuids::uuid
			, std::string const& address
			, std::uint16_t port
			, std::string const& document_root
			, std::uint64_t max_upload_size
			, std::string const& nickname
			, std::chrono::seconds timeout);
	};
}