/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/sml/status.h>

namespace node
{
	namespace sml
	{

		status::status()
		: word_(0)
		{
			reset();
		}

		status::status(status const& other)
			: word_(other.word_)
		{}

		status& status::operator=(status const& other)
		{
			if (this != &other)
			{
				word_ = other.word_;
			}
			return *this;
		}

		void status::reset()
		{
			//
			//	reset status word
			//
			word_ = 0u;
			word_ |= STATUS_BIT_ON;
			word_ |= STATUS_BIT_RESET_BY_WATCHDOG;	
			word_ |= STATUS_BIT_EXT_IF_AVAILABLE;
		}

		status::operator std::uint32_t() const
		{
			return word_;
		}

		void status::set_fatal_error(bool b)
		{
			b ? set(STATUS_BIT_FATAL_ERROR) : remove(STATUS_BIT_FATAL_ERROR);
		}

		void status::set_authorized(bool b)
		{
			//	0 if authorized
			b ? remove(STATUS_BIT_AUTHORIZED_IPT) : set(STATUS_BIT_AUTHORIZED_IPT);
		}

		void status::set_ip_address_available(bool b)
		{
			//	0 if IP address is available (DHCP)
			b ? remove(STATUS_BIT_IP_ADDRESS_AVAILABLE) : set(STATUS_BIT_IP_ADDRESS_AVAILABLE);
		}

		void status::set_ethernet_link_available(bool b)
		{
			//	0 if ethernet link / GSM network is available
			b ? remove(STATUS_BIT_ETHERNET_AVAILABLE) : set(STATUS_BIT_ETHERNET_AVAILABLE);
		}

		void status::set_service_if_available(bool b)
		{
			b ? set(STATUS_BIT_SERVICE_IF_AVAILABLE) : remove(STATUS_BIT_SERVICE_IF_AVAILABLE);
		}

		void status::set_ext_if_available(bool b)
		{
			b ? set(STATUS_BIT_EXT_IF_AVAILABLE) : remove(STATUS_BIT_EXT_IF_AVAILABLE);
		}

		void status::set_mbus_if_available(bool b)
		{
			b ? set(STATUS_BIT_MBUS_IF_AVAILABLE) : remove(STATUS_BIT_MBUS_IF_AVAILABLE);
		}

		bool status::is_set(status_bits e) const
		{
			return (word_ & e) == e;
		}

		void status::set(status_bits e)
		{
			word_ |= e;
		}

		void status::remove(status_bits e)
		{
			word_ &= ~e;
		}

	}	//	sml
}

