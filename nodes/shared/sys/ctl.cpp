/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include <smf/shared/ctl.h>
#include <NODE_project_info.h>
#include <cyng/async/mux.h>
#include <cyng/async/signal_handler.h>
#include <cyng/json.h>
#include <cyng/dom/reader.h>
#include <cyng/io/serializer.h>
#include <cyng/io/io_chrono.hpp>
#include <cyng/factory/set_factory.h>
#include <cyng/set_cast.h>

#if BOOST_OS_LINUX
#include <smf/shared/write_pid.h>
#endif

#include <cyng/compatibility/file_system.hpp>

#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/nil_generator.hpp>

namespace node
{

	ctl::ctl(unsigned int index
		, unsigned int pool_size
		, std::string const& json_path
		, std::string node_name)
	: config_index_(index)
		, pool_size_(pool_size)
		, json_path_(json_path)
		, node_name_(node_name)
		, uidgen_()
	{}

	int ctl::run(bool console)
	{
		//
		//	to calculate uptime
		//
		std::chrono::system_clock::time_point const tp_start = std::chrono::system_clock::now();

		//
		//	controller loop
		//
		try
		{
			//
			//	controller loop
			//
			bool shutdown{ false };
			while (!shutdown)
			{
				//
				//	establish I/O context
				//
				cyng::async::mux mux{ this->pool_size_ };

				//
				//	read configuration file
				//	start application
				//
				auto const r = read_config_section(json_path_, config_index_);
				if (r.second)
				{
#if BOOST_OS_LINUX
					auto logger = cyng::logging::make_sys_logger(node_name_.c_str(), true);
#else
					const cyng::filesystem::path tmp = cyng::filesystem::temp_directory_path();
					auto dom = cyng::make_reader(r.first);
					const cyng::filesystem::path log_dir = cyng::value_cast(dom.get("log-dir"), tmp.string());

					auto logger = (console)
						? cyng::logging::make_console_logger(mux.get_io_service(), node_name_)
						: cyng::logging::make_file_logger(mux.get_io_service(), (log_dir / get_logfile_name()))
						;
#ifdef _DEBUG
					if (!console) {
						std::cout << "log file see: " << (log_dir / get_logfile_name()) << std::endl;
					}
#endif
#endif

					CYNG_LOG_TRACE(logger, cyng::io::to_str(r.first));
					CYNG_LOG_INFO(logger, "pool size: " << this->pool_size_);

					shutdown = pre_start(mux, logger, r.first);

					//
					//	stop all tasks
					//
					CYNG_LOG_INFO(logger, "stop all tasks");
					mux.stop(std::chrono::milliseconds(100), 10);

					//
					//	print uptime
					//
					const auto duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - tp_start);
					CYNG_LOG_INFO(logger, "uptime " << cyng::to_str(duration));

				}
				else
				{
					std::cout
						<< "use option -D to generate a configuration file"
						<< std::endl;

					//
					//	no configuration found
					//
					shutdown = true;
				}

				//
				//	shutdown scheduler
				//
				mux.shutdown();
			}

			return EXIT_SUCCESS;
		}
		catch (std::exception& ex)
		{
			std::cerr
				<< ex.what()
				<< std::endl;
		}
		return EXIT_FAILURE;
	}

	bool ctl::start(cyng::async::mux&, cyng::logging::log_ptr, cyng::reader<cyng::object> const& cfg, boost::uuids::uuid tag)
	{
#if BOOST_OS_WINDOWS
		static std::string msg = "***Error: The [" + boost::uuids::to_string(tag) + "] service is not implemented";
		::OutputDebugString(msg.c_str());
#endif
		return false;
	}

	bool ctl::pre_start(cyng::async::mux& mux, cyng::logging::log_ptr logger, cyng::object const& cfg)
	{
		//
		//	make configuration reader and extract some basic data
		//
		auto const dom = cyng::make_reader(cfg);
		auto const tag = cyng::value_cast(dom.get("tag"), uidgen_());

		//
		//	apply severity threshold
		//
		logger->set_severity(cyng::logging::to_severity(cyng::value_cast<std::string>(dom.get("log-level"), "INFO")));

#if BOOST_OS_LINUX
		cyng::filesystem::path const log_dir{ cyng::value_cast<std::string>(dom.get("log-dir"), ".") };
		write_pid(log_dir, tag);
#endif

		return start(mux, logger, dom, tag);
	}

	int ctl::create_config() const
	{
		std::fstream fout(json_path_, std::ios::trunc | std::ios::out);
		if (fout.is_open())
		{
			std::cout
				<< "write to file "
				<< json_path_
				<< std::endl;

			//
			//	get default values
			//
			auto obj = cyng::make_object(create_config(fout, cyng::filesystem::temp_directory_path(), cyng::filesystem::current_path()));
			cyng::json::write(std::cout, obj);
			std::cout << std::endl;
			cyng::json::write(fout, obj);

			return EXIT_SUCCESS;
		}
		else
		{
			std::cerr
				<< "unable to open file "
				<< json_path_
				<< std::endl;
		}
		return EXIT_FAILURE;
	}

	cyng::vector_t ctl::create_config(std::fstream&, cyng::filesystem::path&& tmp, cyng::filesystem::path&& cwd) const
	{
		return cyng::vector_factory({
			cyng::tuple_factory(cyng::param_factory("log-dir", tmp.string())
				, cyng::param_factory("log-level",
#ifdef _DEBUG
					"TRACE"
#else
					"INFO"
#endif
				)
			, cyng::param_factory("tag", uidgen_())
			, cyng::param_factory("generated", std::chrono::system_clock::now())
			, cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR)))
		});
	}

	int ctl::print_config(std::ostream& os) const
	{
		//
		//	read configuration file
		//
		auto const r = read_config_section(json_path_, config_index_);
		if (r.second)
		{
			os
				<< "configuration index is [ "
				<< config_index_
				<< " ]"
				<< std::endl;
			cyng::json::pretty_print(os, r.first);
			return EXIT_SUCCESS;
		}
		else
		{
			std::cout
				<< "configuration file ["
				<< json_path_
				<< "] not found or configuration index ["
				<< config_index_
				<< "] out of range"
				<< std::endl;
		}
		return EXIT_FAILURE;
	}

	std::string ctl::get_logfile_name() const
	{
		std::string logfile_name;
		std::transform(node_name_.begin(), node_name_.end(), std::back_inserter(logfile_name),
			[](unsigned char c) -> std::size_t { 

				switch (c) {
				case ':':
				case '\\':
				case '/': 
				case '*': 
				case '"':
				case '<':
				case '>':
				case '|':
					return '-';
				default:
					break;
				}
				return c; 
		});
		return logfile_name + ".log";
	}

	int ctl::init_config_db(std::string section)
	{
		//
		//	read configuration file
		//
		auto const r = read_config_section(json_path_, config_index_);
		if (r.second)
		{
			auto const dom = cyng::make_reader(r.first);
			auto const tpl = cyng::to_tuple(dom.get(section));
			auto const mac = dom["hardware"].get("mac");
			return prepare_config_db(cyng::to_param_map(tpl), mac);
		}
		else 
		{
			std::cout
				<< "configuration file ["
				<< json_path_
				<< "] not found or configuration index ["
				<< config_index_
				<< "] out of range"
				<< std::endl;
		}
		return EXIT_FAILURE;
	}

	int ctl::prepare_config_db(cyng::param_map_t&&, cyng::object)
	{
		//	not implemented
		return EXIT_FAILURE;
	}

