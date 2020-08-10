/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include <smf/cluster/session_stub.h>

#include <cyng/vm/domain/log_domain.h>
#include <cyng/vm/domain/asio_domain.h>
#include <cyng/tuple_cast.hpp>
#include <cyng/io/io_bytes.hpp>

#ifdef SMF_IO_DEBUG
#include <cyng/io/hex_dump.hpp>
#endif
#ifdef SMF_IO_LOG
#include <cyng/io/hex_dump.hpp>
#include <boost/uuid/uuid_io.hpp>
#endif

namespace node 
{
	session_stub::session_stub(boost::asio::ip::tcp::socket&& socket
		, cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, bus::shared_type bus
		, boost::uuids::uuid tag
		, std::chrono::seconds timeout)
	: socket_(std::move(socket))
		, buffer_()
		, pending_(false)
		, mux_(mux)
		, logger_(logger)
		, bus_(bus)
		, vm_(mux.get_io_service(), tag)
		, timeout_(timeout)
#ifdef SMF_IO_LOG
		, log_counter_{ 0u }
		, sml_counter_{ 0u }
		, dir_out_()
#endif
		, name_(boost::uuids::to_string(tag))
	{
		//
		//	register logger domain
		//
		cyng::register_logger(logger_, vm_);
		vm_.async_run(cyng::generate_invoke("log.msg.info", "log domain is running"));

		//
		//	register socket operations to session VM
		//
		cyng::register_socket(socket_, vm_);

		vm_.register_function("session.state.pending", 0, [&](cyng::context&) {
			//
			//	set session state => PENDING
			//
			BOOST_ASSERT(!pending_);
			pending_ = true;
			CYNG_LOG_DEBUG(logger_, vm_.tag() << " session.state.pending ON");
		});

		vm_.register_function("session.update.name", 1, [&](cyng::context& ctx) {

			auto const frame = ctx.get_frame();
			auto const tpl = cyng::tuple_cast<
				std::string			//	[0] device name
			>(frame);

			//
			//	set session name
			//
			name_ = std::get<0>(tpl);
			CYNG_LOG_DEBUG(logger_, vm_.tag() << " - " << name_);

#ifdef SMF_IO_LOG
			std::stringstream ss;
			ss
				<< "ipt-"
				<< boost::uuids::to_string(ctx.tag())
				<< "-"
				<< name_
				;
			cyng::filesystem::path const sub_dir(ss.str());
			dir_out_ = cyng::filesystem::temp_directory_path() / sub_dir;
#endif

		});

#ifdef SMF_IO_LOG
		std::stringstream ss;
		ss
			<< "ipt-"
			<< name_
			;
		cyng::filesystem::path const sub_dir(ss.str());
		dir_out_ = cyng::filesystem::temp_directory_path() / sub_dir;
#endif

	}

	session_stub::~session_stub()
	{
		//CYNG_LOG_DEBUG(logger_, "deconstruct connection(" << tag_ << ')');
		BOOST_ASSERT_MSG(vm_.is_halted(), "session not in HALT state");
	}

	cyng::controller& session_stub::vm()
	{
		return vm_;
	}

	cyng::controller const& session_stub::vm() const
	{
		return vm_;
	}

	std::size_t session_stub::hash() const noexcept
	{
		return vm_.hash();
	}

	void session_stub::start()
	{
		do_read();
	}

	void session_stub::close()
	{
		if (socket_.is_open()) {

			//
			//  close socket
			//
			CYNG_LOG_DEBUG(logger_, vm_.tag()
				<< " close socket "
				<< socket_.remote_endpoint());

			boost::system::error_code ec;
			socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
			socket_.close(ec);
		}
	}

	void session_stub::stop(boost::system::error_code ec)
	{
		//
		//	stop all tasks and halt VM
		//
		shutdown();

		//
		//	wait for pending operations
		//
		if (!vm_.wait(15, std::chrono::milliseconds(100)))	{
			CYNG_LOG_FATAL(logger_, "VM " << vm_.tag() << " shutdown failed");
		}

		if (pending_) {
			//
			//	instruct master to close *this* client and send a "client.res.close" response
			//
			bus_->vm_.async_run(client_req_close(vm_.tag(), ec.value()));
		}
		else {
			//
			//	remove from connection map - call destructor
			//	server::remove_client();
			//
			bus_->vm_.async_run(cyng::generate_invoke("server.remove.client", vm_.tag()));
		}
	}

