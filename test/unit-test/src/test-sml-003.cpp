/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#include "test-sml-003.h"
#include <iostream>
#include <fstream>
#include <boost/test/unit_test.hpp>
#include <smf/sml/protocol/parser.h>
#include <smf/sml/protocol/serializer.h>
#include <smf/sml/protocol/value.hpp>
#include <smf/sml/exporter/xml_sml_exporter.h>

#include <cyng/io/serializer.h>
#include <cyng/factory.h>
#include <cyng/async/scheduler.h>
#include <cyng/vm/controller.h>
#include <cyng/vm/generator.h>
#include <cyng/value_cast.hpp>
#include <cyng/xml.h>
#include <cyng/log.h>
#include <cyng/vm/domain/log_domain.h>

#include <boost/uuid/random_generator.hpp>

namespace node 
{
	static sml::xml_exporter exporter("UTF-8", "SML");

	void sml_msg(cyng::context& ctx)
	{
		//	[{353736313131,0,0,{0401,{01A815709448030102,{2,5a7dcd17},0,{8181C78611FF},{1,06768485},cc020282,{{0100000009FF,ff, ,01A815709448030102,null},{0100010800FF,1e, ,3932730457,null},{0100010801FF,1e, ,0,null},{0100010802FF,1e, ,3932730457,null},{0100020800FF,1e, ,3325,null},{0100020801FF,1e, ,0,null},{0100020802FF,1e, ,3325,null},{0100100700FF,1b, ,113294,null},{8181C78203FF,ff, ,454D48,null},{8181C78205FF,ff, ,7CF3A773F6A6077267FB519084A3853C645C16E8744B6CCAECE7C74A24BE704B7AB437F09895E9CE760CE4C2EBF426DB,null},{010000090B00,7, ,59f1e350,null},{81062B070000,ff, ,-94,null}},null,null}},b68a},91]
		const cyng::vector_t frame = ctx.get_frame();

		//
		//	print sml message number
		//
		const std::size_t idx = cyng::value_cast<std::size_t>(frame.at(1), 0);
		std::cout << "#" << idx << std::endl;

		//
		//	get message body
		//
		cyng::tuple_t msg;
		msg = cyng::value_cast(frame.at(0), msg);

		exporter.read(msg, idx);
		//std::cout << "sml.msg: " << cyng::io::to_str(frame) << std::endl;
	}
	void sml_eom(cyng::context& ctx)
	{
		//	[890e,102]
		const cyng::vector_t frame = ctx.get_frame();
		//CYNG_LOG_INFO(logger_, "client.req.login " << cyng::io::to_str(frame));
		std::cout << "sml.eom: " << cyng::io::to_str(frame) << std::endl;
		//const auto p = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path("unit-test-%%%%-%%%%-%%%%-%%%%.xml");
		const auto p = boost::filesystem::temp_directory_path() / boost::filesystem::path("unit-test.xml");
		boost::filesystem::remove(p);
		exporter.write(p);
		exporter.reset();
	}

	bool test_sml_003()
	{
		cyng::async::scheduler ios;
		cyng::controller ctrl(ios.get_io_service(), boost::uuids::random_generator()());

		auto clog = cyng::logging::make_console_logger(ios.get_io_service(), "test");
		cyng::register_logger(clog, ctrl);


		ctrl.async_run(cyng::register_function("sml.msg", 1, std::bind(&sml_msg, std::placeholders::_1)));
		ctrl.async_run(cyng::register_function("sml.eom", 1, std::bind(&sml_eom, std::placeholders::_1)));

		ctrl.register_function("sml.log", 1, [&](cyng::context& ctx) {
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(clog, "sml.log - " << cyng::value_cast<std::string>(frame.at(0), ""));
		});

		sml::parser p([&](cyng::vector_t&& prg) {
			//cyng::vector_t r = std::move(prg);
			//std::cout << cyng::io::to_str(r) << std::endl;
			ctrl.async_run(std::move(prg));
		}, false, true);

		//	GetProfileListResponse
		//std::ifstream ifile("C:\\projects\\workplace\\node\\Debug\\push-data-3586334585-418932835.bin", std::ios::binary | std::ios::app);
		//	GetProcParameterResponse
		//std::ifstream ifile("C:\\projects\\workplace\\node\\Debug\\sml\\smf-48cd47c-e9d30005.bin", std::ios::binary | std::ios::app);
		//std::ifstream ifile("C:\\projects\\workplace\\node\\Debug\\sml\\smf-4fabc9ed-474ba8c4-LSM_Status.bin", std::ios::binary | std::ios::app);
		//std::ifstream ifile("C:\\projects\\workplace\\node\\Debug\\sml\\smf-ff67ba9f-edb28f26-2018090T121930-SML-water@solostec.bin", std::ios::binary | std::ios::app);
		std::ifstream ifile("C:\\projects\\node\\build\\Debug\\SML-GetListResponse.bin", std::ios::binary | std::ios::app);
		if (ifile.is_open())
		{
			//	dont skip whitepsaces
			ifile >> std::noskipws;
			p.read(std::istream_iterator<std::uint8_t>(ifile), std::istream_iterator<uint8_t>());

		}
		return true;
	}

}
