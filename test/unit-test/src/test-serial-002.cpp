/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Sylko Olzscher 
 * 
 */ 
#include "test-serial-002.h"
#include <cyng/async/mux.h>
#include <boost/test/unit_test.hpp>
#include <boost/core/ref.hpp>
#include <boost/bind.hpp>

namespace node 
{
	std::array<char, NODE::PREFERRED_BUFFER_SIZE> buffer;

	void doRead(boost::asio::serial_port& port);
	void do_end(boost::system::error_code const& ec, std::size_t bytes_transferred, boost::asio::serial_port& port) {
		std::cerr << bytes_transferred << " bytes received - " << ec << std::endl;
		if (!ec)	doRead(port);

	}

	void doRead(boost::asio::serial_port& port)
	{
		port.async_read_some(boost::asio::buffer(buffer, NODE::PREFERRED_BUFFER_SIZE),
			boost::bind(do_end,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred,
				boost::ref(port)));
	}

	void send(boost::asio::serial_port& port)
	{
		boost::system::error_code ec;
		//	Remove the secondary address matching symbol of all the meters on BUS.
		//	10 40 FD 3D 16
		//port.write_some(boost::asio::buffer(cyng::make_buffer({ 0x10, 0x40, 0xFD, 0x3D, 0x16 })), ec);
		//std::this_thread::sleep_for(std::chrono::seconds(1));

		//	initialize all meters on the bus line by using FF as broadcast address
		//	10 40 FF 3F 16
		//port.write_some(boost::asio::buffer(cyng::make_buffer({ 0x10, 0x40, 0xFF, 0x3F, 0x16 })), ec);
		//std::this_thread::sleep_for(std::chrono::seconds(1));

		//	10 40 01 41 16
		port.write_some(boost::asio::buffer(cyng::make_buffer({ 0x10, 0x40, 0x01, 0x41, 0x16 })), ec);
		std::this_thread::sleep_for(std::chrono::seconds(1));
		//	e5 as response expected

		//	read out of Energy information from address 01
		port.write_some(boost::asio::buffer(cyng::make_buffer({ 0x10, 0x7B, 0x01, 0x7C, 0x16 })), ec);
		std::this_thread::sleep_for(std::chrono::seconds(4));
	}

	void read(boost::asio::serial_port& port)
	{
		boost::system::error_code ec;

		//	10 40 FF 3F 16                              
		//port.write_some(boost::asio::buffer(cyng::make_buffer({ 0x10, 0x40, 0xFF, 0x3F, 0x16 })), ec);
		//std::this_thread::sleep_for(std::chrono::milliseconds(390));	//	 68 1B     
		//std::this_thread::sleep_for(std::chrono::milliseconds(400));	//	 68 1B 1B 68          
		//std::this_thread::sleep_for(std::chrono::milliseconds(410));	//	 68
		//std::this_thread::sleep_for(std::chrono::milliseconds(450));	//	 68 1B 1B           
		//	e5 as response expected

		auto buf = cyng::make_buffer({ 0x10, 0x40, 0xFF, 0x3F, 0x16 });
		port.async_write_some(boost::asio::buffer(buf, buf.size()), [](boost::system::error_code const& error, std::size_t bytes_transferred) {});
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));

		buf = cyng::make_buffer({ 0x68, 0x0B, 0x0B, 0x68, 0x73, 0xFD, 0x52, 0x16 });
		port.async_write_some(boost::asio::buffer(buf, buf.size()), [](boost::system::error_code const& error, std::size_t bytes_transferred) {});
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));

		buf = cyng::make_buffer({ 0x10, 0x00, 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xF6, 0x16 });
		port.async_write_some(boost::asio::buffer(buf, buf.size()), [](boost::system::error_code const& error, std::size_t bytes_transferred) {});
		//	e5 as response expected
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));



		//	read out of Energy information from address 01
		//port.write_some(boost::asio::buffer(cyng::make_buffer({ 0x10, 0x7B, 0x01, 0x7C, 0x16 })), ec);
		//std::this_thread::sleep_for(std::chrono::seconds(4));

		//port.write_some(boost::asio::buffer(cyng::make_buffer({ 0x68, 0x0B, 0x0B, 0x68, 0x73, 0xFD, 0x52, 0x16, 0x10, 0x00, 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xF6, 0x16 })), ec);
		//std::this_thread::sleep_for(std::chrono::milliseconds(900));

		buf = cyng::make_buffer({ 0x10, 0x7B, 0xFD, 0x78, 0x16 });
		port.async_write_some(boost::asio::buffer(buf, buf.size()), [](boost::system::error_code const& error, std::size_t bytes_transferred) {});
		//auto size = port.read_some(boost::asio::buffer(buffer, NODE::PREFERRED_BUFFER_SIZE));
		std::this_thread::sleep_for(std::chrono::seconds(4));

		port.async_write_some(boost::asio::buffer(buf, buf.size()), [](boost::system::error_code const& error, std::size_t bytes_transferred) {});
		//auto size = port.read_some(boost::asio::buffer(buffer, NODE::PREFERRED_BUFFER_SIZE));
		std::this_thread::sleep_for(std::chrono::seconds(4));

	}


	bool test_serial_002()
	{
		cyng::async::mux mux{ 2 };
		boost::asio::serial_port port(mux.get_io_service());
		boost::system::error_code ec;
		port.open("COM6", ec);
		if (!ec) {
			//	8N1
			port.set_option(boost::asio::serial_port_base::character_size(8));
			port.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::even));
			//port.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));	// default none
			port.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));	// default one
			//port.set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));


			//port.async_read_some(boost::asio::buffer(buffer, NODE::PREFERRED_BUFFER_SIZE),
			//	boost::bind(do_read,
			//		boost::asio::placeholders::error,
			//		boost::asio::placeholders::bytes_transferred,
			//		boost::ref(port)));
			//port.async_read_some(boost::asio::buffer(buffer), [](boost::system::error_code ec, std::size_t bytes_transferred) {
			//	std::cerr << bytes_transferred << " bytes received" << std::endl;
			//	});

			doRead(port);
			//port.set_option(boost::asio::serial_port_base::baud_rate(13));	//	BR_2400
			//send(port);

			//port.set_option(boost::asio::serial_port_base::baud_rate(300));
			//send(port);

			//port.set_option(boost::asio::serial_port_base::baud_rate(600));
			//send(port);

			//port.set_option(boost::asio::serial_port_base::baud_rate(1200));
			//send(port);

			//port.set_option(boost::asio::serial_port_base::baud_rate(1800));
			//send(port);

			//port.set_option(boost::asio::serial_port_base::baud_rate(2400));
			//send(port);

			//port.set_option(boost::asio::serial_port_base::baud_rate(4800));
			//send(port);

			//port.set_option(boost::asio::serial_port_base::baud_rate(9600));
			//send(port);

			//port.set_option(boost::asio::serial_port_base::baud_rate(19200));
			//send(port);

			//port.set_option(boost::asio::serial_port_base::baud_rate(300));
			//read(port);

			port.set_option(boost::asio::serial_port_base::baud_rate(2400));
			read(port);
			return true;
		}
		return false;
	}


}
