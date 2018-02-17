/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#include "write_db.h"
#include <cyng/async/task/task_builder.hpp>
#include <cyng/dom/reader.h>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/vm/generator.h>

#include <boost/uuid/random_generator.hpp>

namespace node
{
	write_db::write_db(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid tag
		, std::uint32_t channel
		, std::uint32_t source
		, std::string target
		, cyng::db::session db)
	: base_(*btp)
		, logger_(logger)
		, vm_(base_.mux_.get_io_service(), tag)
		, parser_([this](cyng::vector_t&& prg) {

			CYNG_LOG_DEBUG(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> process "
				<< prg.size()
				<< " instructions");

			//
			//	execute programm
			//
			vm_.run(std::move(prg));

		}, true)
		, db_(db)
		, exporter_(vm_, channel, source, target)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> established");

		vm_.run(cyng::register_function("sml.msg", 1, std::bind(&write_db::sml_msg, this, std::placeholders::_1)));
		vm_.run(cyng::register_function("sml.eom", 1, std::bind(&write_db::sml_eom, this, std::placeholders::_1)));

	}

	void write_db::run()
	{
		CYNG_LOG_INFO(logger_, "write_db is running");
	}

	void write_db::stop()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " stopped");
	}

	cyng::continuation write_db::process(cyng::buffer_t const& data)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> processing "
			<< data.size()
			<< " bytes");

		parser_.read(data.begin(), data.end());
		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation write_db::process()
	{
		return cyng::continuation::TASK_STOP;
	}

	void write_db::sml_msg(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		//CYNG_LOG_DEBUG(logger_, "sml.msg " << cyng::io::to_str(frame));

		//
		//	print sml message number
		//
		const std::size_t idx = cyng::value_cast<std::size_t>(frame.at(1), 0);
		//std::cout << "#" << idx << std::endl;
		CYNG_LOG_DEBUG(logger_, "sml.msg #" << idx);

		//
		//	get message body
		//
		cyng::tuple_t msg;
		msg = cyng::value_cast(frame.at(0), msg);

		exporter_.read(msg, idx);
	}


	void write_db::sml_eom(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, "sml.eom " << cyng::io::to_str(frame));

		//const auto p = root_dir_ / exporter_.get_filename();

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> sml.eom ");

		//boost::filesystem::remove(p);
		//exporter_.write(p);
		exporter_.reset();

		ctx.attach(cyng::generate_invoke("stop.writer", base_.get_id()));
	}

}
