/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#include "mail_config.h"
#include <cyng/dom/reader.h>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>

namespace node 
{
	void init(cyng::object obj, mail_config& mc)
	{
// 		std::cout << std::endl << "mail configuration: " << cyng::io::to_str(obj) << std::endl;
		cyng::select_reader<cyng::object>::type dom(obj);

		mc.host_ = cyng::io::to_str(dom.get("host"));
		mc.port_ = cyng::value_cast<int>(dom.get("port"), 465);
		mc.auth_name_ = cyng::io::to_str(dom["auth"].get("name"));
		mc.auth_pwd_ = cyng::io::to_str(dom["auth"].get("pwd"));
		mc.auth_method_ = cyng::io::to_str(dom["auth"].get("method"));
		mc.sender_.name_ = cyng::io::to_str(dom["sender"].get("name"));
		mc.sender_.address_ = cyng::io::to_str(dom["sender"].get("address"));
// 		dom.get("recipients");
// 		std::cout << cyng::io::to_str(dom.get("recipients")) << std::endl;
		cyng::vector_t recipients;
		recipients = cyng::value_cast(dom.get("recipients"), recipients);
// 		std::cout << recipients.size() << " recipients" << std::endl;
		for (auto r : recipients)
		{
			cyng::select_reader<cyng::object>::type r_dom(r);
			mc.recipients_.emplace_back(cyng::io::to_str(r_dom.get("name")), cyng::io::to_str(r_dom.get("address")));
		}
	}
	
	mail_address::mail_address()
	: name_()
	, address_()
	{}
	
	mail_address::mail_address(std::string const& name, std::string const& address)
	: name_(name)
	, address_(address)
	{}
}

// 		std::string host_;
// 		std::uint16_t port_;
// 		std::string auth_name_;
// 		std::string auth_pwd_;
// 		std::string auth_method_;
// 		mail_address sender_;
// 		std::vector<mail_address> recipients_;

namespace std 
{
	ostream& operator<<(ostream& os, const node::mail_config& mc)
	{
		os 
		<< '['
		<< mc.auth_name_
		<< ':'
		<< mc.auth_pwd_
		<< ']'
		<< '@'
		<< mc.host_
		<< ':'
		<< mc.port_
		<< '?'
		<< '['
		<< mc.sender_.name_
		<< '/'
		<< mc.sender_.address_
		<< ']'
		<< '&'
		<< mc.recipients_.size()
		;
		return os;
	}
}

