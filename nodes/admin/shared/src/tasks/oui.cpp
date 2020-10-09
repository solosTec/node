/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "oui.h"
#include <smf/sml/srv_id_io.h>

#include <cyng/io/serializer.h>
#include <cyng/table/key.hpp>
#include <cyng/table/body.hpp>
#include <cyng/csv.h>
#include <cyng/parser/buffer_parser.h>

namespace node
{
	oui::oui(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::store::db& db
		, std::string file_name
		, boost::uuids::uuid tag)
	: base_(*btp)
		, logger_(logger)
		, cache_(db)
		, file_name_(file_name)
		, tag_(tag)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");
	}

	cyng::continuation oui::run()
	{	
		cyng::error_code ec;
		if (cyng::filesystem::exists(file_name_, ec)) {
			load();
		}

		return cyng::continuation::TASK_STOP;
	}

	void oui::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "system task is stopped");
	}

	cyng::continuation oui::process()
	{

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

	void oui::load()
	{
		CYNG_LOG_INFO(logger_, "start loading OUI records");
		auto const now = std::chrono::system_clock::now();

		std::size_t counter{ 0 };
		cache_.access([&](cyng::store::table* tbl_oui) {

			//
			//	expected size 
			//
			tbl_oui->reserve(28671u);

			cyng::csv::read_file(file_name_, [&](cyng::tuple_t&& tpl)->void {

				if (tpl.size() == 4) {
					//
					//	skip first entry
					//
					if (counter != 0) {

						//
						//	get OUI
						//
						auto pos = tpl.begin();
						std::advance(pos, 1);
						auto const str = cyng::value_cast<std::string>(*pos, "");
						if (str.size() == 6) {

							auto const r = cyng::parse_hex_string_safe(str);
							if (r.second) {

								//
								//	get organisation
								//
								std::advance(pos, 1);
								auto const org = cyng::value_cast<std::string>(*pos, "");

								tbl_oui->insert(cyng::table::key_generator(r.first)
									, cyng::table::data_generator(org)
									, 1u
									, tag_);
							}
						}
					}
					++counter;

					if ((counter % 1000) == 0) {

						log_progress(counter, now);

					}
				}
				});

			}, cyng::store::write_access("_OUI"));

		//
		//	last log message
		//
		log_progress(counter, now);

	}

	void oui::log_progress(std::size_t counter, std::chrono::system_clock::time_point now)
	{
		auto const duration = std::chrono::system_clock::now() - now;
		auto const seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
		if (seconds != 0) {
			CYNG_LOG_INFO(logger_, counter
				<< " OUI records inserted in "
				<< cyng::to_str(duration)
				<< " ("
				<< (counter / seconds)
				<< "/sec)");
		}
		else {
			CYNG_LOG_INFO(logger_, counter
				<< " OUI records inserted in "
				<< cyng::to_str(duration));
		}

	}


}