	void session_stub::shutdown()
	{
		if (socket_.is_open()) {
			CYNG_LOG_WARNING(logger_, "socket still open on session " << vm_.tag());
		}

		//
		//	gracefull shutdown
		//	device/party closed connection or network shutdown
		//
		CYNG_LOG_TRACE(logger_, vm_.tag() << " halt VM");
		vm_.halt();
	}

#ifdef SMF_IO_LOG
	void session_stub::update_io_log(cyng::buffer_t const& buffer)
	{
		if (buffer.size() > 12
			&& buffer.at(0) == 0x1b
			&& buffer.at(1) == 0x1b
			&& buffer.at(2) == 0x1b
			&& buffer.at(3) == 0x1b
			&& buffer.at(4) == 0x1b
			&& buffer.at(5) == 0x1b
			&& buffer.at(6) == 0x1b
			&& buffer.at(7) == 0x1b

			&& buffer.at(8) == 0x01
			&& buffer.at(9) == 0x01
			&& buffer.at(10) == 0x01
			&& buffer.at(11) == 0x01) {

			//
			//	start of new SML message
			//
			++sml_counter_;

			//if (buffer.size() > 0x4a) {

			//	//
			//	//	search for MAC starting with 05 00 15 3B
			//	//
			//	auto const mac = cyng::make_buffer({ 0x08, 0x05, 0x00, 0x15, 0x3B });
			//	//cyng::buffer_t const mac({ 0x62, 0x01, 0x62, 0x00, 0x72, 0x63 });
			//	auto pos = std::search(buffer.begin(), buffer.end(), mac.begin(), mac.end());
			//	if (pos != buffer.end()) {
			//		//
			//		//	update dir_out_
			//		//
			//		//++pos;
			//		pos += mac.size();

			//		auto name = cyng::io::to_hex(pos, pos + 7);
			//		boost::algorithm::to_upper(name);

			//		std::stringstream ss;
			//		ss
			//			<< "ipt-"
			//			<< name_
			//			<< "-"
			//			<< name
			//			;
			//		cyng::filesystem::path const sub_dir(ss.str());
			//		dir_out_ = cyng::filesystem::temp_directory_path() / sub_dir;

			//	}
			//	else {
			//		auto const mac = cyng::make_buffer({ 0x05, 0x00, 0xFF, 0xB0, 0x4B, 0x94, 0xF8 });
			//		auto pos = std::search(buffer.begin(), buffer.end(), mac.begin(), mac.end());
			//		if (pos != buffer.end()) {
			//			//
			//			//	update dir_out_
			//			//
			//			//pos += mac.size();

			//			auto name = cyng::io::to_hex(pos, pos + 7);
			//			boost::algorithm::to_upper(name);

			//			std::stringstream ss;
			//			ss
			//				<< "ipt-"
			//				<< name_
			//				<< "-"
			//				<< name
			//				;
			//			cyng::filesystem::path const sub_dir(ss.str());
			//			dir_out_ = cyng::filesystem::temp_directory_path() / sub_dir;
			//		}
			//	}
			//}
		}
	}

#endif

	void session_stub::do_read()
	{
		socket_.async_read_some(boost::asio::buffer(buffer_),
			[this](boost::system::error_code ec, std::size_t bytes_transferred)
		{
			if (!ec && !pending_)
			{
				CYNG_LOG_TRACE(logger_, "session "
					<< vm_.tag()
					<< " received "
					<< cyng::bytes_to_str(bytes_transferred));


				//
				//	buffer contains the unscrambled input
				//
				const auto buf = parse(buffer_.cbegin(), buffer_.cbegin() + bytes_transferred);

#ifdef SMF_IO_DEBUG
				{
					cyng::io::hex_dump hd;
					std::stringstream ss;
					if (buf.size() > 512) {
						hd(ss, buf.cbegin(), buf.cbegin() + 128);
					}
					else {
						hd(ss, buf.cbegin(), buf.cend());
					}
					CYNG_LOG_TRACE(logger_, "session " << vm().tag() << " input dump " << buf.size() << " bytes:\n" << ss.str());
				}
#endif
#ifdef SMF_IO_LOG
				{
					//
					//	update sml_counter_ and dir_out_
					//	if necessary
					//
					update_io_log(buf);

					std::stringstream ss;
					if (!cyng::filesystem::is_directory(dir_out_)) {

						//
						//	create directory if not exist
						//
						boost::system::error_code ec;
						cyng::filesystem::create_directory(dir_out_, ec);
					}

					ss.str("");
					ss
						<< "rx-"
						<< std::setw(4)
						<< std::setfill('0')
						<< std::dec
						<< ++log_counter_
						<< ".log"
						;
					cyng::filesystem::path const file_name(ss.str());

					{
						std::string const path = (dir_out_ / file_name).string();
						std::ofstream of(path, std::ios::out | std::ios::app);
						if (of.is_open())
						{
							cyng::io::hex_dump hd;
							hd(of, buf.begin(), buf.end());

							CYNG_LOG_TRACE(logger_, "write debug log " << file_name);
							of.close();
						}
					}

					ss.str("");
					ss
						<< std::setw(4)
						<< std::setfill('0')
						<< std::dec
						<< sml_counter_
						<< ".sml"
						//<< ".sml.bin"
						;
					{
						std::string const file_name = (dir_out_ / ss.str()).string();
						std::ofstream of(file_name, std::ios::out | std::ios::app | std::ios::binary);
						if (of.is_open())
						{
							//
							//	Write binary data and remove escaped characters.
							//	This simplifies analysing with an SML parser.
							//
							bool esc{ false };
							for (auto const c : buf) {
								if (c == 0x1b) {
									if (esc) {
										esc = false;
										of.put(c);
									}
									else {
										esc = true;
									}
								}
								else {
									of.put(c);
								}
							}
							//of.write(buf.data(), buf.size());

							CYNG_LOG_TRACE(logger_, "write SML debug log " << file_name);
							of.close();
						}
					}
				}
#endif

				//
				//	continue reading
				//
				do_read();
			}


			else if (ec != boost::asio::error::operation_aborted)
			{
				//
				//	device/party closed connection or network shutdown
				//
				CYNG_LOG_WARNING(logger_, "session "
					<< vm_.tag()
					<< " closed: "
					<< ec
					<< ':'
					<< ec.value()
					<< " ["
					<< ec.message()
					<< ']');

				//
				//	master has to acknowledge that session is closed
				//
				pending_ = true;	
				socket_.close(ec);
				stop(ec);
			}
			else
			{
				//
				//	The session was closed intentionally.
				//	At this point nothing more is to do. Service is going down and all session have to be stopped fast.
				//
				CYNG_LOG_WARNING(logger_, "session "
					<< vm_.tag()
					<< " closed intentionally: "
					<< ec
					<< ':'
					<< ec.value()
					<< " ["
					<< ec.message()
					<< ']');

				stop(ec);

			}
		});
	};
}

