/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 
#ifndef NODE_MBUS_HEADER_H
#define NODE_MBUS_HEADER_H


#include <smf/mbus/defs.h>
#include <smf/mbus/units.h>

#include <cyng/intrinsics/buffer.h>
#include <cyng/object.h>
#include <cyng/crypto/aes.h>

#include <array>

#pragma pack(push, 1)

namespace node 
{

	/**
	 * 7.2.4 Configuration Field
	 * Containing 5 bytes defining the encryption mode
	 * This is the 2. byte (MSB)
	 * You can mask the first 5 bits with 0x1F.
	 */
	struct cfg_field_mode
	{
		std::uint16_t mode_ : 5;	//	mode (0, 5, 7, or 13)
		std::uint16_t x1_ : 3;	//	mode specific
	};

	/** @brief 7.2.4.3 Configuration Field for Security Mode 5
	 *
	 * Security Mode 5 is a symmetric encryption method using AES128 with CBC, a special
	 * Initialisation Vector and a persistent key.
	 */
	struct cfg_field_5 
	{
		std::uint16_t hops_ : 1;	//	hop counter
		std::uint16_t r_ : 1;	//	Repeated Access
		std::uint16_t content_ : 2;	//	content of message
		std::uint16_t number_ : 4;	//	number of encrypted blocks

		std::uint16_t mode_ : 5;	//	mode (== 5)
		std::uint16_t s_ : 1;	//	synchronous
		std::uint16_t a_ : 1;	//	accessibility
		std::uint16_t b_ : 1;	//	bidrectional communication (static/dynamic)
	};

	/** @brief 7.2.4.4 Configuration Field for Security Mode 7
	 * Security Mode 7 is a symmetric encryption method using AES128 with CBC and an ephemeral key
	 */
	struct cfg_field_7
	{
		std::uint16_t r2_ : 4;	//	reserved
		std::uint16_t number_ : 4;	//	number of encrypted blocks
		std::uint16_t mode_ : 5;	//	mode (== 7)
		std::uint16_t r1_ : 1;	//	reserved
		std::uint16_t c_ : 2;	//	Content of Message
	};

	/** @brief 7.2.4.5 Configuration Field for Security Mode 13
	 *
	 * Security Mode 13 is an asymmetric encryption method using TLS
	 */
	struct cfg_field_D
	{
		std::uint16_t number_ : 8;	//	number of encrypted blocks
		std::uint16_t mode_ : 5;	//	mode (== 13)
		std::uint16_t r1_ : 1;	//	reserved
		std::uint16_t c_ : 2;	//	Content of Message
	};

	union ucfg_fields
	{
		char		raw_[2];
		cfg_field_5	cfg_field_5_;
		cfg_field_7	cfg_field_7_;
		cfg_field_D	cfg_field_D_;
	};

	/**
	 * Short header used only by wireless M-Bus.
	 * Applied from master with CI = 0x5A, 0x61, 0x65
	 * Applied from slave with CI = 67, 0x6E, 0x74, 0x7A, 0x7D, 0x7F, 0x9Eh
	 */
	class header_short
	{
		friend std::pair<header_short, bool> make_header_short(cyng::buffer_t const&);

	public:
		header_short();
		header_short(header_short const&);
		header_short(header_short&&);
		virtual ~header_short();
		
		header_short& operator=(header_short const&);

		std::uint8_t get_access_no() const;
		std::uint8_t get_status() const;

		/**
		 * method of data encryption
		 *
		 * 0 - no enryption
		 * 5 - symmetric enryption (AES128 with CBC)
		 * 7 - advanced symmetric enryption
		 * 13 - assymetric enryption
		 *
		 * @return 0, 5, 7, or 13
		 */
		std::uint8_t get_mode() const;

		/*
		 * Returns 0 for not-decrypted messages
		 * 
		 * @return count of (encrypted) blocks
		 */
		std::uint8_t get_block_counter() const;

		/**
		 * encoded data
		 */
		cyng::buffer_t data() const;

		/**
		 * Decode data with AES CBC (mode 5)
		 * @return true if encryption was successful (first two bytes are 0x2F)
		 */
		bool decode(cyng::crypto::aes_128_key const&, cyng::crypto::aes::iv_t const& iv);

		/**
		 * Test for AES bytes
		 */
		bool verify_encryption() const;

		/**
		 * Remove all elements with value 0x2F from end of data vector
		 */
		std::size_t remove_aes_trailer();

	private:
		/**
		 * Access Number (From master initiated session uses Gateway Access	Number.
		 * From slave initiated session uses Meter Access Number.)
		 */
		std::uint8_t access_no_;

		/**
		 * Status (from master to slave) used for gateway status (RSSI)
		 * (from slave to master) used for meter status
		 */
		std::uint8_t status_;

		/**
		 * Configuration Field / Configuration Field Extension
		 */
		std::array<char, 2>   cfg_;

		/**
		 * raw data
		 */
		cyng::buffer_t data_;

	};

	/**
	 * Long header used by wired and wireless M-Bus.
	 * Applied from master with CI = 0x53, 0x55, 0x5B, 0x5F, 0x60, 0x64, 0x6Ch, 0x6D
	 * Applied from slave with CI = 0x68, 0x6F, 0x72, 0x75, 0x7C, 0x7E, 0x9F
	 */
	class header_long
	{
		friend std::pair<header_long, bool> make_header_long(char type, cyng::buffer_t const&);

	public:
		header_long();
		header_long(header_long const&);
		header_long(header_long&&);
		virtual ~header_long();

		header_long& operator=(header_long const&);
		
		cyng::buffer_t get_srv_id() const;

		/** 
		 * @return short part of header
		 */
		header_short const& header() const;
		header_short& header();

		/**
		 * @return initial vector for AES CBC decoding
		 */
		cyng::crypto::aes::iv_t get_iv() const;

		/**
		 * Decode data with AES CBC (mode 5)
		 * @return true if encryption was successful (first two bytes are 0x2F)
		 */
		bool decode(cyng::crypto::aes_128_key const&);

	private:
		/**
		 * M-Bus type + meter address
		 */
		std::array<char, 9>	server_id_;

		header_short	hs_;
	};


	std::pair<header_short, bool> make_header_short(cyng::buffer_t const& inp);
	std::pair<header_long, bool> make_header_long(char type, cyng::buffer_t const&);

	/**
	 * @return true is buffer is longer then 2 bytes and first two bytes are 0x2F
	 */
	bool test_aes(cyng::buffer_t const&);

	
}	//	node

#pragma pack(pop)

#endif