#if BOOST_OS_WINDOWS

	void ctl::control_handler(DWORD sig)
	{
		//	forward signal to shutdown manager
		cyng::forward_signal(sig);
	}

#endif

	boost::uuids::uuid ctl::get_random_tag() const
	{
		try {
			return uidgen_();
		}
		catch (std::exception const& ex) {
			boost::ignore_unused(ex);
		}

		//
		//	use other mechanism
		//
		boost::uuids::uuid u;
		int pos = 0;
		auto const start = std::chrono::system_clock::now();
		for (boost::uuids::uuid::iterator it = u.begin(), end = u.end(); it != end; ++it, ++pos) {

			unsigned long random_value{ 0 };
			std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - start;
			if (pos == sizeof(unsigned long)) {
				random_value = elapsed_seconds.count() * 4294967296.0;
				pos = 0;
			}

			// static_cast gets rid of warnings of converting unsigned long to boost::uint8_t
			*it = static_cast<boost::uuids::uuid::value_type>((random_value >> (pos * 8)) & 0xFF);
		}

		boost::uuids::detail::set_uuid_random_vv(u);
		return u;
	}

	bool wait(cyng::logging::log_ptr logger)
	{
		//
		//	wait for system signals
		//
		bool shutdown = false;
		cyng::signal_mgr signal;
		switch (signal.wait())
		{
#if BOOST_OS_WINDOWS
		case CTRL_BREAK_EVENT:
#else
		case SIGHUP:
#endif
			CYNG_LOG_INFO(logger, "SIGHUP received");
			break;
		default:
			CYNG_LOG_WARNING(logger, "SIGINT received");
			shutdown = true;
			break;
		}
		return shutdown;
	}

	std::pair<cyng::object, bool> get_config_section(cyng::object config, unsigned int config_index)
	{
		cyng::vector_t vec;
		vec = cyng::value_cast(config, vec);
		if (vec.size() > config_index) {

			return std::make_pair(vec.at(config_index), true);
		}
		return std::make_pair(cyng::make_object(), false);
	}

	std::pair<cyng::object, bool> read_config_section(std::string const& json_path, unsigned int config_index)
	{
		//
		//	read configuration file
		//
		cyng::object const config = cyng::json::read_file(json_path);
		return (config)
			? get_config_section(config, config_index)
			: std::make_pair(cyng::make_object(), false)
			;
	}
}
