/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "transformer.h"

#include <smf/sml/status.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/intrinsics/obis_factory.hpp>
#include <smf/sml/parser/obis_parser.h>

#include <cyng/factory.h>
#include <cyng/numeric_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/io/serializer.h>
#include <cyng/dom/reader.h>

namespace node
{
	cyng::param_map_t transform_status_word(cyng::object obj)
	{
		sml::status_word_t word{ 0 };
		sml::status wrapper(word);
		wrapper.reset(cyng::numeric_cast<std::uint32_t>(obj, 0u));

		return cyng::param_map_factory("word", sml::to_param_map(word));
	}


	cyng::param_map_t transform_broker_params(cyng::logging::log_ptr logger, cyng::param_map_t const& pm)
	{
		BOOST_ASSERT_MSG(pm.size() % 2 == 0, "invalid broker data");

		//
		//	collect all broker
		//
		cyng::vector_t brokers;

		for (std::uint8_t idx_pm = 1; idx_pm < pm.size() + 1; ++idx_pm) {

			auto pos_broker = pm.find(sml::make_obis(sml::OBIS_ROOT_BROKER, idx_pm).to_str());
			if (pos_broker != pm.end()) {

				auto const targets = cyng::to_param_map(pos_broker->second);
				//BOOST_ASSERT_MSG((targets.size() - 1) % 4 == 0, "invalid broker target data");

				auto pos_port = targets.find(sml::OBIS_SERIAL_NAME.to_str());
				if (pos_port != targets.end()) {
					auto pos_login = targets.find(sml::OBIS_BROKER_LOGIN.to_str());
					if (pos_login != targets.end()) {

						//
						//	collect all addresses of this broker
						//
						cyng::vector_t addresses;

						for (std::uint8_t idxi = 1; idxi < (targets.size() / 4) + 1; ++idxi) {
							auto pos_host = targets.find(sml::make_obis(sml::OBIS_BROKER_SERVER, idxi).to_str());
							if (pos_host != targets.end()) {
								auto pos_service = targets.find(sml::make_obis(sml::OBIS_BROKER_SERVICE, idxi).to_str());
								if (pos_service != targets.end()) {
									auto pos_user = targets.find(sml::make_obis(sml::OBIS_BROKER_USER, idxi).to_str());
									if (pos_user != targets.end()) {
										auto pos_pwd = targets.find(sml::make_obis(sml::OBIS_BROKER_PWD, idxi).to_str());
										if (pos_pwd != targets.end()) {

											//
											//	addresses complete - build broker tuple
											//
											auto const tpl = cyng::tuple_factory(
												cyng::param_t("host", pos_host->second),
												cyng::param_t("service", pos_service->second),
												cyng::param_t("user", pos_user->second),
												cyng::param_t("pwd", pos_pwd->second)
											);

											addresses.push_back(cyng::make_object(tpl));

										}
									}
								}
							}
						}

						//
						//	addresses complete - build broker tuple
						//
						auto const tpl = cyng::tuple_factory(
							cyng::param_t("hardwarePort", pos_port->second),
							cyng::param_t("login", pos_login->second),
							cyng::param_factory("addresses", addresses)
						);

						brokers.push_back(cyng::make_object(tpl));

					}
				}
			}
			else {
				CYNG_LOG_WARNING(logger, "missing login data: "
					<< cyng::io::to_type(pm));
			}
		}	//	idx loop

		//CYNG_LOG_DEBUG(logger, "broker: "
		//	<< cyng::io::to_type(brokers));

		return cyng::param_map_factory("brokers", brokers);
	}

	cyng::param_map_t transform_broker_hw_params(cyng::logging::log_ptr logger, cyng::param_map_t const& pm)
	{
		//	%(
		//		("910000000001":%(
		//			("9100000001FF":"COM8"),("910000000201":8u8),("910000000301":"none"),("910000000401":"none"),("910000000501":"one"),("910000000601":e100u16))),
		//		("910000000002":%(
		//			("910000000102":"COM6"),("910000000202":8u8),("910000000302":"even"),("910000000402":"none"),("910000000502":"one"),("910000000602":0960u16)))
		//	)

		//BOOST_ASSERT_MSG(pm.size() % 6 == 0, "invalid broker hardware data");
		//CYNG_LOG_WARNING(logger, "CONVERT: "
		//	<< cyng::io::to_type(pm));

		cyng::param_map_t result;
		for (auto const& param : pm) {
			//CYNG_LOG_DEBUG(logger, param.first << " = " << cyng::io::to_type(param.second));
			auto const r = sml::parse_obis(param.first);
			if (r.second) {

				//	
				//	get index
				//
				auto const nr = r.first.get_storage();


#ifdef _DEBUG
				//
				//	loop over values
				//
				auto const serial = cyng::to_param_map(param.second);
				BOOST_ASSERT_MSG(serial.size() == 6, "invalid broker hardware data");

				for (auto const& value : serial) {
					CYNG_LOG_DEBUG(logger, value.first << " = " << cyng::io::to_type(value.second));

					 //	9100000001FF = "COM8"
					 //	910000000201 = 8u8
					 //	910000000301 = "none"
					 //	910000000401 = "none"
					 //	910000000501 = "one"
					 //	910000000601 = e100u16

				}
#endif
				auto const dom = cyng::make_reader(param.second);

				auto const tpl = cyng::tuple_factory(
					cyng::param_t("databits", dom.get(sml::make_obis(sml::OBIS_SERIAL_DATABITS, nr).to_str())),
					cyng::param_t("parity", dom.get(sml::make_obis(sml::OBIS_SERIAL_PARITY, nr).to_str())),
					cyng::param_t("flowcontrol", dom.get(sml::make_obis(sml::OBIS_SERIAL_FLOW_CONTROL, nr).to_str())),
					cyng::param_t("stopbits", dom.get(sml::make_obis(sml::OBIS_SERIAL_STOPBITS, nr).to_str())),
					cyng::param_t("baudrate", dom.get(sml::make_obis(sml::OBIS_SERIAL_SPEED, nr).to_str())),
					cyng::param_factory("index", nr));

				auto const name = cyng::value_cast(dom.get(sml::OBIS_SERIAL_NAME.to_str()), param.first);
				result.emplace(name, cyng::make_object(tpl));

			}
			else {
				CYNG_LOG_WARNING(logger, "invalid OBIS code: " << param.first << ": " << cyng::io::to_type(param.second));
			}
		}
		return result;

	}

	cyng::param_map_t transform_data_collector_params(cyng::logging::log_ptr logger, cyng::param_map_t const& collectors)
	{
		//	8181C78620[NN]
		//	->	8181C78621FF
		//	->	8181C78622FF
		//	->	8181C78781FF
		//	->	8181C78A23FF
		//		-> list of registers (OBIS code 8181C78A23[NN])
		//	->	8181C78A83FF

		cyng::param_map_t result;

		for (auto const& collector : collectors) {
			auto const pm = cyng::to_param_map(collector.second);
			cyng::param_map_t params;
			for (auto const& entry : pm) {
				if (boost::algorithm::equals(entry.first, sml::OBIS_DATA_COLLECTOR_REGISTER.to_str())) {
					auto const regs = cyng::to_param_map(entry.second);
					cyng::vector_t vec;
					for (auto const& reg : regs) {
						vec.push_back(reg.second);
					}
					params[entry.first] = cyng::make_object(vec);
				}
				else {
					params[entry.first] = entry.second;
				}
			}
			result[collector.first] = cyng::make_object(params);
		}
		return result;
	}

}


