/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MBUS_VIF_H
#define SMF_MBUS_VIF_H

#include <cstdint>

namespace smf
{
	namespace mbus	
	{
		/**
		 * value information field
		 */
		class vif
		{
		public:
			explicit vif(char);
			explicit vif(std::uint8_t);
			vif() = delete;

			/**
			 * bit [0..6] Unit and multiplier (value)
			 */
			std::uint8_t get_unit() const;

			std::int8_t get_scaler() const;

			/**
			 * bit [7]
			 */
			bool is_extended() const;

			/**
			 *  E111 1001 (Enhanced Identification)
			 *	Data = Identification No. (8 digit BCD)
			 */
			bool has_enhanced_id() const;

			/**
			 *  E111 1011 Extension of VIF-codes (true VIF is given in the first VIFE)
			 */
			bool is_ext_7B() const;

			/**
			 *  E111 1101 Extension of VIF-codes (true VIF is given in the first VIFE)
			 *	Multiplicative correction factor for value (not unit)
			 */
			bool is_ext_7D() const;

			/**
			 * E111 1111 (Manufacturer Specific)
			 */
			bool is_man_specific() const;

		private:
			std::uint8_t value_;
		};

		/**
		 * value information field
		 */
		//struct vif
		//{
		//	std::uint8_t val_ : 7;	//	[0..6] Unit and multiplier (value)
		//	std::uint8_t ext_ : 1;	//	[7] extension bit - Specifies if a VIFE Byte follows
		//};

		/**
		 * map union over dif bit field
		 */
		//union uvif
		//{
		//	std::uint8_t raw_;
		//	vif vif_;
		//};

		/**
		 * Up to 10 bytes
		 */
		//struct vife
		//{
		//	std::uint8_t val_ : 7;	//	[0..6] Contains Information on the single Value, such as Unit, Multiplicator, etc.
		//	std::uint8_t ext_ : 1;	//	[7] extension bit - Specifies if a VIFE Byte follows
		//};

		/**
		 *  Definition of the G format (coding the date)
		 */
		//struct format_G
		//{
		//	std::uint16_t day_ : 5;	//	1..31
		//	std::uint16_t y_ : 3;	//	
		//	std::uint16_t month_ : 4;	//	0..12
		//	std::uint16_t year_ : 4;	//	0..99

		//};

		//union uformat_G
		//{
		//	std::uint8_t raw_[2];
		//	format_G g_;
		//};

		/**
		 *  Definition of the F format (coding the date and time)
		 */
		//struct format_F
		//{
		//	std::uint16_t minute_ : 6;	//	0..59
		//	std::uint16_t reserved_2 : 2;
		//	std::uint16_t hour_ : 5;	//	0..23
		//	std::uint16_t reserved_1 : 3;
		//	format_G g_;
		//};

		//union uformat_F
		//{
		//	std::uint8_t raw_[4];
		//	format_F f_;
		//};

	}
}


#endif
