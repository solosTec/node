/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_IEC_PARSER_H
#define SMF_IEC_PARSER_H

#include <smf/iec/defs.h>

#include <cyng/obj/intrinsics/edis.h>
#include <cyng/obj/intrinsics/obis.h>

#include <string>
#include <functional>
#include <algorithm>

namespace smf
{
	namespace iec
	{
		/**
		 * /device id<CR><LF>
		 * STX
		 * data
		 * !<CR><LF>
		 * ETX
		 * BBC (all data between STX and ETX)
		 */
		class tokenizer
		{
		public:
			using cb_f = std::function<void(std::string)>;
			using cb_final_f = std::function<void(std::string, bool)>;

		private:

			enum class state {
				START,
				DEVICE_ID,
				DEVICE_ID_COMPLETE,
				DATA,
				DATA_LINE,
				DATA_COMPLETE,
				DATA_COMPLETE_LF,
				STX,
				ETX,
				BBC,
			} state_;

		public:
			tokenizer(cb_f, cb_final_f);

			/**
			 * parse the specified range
			 */
			template < typename I >
			void read(I start, I end)
			{
				static_assert(std::is_same_v<char, typename std::iterator_traits<I>::value_type>, "data type char expected");
				std::for_each(start, end, [this](char c)
					{
						//
						//	parse
						//
						this->put(c);
					});
			}

			/**
			 * read a single byte and update
			 * parser state.
			 * Implements the state machine
			 */
			void put(char);

			std::string get_device_id() const;
			std::uint8_t get_bbc() const;

		private:

			state state_start(char c);
			state state_device_id(char c);
			state state_data(char c);
			state state_bbc(char c);

		private:
			cb_f cb_;
			cb_final_f final_;
			std::string id_, data_;
			std::uint8_t bbc_, crc_;
		};

		/** @brief Implementation of the iec 62056-21 protocol
		 * 
		 */
		class parser
		{
		public:
			using cb_data_f = std::function<void(cyng::obis, std::string value, std::string unit)>;
			using cb_final_f = tokenizer::cb_final_f;

		private:
			//	examples:
			//	F.F(00000000)
			//	0.0.0(03218421)		- device ID
			//	0.9.1(152036)		- time
			//	0.9.2(160608)		- date
			//	0.1.2*01(10-02-01 00:00) 
			//	1.8.0*01(320695.7*kWh) 
			//	1.7.0(0.000*kW)
			//	1.8.0(000000.000*kWh)
			//	2.7.0(0.000*kW)
			//	0.2.2(00000001)
			//	0.2.0(FE63)
			//	3.4.0(05)(00.000*kvar)	- profile data entry 5
			//	3.6.3(186*kvar)(10-02-01 00:15) 
			//	4.6.1(0*kvar)(10-02-01 00:00) 
			//	9.5.0(00.000*kVA)
			//	32.7.0(231.0*V)
			//	31.7.0(0.000*A)
			//	32.32.0(000000)
			//	52.32.0(094050)
			//	1.6.1*87(01.05)(2101082215)
			//	0.1.2.04 (98-05-01 00:00)	- time of last reset
			// 
			//	OBIS(data)	- Measurement value
			//	OBIS(data)(time) - Extreme value
			//	OBIS(minutes)(data) - Average value
			//	OBIS(minutes)(data) - Average value
			//	OBIS(data)(Sum of the overrun times)(Number of the overrun times) - Overconsumption

		public:
			parser(cb_data_f, cb_final_f, std::uint8_t medium = 1);

			/**
			 * parse the specified range
			 */
			template < typename I >
			void read(I start, I end)
			{
				static_assert(std::is_same_v<char, typename std::iterator_traits<I>::value_type>, "data type char expected");
				std::for_each(start, end, [this](char c)
					{
						//
						//	parse
						//
						this->tokenizer_.put(c);
					});
			}

		private:

			void convert(std::string const&, std::string const&);
			void convert(std::string const&, std::string const&, std::string const&);

		private:
			tokenizer tokenizer_;
			cb_data_f data_;
			std::uint8_t const medium_;
		};

		/**
		 * split data line into separate entities
		 */
		std::vector<std::string> split_line(std::string const&);

		/**
		 * convert a string into an edis code.
		 * format is "a.b.c"
		 */
		cyng::obis to_obis(std::string, std::uint8_t medium);

		/**
		 * convert 3 strings into u8 values
		 */
		std::tuple<std::uint8_t, std::uint8_t, std::uint8_t> to_u8(std::string, std::string, std::string);

		/**
		 * convert 4 strings into u8 values
		 */
		std::tuple<std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t> to_u8(std::string, std::string, std::string, std::string);

		/**
		 * Split a string over the '*' symbol
		 */
		std::tuple<std::string, std::string> split_edis_value(std::string);

	}
}

#endif
