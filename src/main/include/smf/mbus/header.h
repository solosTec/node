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

// #pragma pack(push, 1)

namespace node 
{

	/**
	 * define an array that contains the server ID
	 */
	using srv_id_t = std::array<char, 9>;

	/**
	 * Provide a base class for wired and wireless header data.
	 * Both mBus header share some information like server ID.
	 * access number, status and the payload data
	 */
	class header_base
	{
	public:
		header_base();
		header_base(srv_id_t const&
			, std::uint8_t access_no
			, std::uint8_t status
			, cyng::buffer_t&& payload);

		virtual cyng::buffer_t get_srv_id() const = 0;

		/**
		 * @return a copy of the (decoded) data
		 */
		virtual cyng::buffer_t data() const = 0;

		std::uint8_t get_access_no() const;
		std::uint8_t get_status() const;

	protected:
		/**
		 * The server ID contains the identification number (secondary address),
		 * manufacturer ID, generation/version and media type.
		 */
		srv_id_t const server_id_;

		/**
		 * Access Number (From master initiated session uses Gateway Access	Number.
		 * From slave initiated session uses Meter Access Number.)
		 */
		std::uint8_t const access_no_;

		/**
		 * Status (from master to slave) used for gateway status (RSSI)
		 * (from slave to master) used for meter status
		 */
		std::uint8_t const status_;

		/**
		 * raw data / payload
		 */
		cyng::buffer_t const data_;

	};

	/**
	 * Context data required to decrypt wireless mBus data.
	 * Part of the mBus wireless header.
	 */
	class wireless_prefix
	{
	public:
		wireless_prefix();
		wireless_prefix(std::uint8_t access_no
		, std::uint8_t status
		, std::uint8_t aes_mode
		, std::uint8_t block_counter
		, cyng::buffer_t&&);

		std::uint8_t get_access_no() const;
		std::uint8_t get_status() const;
		std::uint8_t get_mode() const;
		std::uint8_t get_block_counter() const;

		//	block counter * 16
		std::uint8_t blocks() const;
		cyng::buffer_t get_data() const;

		std::pair<cyng::buffer_t, bool> decode_mode_5(cyng::buffer_t const& server_id
			, cyng::crypto::aes_128_key const& key) const;

		/**
		 * @return initial vector for AES CBC decoding
		 */
		cyng::crypto::aes::iv_t get_iv(cyng::buffer_t const& server_id) const;

	private:
		std::uint8_t const access_no_;
		std::uint8_t const status_;
		std::uint8_t const aes_mode_;
		std::uint8_t const block_counter_;
		cyng::buffer_t const data_;
	};

	/**
	 * Long header used by wireless M-Bus.
	 * Applied from master with CI = 0x53, 0x55, 0x5B, 0x5F, 0x60, 0x64, 0x6Ch, 0x6D
	 * Applied from slave with CI = 0x68, 0x6F, 0x72, 0x75, 0x7C, 0x7E, 0x9F
	 */
	class header_long_wireless : public header_base
	{
		friend std::pair<header_long_wireless, bool> make_header_long_wireless(cyng::buffer_t const&);

	public:
		header_long_wireless();
		header_long_wireless(srv_id_t const& id
			, std::uint8_t access_no
			, std::uint8_t status
			, std::uint8_t mode
			, std::uint8_t block_counter
			, cyng::buffer_t&&);

		header_long_wireless(srv_id_t const& id
			, wireless_prefix const&);

		header_long_wireless& operator=(header_long_wireless const&) = delete;
		
		/**
		 * @return server ID
		 */
		virtual cyng::buffer_t get_srv_id() const override;

		/**
		 * @return exncryption context
		 */
		wireless_prefix get_prefix() const;

		/**
		 * method of data encryption (AES mode)
		 *
		 * 0 - no enryption
		 * 4 - deprecated
		 * 5 - symmetric enryption (AES128 with CBC)
		 * 7 - advanced symmetric enryption
		 * 13 - assymetric enryption
		 *
		 * @return 0, 5, 7, or 13
		 */
		std::uint8_t get_mode() const;

		/*
		 * A value of 0 means that this message is not encrypted
		 * at all. A value of 0x0F specifies that partial encryption is disabled 
		 * and no unencrypted data follow after the encrypted data
		 * 
		 * @return number of encrypted 16 Byte Blocks for CBC Mode.
		 */
		std::uint8_t get_block_counter() const;

		/**
		 * encoded data (without verification bytes)
		 */
		virtual cyng::buffer_t data() const override;

		/**
		 * encoded data (with verification bytes)
		 */
		cyng::buffer_t const& data_raw() const;

		/**
		 * Test for AES bytes
		 */
		bool verify_encryption() const;

		/**
		 * @return initial vector for AES CBC decoding
		 */
		cyng::crypto::aes::iv_t get_iv() const;


	private:
		/**
		 * Configuration Field / Configuration Field Extension
		 */
		std::uint8_t const mode_;	//	AES mode
		std::uint8_t const block_counter_;
	};

	/**
	 * Long header used by wired M-Bus.
	 */
	class header_long_wired : public header_base
	{
		friend std::pair<header_long_wired, bool> make_header_long_wired(cyng::buffer_t const&);
	public:
		header_long_wired();
		header_long_wired(srv_id_t const& id
			, std::uint8_t access_no
			, std::uint8_t status
			, std::uint16_t signature
			, cyng::buffer_t&&);


		virtual cyng::buffer_t get_srv_id() const override;
		virtual cyng::buffer_t data() const override;

		std::uint16_t get_signature() const;

	private:
		std::uint16_t const signature_;

	};

	/**
	 * read first 9 bytes to get the server ID
	 *
	 * @return true if range was sufficient.
	 */
	cyng::buffer_t::const_iterator read_server_id(srv_id_t&, cyng::buffer_t::const_iterator, cyng::buffer_t::const_iterator);

	/**
	 * The wireless prefix is part of the wireless mBus header
	 *
	 * @return wireless prefix and success flag
	 */
	std::pair<wireless_prefix
		, bool> make_wireless_prefix(cyng::buffer_t::const_iterator, cyng::buffer_t::const_iterator);

	/**
	 * The wireless prefix is part of the wireless mBus header
	 *
	 * @return wireless prefix and success flag
	 */
	std::pair<wireless_prefix
		, bool> make_wireless_prefix(cyng::buffer_t const&);

	/**
	 * @return wireless mBus header and success flag
	 */
	std::pair<header_long_wireless, bool> make_header_long_wireless(cyng::buffer_t const&);

	/**
	 * @return wired mBus header and success flag
	 */
	std::pair<header_long_wired, bool> make_header_long_wired(cyng::buffer_t const&);


	/**
	 * @return true is buffer is longer then 2 bytes and first two bytes are 0x2F
	 */
	bool test_aes(cyng::buffer_t const&);

	/**
	 * Decode data with AES CBC (mode 5)
	 * @return true if encryption was successful (first two bytes are 0x2F)
	 */
	std::pair<cyng::buffer_t, bool> decode(cyng::buffer_t const& data
		, cyng::crypto::aes_128_key const& key
		, cyng::crypto::aes::iv_t const& iv);
	
}	//	node

// #pragma pack(pop)

#endif
