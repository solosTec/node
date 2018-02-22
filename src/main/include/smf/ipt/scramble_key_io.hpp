/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_SCRAMBLE_KEY_IO_HPP
#define NODE_IPT_SCRAMBLE_KEY_IO_HPP

#include <smf/ipt/scramble_key.h>
#include <ios>
#include <ostream>
#include <istream>
#include <boost/io/ios_state.hpp>
#include <locale>

namespace node
{
	namespace ipt
	{
		template <typename ch, typename char_traits>
		std::basic_istream<ch, char_traits>& operator>>(std::basic_istream<ch, char_traits> &is, scramble_key &sk)
		{
			const typename std::basic_istream<ch, char_traits>::sentry ok(is);
			if (ok)
			{
				scramble_key::key_type data;

				typedef std::ctype<ch> ctype_t;
				ctype_t const& ctype = std::use_facet<ctype_t>(is.getloc());

				ch xdigits[16];
				{
					char szdigits[] = "0123456789ABCDEF";
					ctype.widen(szdigits, szdigits + 16, xdigits);
				}
				ch *const xdigits_end = xdigits + 16;

				ch c;
				for (std::size_t i = 0; i < sk.size() && is; ++i)
				{
					is >> c;
					c = ctype.toupper(c);

					ch* f = std::find(xdigits, xdigits_end, c);
					if (f == xdigits_end) 
					{
						is.setstate(std::ios_base::failbit);
						break;
					}

					unsigned char byte = static_cast<unsigned char>(std::distance(&xdigits[0], f));

					is >> c;
					c = ctype.toupper(c);
					f = std::find(xdigits, xdigits_end, c);
					if (f == xdigits_end) 
					{
						is.setstate(std::ios_base::failbit);
						break;
					}

					byte <<= 4;
					byte |= static_cast<unsigned char>(std::distance(&xdigits[0], f));

					data[i] = byte;
				}

				if(is)	sk = data;
			}

			return is;
		}

		template <typename ch, typename char_traits>
		std::basic_ostream<ch, char_traits>&  operator<<(std::basic_ostream<ch, char_traits>& os, scramble_key const& v)
		{
			return v.dump(os);
		}

	}
}


#endif	//	NODE_IPT_SCRAMBLE_KEY_IO_HPP
