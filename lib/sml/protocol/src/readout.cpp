/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/sml/protocol/readout.h>
#include <smf/sml/obis_io.h>
#include <smf/sml/srv_id_io.h>

#include <cyng/io/io_buffer.h>
#include <cyng/io/io_chrono.hpp>
#include <cyng/io/serializer.h>
#include <cyng/factory.h>

namespace node
{
	namespace sml
	{
		readout::readout()
			: trx_()
			, server_id_()
			, client_id_()
			, values_()
		{}

		void readout::reset()
		{
			trx_.clear();
			server_id_.clear();
			client_id_.clear();
			values_.clear();
		}

		readout& readout::set_trx(cyng::buffer_t const& buffer)
		{
			trx_ = cyng::io::to_ascii(buffer);
			return *this;
		}

		readout& readout::set_pk(boost::uuids::uuid pk)
		{
			return set_value("pk", cyng::make_object(pk));
		}

		readout& readout::set_value(std::string const& name, cyng::object obj)
		{
			values_.emplace(name, obj);
			return *this;
		}

		readout& readout::set_value(cyng::param_t const& param)
		{
			values_[param.first] = param.second;
			return *this;
		}

		readout& readout::set_value(cyng::param_map_t&& params)
		{
			//	preserve the elements in values_
			//values_.insert(params.begin(), params.end());

			//	overwrite existing values
			for (auto const& value : params) {
				values_[value.first] = value.second;
			}
			return *this;
		}

		readout& readout::set_value(obis code, cyng::object obj)
		{
			values_[code.to_str()] = obj;
			return *this;
		}

		readout& readout::set_value(obis code, std::string name, cyng::object obj)
		{
			BOOST_ASSERT_MSG(!name.empty(), "name for map entry is empty");
			if (values_.find(name) == values_.end()) {

				//
				//	new element
				//
				auto map = cyng::param_map_factory(code.to_str(), obj)();
				values_.emplace(name, map);
			}
			else {

				//
				//	more elements
				//
				auto p = cyng::object_cast<cyng::param_map_t>(values_.at(name));
				if (p != nullptr) {
					const_cast<cyng::param_map_t*>(p)->emplace(code.to_str(), obj);
				}
			}
			return *this;
		}

		cyng::object readout::get_value(std::string const& name) const
		{
			auto pos = values_.find(name);
			return (pos != values_.end())
				? pos->second
				: cyng::make_object()
				;
		}

		cyng::object readout::get_value(obis code) const
		{
			return get_value(code.to_str());
		}

		std::string readout::get_string(obis code) const
		{
			auto obj = get_value(code);
			cyng::buffer_t buffer;
			buffer = cyng::value_cast(obj, buffer);
			return std::string(buffer.begin(), buffer.end());
		}

		std::string readout::read_server_id(cyng::object obj)
		{
			server_id_ = cyng::value_cast(obj, server_id_);
			return from_server_id(server_id_);
		}

		std::string readout::read_client_id(cyng::object obj)
		{
			client_id_ = cyng::value_cast(obj, client_id_);
			return from_server_id(client_id_);
		}

	}	//	sml
}

