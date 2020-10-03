/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "console.h"
#include <cyng/parser/bom_parser.h>
#include <cyng/io/io_bytes.hpp>
#include <cyng/io/serializer.h>
#include <cyng/rnd.h>
#include <cyng/numeric_cast.hpp>

#include <boost/predef.h>	//	requires Boost 1.55
#if BOOST_OS_LINUX
#include <readline/readline.h>
#include <readline/history.h>
//#include <unistd.h>
//#include <term.h>
#endif
#include <sstream>
#include <iomanip>
#include <fstream>
#include <numeric>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace node
{
	console::console(cyng::io_service_t& ios, boost::uuids::uuid tag, std::ostream& out, std::istream& in)
		: out_(out)
		, in_(in)
		, exit_(false)
		, history_()
		, call_depth_(0)
		, vm_(ios, tag, out, std::cerr)
	{
		vm_.register_function("help", 0, [this](cyng::context& ctx) {
			auto const frame = ctx.get_frame();
			//out_ << cyng::io::to_str(frame);
			if (frame.empty()) {
				out_
					<< "help\t\tprint this page" << std::endl
					<< "q[uit]\t\tquit this program" << std::endl
					<< "history\t\tlist command history" << std::endl
					<< "! N\t\trepeat command #N" << std::endl
					<< "nl\t\tprint new line" << std::endl
					<< "echo ...\tprint all parameters to console (commas are treated as separators)" << std::endl
					<< "run ...\t\trun a script" << std::endl
					<< "ip\t\tlist network configuration" << std::endl
					<< "ls\t\tlist file system" << std::endl
					<< "pwd [N]\t\tgenerate a password of length N" << std::endl
					<< "uuid\t\tgenerate a UUIDv4" << std::endl
					<< "convert\t\toffers some file conversions" << std::endl
					<< "tracking\tkeep a track of your time" << std::endl
					<< "cleanup\t\tanalyse log files to find inactive devices" << std::endl
					;
			}
			else {
				out_ 
					<< cyng::io::to_str(frame);
			}

		});

		vm_.register_function("!", 1, [this](cyng::context& ctx) {
			auto const frame = ctx.get_frame();
			//out_ << cyng::io::to_str(frame);
			auto str = cyng::value_cast<std::string>(frame.at(0), "0");
			try {
				auto const idx = std::stoul(str);
				if (idx < history_.size()) {
					auto pos = history_.begin();
					std::advance(pos, idx);
					this->parse(*pos);
				}
			}
			catch (std::exception const& ex) {
				out_ << ex.what() << std::endl;
			}


			});

		vm_.register_function("nl", 0, [this](cyng::context& ctx) {

#if BOOST_OS_WINDOWS
			INPUT ip;
			// Set up a generic keyboard event.
			ip.type = INPUT_KEYBOARD;
			ip.ki.wScan = 0; // hardware scan code for key
			ip.ki.time = 0;
			ip.ki.dwExtraInfo = ::GetMessageExtraInfo();

			// Press the <ENTER> key
			ip.ki.wVk = VK_RETURN; 
			ip.ki.dwFlags = 0; // 0 for key press
			::SendInput(1, &ip, sizeof(INPUT));

			// Release the <ENTER> key
			ip.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
			::SendInput(1, &ip, sizeof(INPUT));
#else
			out_ << std::endl;
#endif
			});

		vm_.register_function("cls", 0, [this](cyng::context& ctx) {
#if BOOST_OS_WINDOWS
			HANDLE h_out = ::GetStdHandle(STD_OUTPUT_HANDLE);
			if (h_out == INVALID_HANDLE_VALUE) return;

			CONSOLE_SCREEN_BUFFER_INFO csbi_info;	//!<	save current text colors
			::GetConsoleScreenBufferInfo(h_out, &csbi_info);

			COORD coord_screen{ 0, 0 };    // home for the cursor 
			DWORD chars_written = 0;
			DWORD con_size = csbi_info.dwSize.X * csbi_info.dwSize.Y;

			if (!::FillConsoleOutputCharacter(h_out,        // Handle to console screen buffer 
				0x20,		// UTF-8 SPACE
				//'.',		// Character to write to the buffer
				con_size,			// Number of cells to write 
				coord_screen,		// Coordinates of first cell 
				&chars_written))	// Receive number of characters written
			{
				return;
			}

			if (!::FillConsoleOutputAttribute(h_out,         // Handle to console screen buffer 
				csbi_info.wAttributes,	// Character attributes to use
				con_size,			// Number of cells to set attribute 
				coord_screen,      // Coordinates of first cell 
				&chars_written))	// Receive number of characters written
			{
				return;
			}

			//	reset cursor
			::SetConsoleCursorPosition(h_out, coord_screen);
#else
			//	POSIX version
			std::cerr
				<< "\033[2J"	//	may not work in every terminal
				;
#endif
			});

		vm_.register_function("pwd", 0, [this](cyng::context& ctx) {
			auto const frame = ctx.get_frame();
			static auto pwd = cyng::crypto::make_rnd_pwd();	//	password generator
			if (frame.empty()) {
				out_
					<< pwd(8u)
					;
			}
			else {
				out_
					<< pwd(cyng::numeric_cast<std::uint32_t>(frame.at(0), 8))
					;
			}
		});

		vm_.register_function("uuid", 0, [this](cyng::context& ctx) {
			//auto const frame = ctx.get_frame();
			static boost::uuids::random_generator gen;	//	UUID generator
			out_
				<< gen()
				;
		});

		vm_.register_function("run", 1, [this](cyng::context& ctx) {
			auto const frame = ctx.get_frame();
			auto const file_name = cyng::value_cast<std::string>(frame.at(0), "this.script");
			cmd_run_script(file_name);
		});

	}

	console::~console()
	{}

	int console::loop()
	{
		std::string line;
		std::stringstream	prompt;

		while (!exit_)
		{
			prompt.str("");
			set_prompt(prompt);

#if BOOST_OS_WINDOWS
			out_
				<< prompt.str()
				;
			//	read new command
			std::getline(in_, line);
			boost::trim(line);
			if (exec(line))
			{

				//	save last command
				history_.push_back(line);

			}

#else
			//	read new command
			char* inp = ::readline(prompt.str().c_str());
			line = boost::trim_copy(std::string(inp));
			if (exec(line))
			{

				//	readline history
				::add_history(inp);

				//	save last command
				history_.push_back(line);
			}
			::free(inp);

#endif

		}

		return EXIT_SUCCESS;
	}

	void console::set_prompt(std::iostream& os)
	{
		os
			<< history_.size()
			<< "> "
			;
	}

	bool console::exec(std::string const& line)
	{
		try
		{
			if (line.empty())
			{
				return false;
			}
			else if (boost::algorithm::iequals(line, "q"))	return quit();
			else if (boost::algorithm::iequals(line, "history"))	return history();
			else if (boost::algorithm::iequals(line, "ip"))	return cmd_ip();
			else return parse(line);
		}
		catch (std::exception const& ex)
		{
			std::cerr
				<< "***Warning: "
				<< ex.what()
				<< std::endl
				;

		}
		return false;
	}

	bool console::quit()
	{
		exit_ = true;
		shutdown();
		out_ << "Bye" << std::endl;
		return true;
	}

	void console::shutdown()
	{}

	bool console::history() const
	{
		std::size_t counter = 0;
		out_
			<< std::setfill(' ')
			;

		for (const std::string& s : history_)
		{
			out_
				<< std::setw(4)
				<< counter++
				<< ' '
				<< s
				<< std::endl
				;
		}
		return true;
	}

	bool console::cmd_run(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
	{
		if (pos != end)
		{
			const std::string file_name = cyng::io::to_str(*pos);
			const cyng::filesystem::path p = verify_extension(file_name, ".script");
			if (cyng::filesystem::exists(p))
			{
				return cmd_run_script(p);
			}
			else
			{
				std::cerr
					<< "***Warning: "
					<< "script file ["
					<< cyng::filesystem::absolute(p)
					<< "] not found"
					<< std::endl
					;
			}
		}
		else
		{
			std::cerr
				<< "***Warning: missing script file name"
				<< std::endl
				;
		}
		return false;
	}

	bool console::cmd_run_script(cyng::filesystem::path const& p)
	{

		std::ifstream infile(p.string(), std::ios::in);
		if (!infile.is_open())
		{
			std::cerr
				<< "***Warning: "
				<< "script file ["
				<< p
				<< "] not found"
				<< std::endl
				;
			return false;
		}

		//
		//	update call depth
		//
		++call_depth_;

		std::cerr
			<< "***Info: "
			<< "execute script ["
			<< p
			<< "] - "
			<< call_depth_
			<< std::endl
			;

		std::string line;

		//
		//	read/skip optional BOM
		//
		cyng::bom_parser bomp;
		auto code = bomp.parse(infile);
		switch (code)
		{
		case cyng::bom::UTF16BE:
			//	FE FF big endian
			std::cerr << "***Warning: cannot read UTF16 big endian encoding" << std::endl;
			return false;
		case cyng::bom::UTF16LE:
			//	FF FE
			std::cerr << "***Warning: cannot read UTF16 little endian encoding" << std::endl;
			return false;
		case cyng::bom::UTF32BE:
			//	00 00 FE FF
			std::cerr << "***Warning: cannot read UTF32 big endian encoding" << std::endl;
			return false;
		case cyng::bom::UTF32LE:
			//	FF FE 00 00
			std::cerr << "***Warning: cannot read UTF32 little endian encoding" << std::endl;
			return false;
		case cyng::bom::UTF7a:
		case cyng::bom::UTF7b:
		case cyng::bom::UTF7c:
		case cyng::bom::UTF7d:
			//	2B 2F 76 38, 2B 2F 76 39, 2B 2F 76 2B, 2B 2F 76 2F
			std::cerr << "***Warning: cannot read UTF7 encoding" << std::endl;
			return false;
		case cyng::bom::UTF1:
			//	F7 64 4C
			std::cerr << "***Warning: cannot read UTF1 encoding" << std::endl;
			return false;
		case cyng::bom::UTFEBCDIC:	//	DD 73 66 73
			std::cerr << "***Warning: cannot read EBCDIC encoding" << std::endl;
			return false;
		case cyng::bom::SCSU:		//	0E FE FF
			std::cerr << "***Warning: cannot read SCSU encoding" << std::endl;
			return false;
		case cyng::bom::BOCU1:		//	FB EE 28
			std::cerr << "***Warning: cannot read BOCU1 encoding" << std::endl;
			return false;
		case cyng::bom::GB18030:	//	84 31 95 33
			std::cerr << "***Warning: cannot read GB18030 encoding" << std::endl;
			return false;
		default:
			break;
		}

		//	test for fail() state
		std::size_t line_counter = 0;
		while (std::getline(infile, line, '\n'))
		{
			//	update read state
			line_counter++;	//	line counter

			boost::algorithm::trim(line);
			if (!line.empty() && !boost::algorithm::starts_with(line, ";"))
			{
				if (!exec(line))
				{
					std::cerr
						<< "***Error in "
						<< p
						<< ':'
						<< line_counter
						<< " -- "
						<< line
						<< std::endl
						;
				}
			}

		}

		//
		//	update call depth
		//
		--call_depth_;

		std::cerr
			<< "***Info: "
			<< p
			<< " done"
			<< std::endl
			;

		return true;
	}

	//bool console::cmd_echo(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
	//{
	//	for (; pos != end; ++pos)
	//	{
	//		out_
	//			<< cyng::io::to_str(*pos)
	//			<< ' '
	//			;
	//	}
	//	out_ << std::endl;
	//	return true;
	//}

	bool console::cmd_list(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
	{
		if (pos != end)
		{
			const std::string dir = cyng::io::to_str(*pos);
			if (cyng::filesystem::exists(dir))
			{
				return cmd_list(dir);
			}
			return false;
		}
		cmd_list(cyng::filesystem::current_path());
		return true;
	}

	bool console::cmd_list(cyng::filesystem::path const& p)
	{
		if (cyng::filesystem::is_directory(p))
		{
			std::uint64_t total_size{ 0 };
			std::for_each(cyng::filesystem::directory_iterator(p)
				, cyng::filesystem::directory_iterator()
				, [&total_size](cyng::filesystem::path const& p)
				{

					if (cyng::filesystem::is_directory(p))
					{
						//
						//	get folder size
						//
						;
						std::uint64_t const count = std::accumulate(cyng::filesystem::recursive_directory_iterator(p), cyng::filesystem::recursive_directory_iterator(), 0ULL, [](std::size_t sum, cyng::filesystem::path const& wd) {
							return cyng::filesystem::is_directory(wd)
								? sum
								: sum + cyng::filesystem::file_size(wd)
								;
							});

						//
						//	sum up
						//
						total_size += count;

						std::cout
							<< std::setw(12)
							<< std::setfill(' ')
							<< cyng::bytes_to_str(count)
							<< ' '
							<< '<'
							<< boost::algorithm::trim_copy_if(p.filename().string(), boost::is_any_of("\""))
							<< '>'
							<< std::endl
							;
					}
					else
					{
						auto const count(cyng::filesystem::file_size(p));

						std::cout
							<< std::setw(12)
							<< std::setfill(' ')
							<< cyng::bytes_to_str(count)
							<< ' '
							<< boost::algorithm::trim_copy_if(p.filename().string(), boost::is_any_of("\""))
							<< std::endl
							;

						//
						//	sum up
						//
						total_size += count;

					}

				});

			std::cout
				<< std::setw(12)
				<< std::setfill(' ')
				<< cyng::bytes_to_str(total_size)
				<< ' '
				<< '<'
				<< boost::algorithm::trim_copy_if(p.filename().string(), boost::is_any_of("\""))
				<< "> -- total size"
				<< std::endl
				;

			return true;
		}

		std::cerr
			<< "***Warning: "
			<< p
			<< " is not a directory"
			<< std::endl
			;
		return false;
	}

	bool console::cmd_ip() const
	{
		const std::string host = boost::asio::ip::host_name();
		std::cout
			<< "host name: "
			<< host
			<< std::endl
			;

		try
		{
			boost::asio::io_service io_service;
			boost::asio::ip::tcp::resolver resolver(io_service);
			boost::asio::ip::tcp::resolver::query query(host, "");
			boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);
			boost::asio::ip::tcp::resolver::iterator end; // End marker.
			while (iter != end)
			{
				boost::asio::ip::tcp::endpoint ep = *iter++;
				std::cout
					<< (ep.address().is_v4() ? "IPv4: " : "IPv6: ")
					<< ep
					<< std::endl;
			}
			return true;
		}
		catch (std::exception const& ex)
		{
			std::cerr
				<< "***Error: "
				<< ex.what()
				<< std::endl
				;
		}
		return false;
	}

	cyng::filesystem::path verify_extension(cyng::filesystem::path p, std::string const& ext)
	{
		if (!p.has_extension())
		{
			p.replace_extension(ext);
		}
		return p;
	}

}