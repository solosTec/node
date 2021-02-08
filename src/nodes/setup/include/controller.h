/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/controller_base.h>


 namespace smf {

	class controller : public config::controller_base
	{
	public:
		controller(config::startup const&);
		virtual int run() override;

	protected:
		cyng::vector_t create_default_config(std::chrono::system_clock::time_point&& now
			, std::filesystem::path&& tmp
			, std::filesystem::path&& cwd) override;
		void print_configuration(std::ostream&) override;

	private:
		cyng::param_t create_cluster_spec();


	};
}