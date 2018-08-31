/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/ipt/scramble_key.h>
#include <iomanip>
#include <sstream>
#include <random>
#include <algorithm>
#include <boost/io/ios_state.hpp>
#include <boost/functional/hash.hpp>

namespace node
{
	namespace ipt
	{
		scramble_key::scramble_key()
			: value_(null_scramble_key_)
		{}

		scramble_key::scramble_key(key_type const& key)
			: value_(key)
		{}

		scramble_key::scramble_key(scramble_key const& other)
			: value_(other.value_)
		{}

		void scramble_key::swap(scramble_key& other)	
		{
			value_.swap(other.value_);
		}

		bool scramble_key::equal(scramble_key const& other) const	
		{
			return std::equal(value_.begin(), value_.end(), other.value_.begin());
		}

		bool scramble_key::less(scramble_key const& other) const	
		{
			return std::lexicographical_compare(key().begin()
				, key().end()
				, other.key().begin()
				, other.key().end());
		}

		scramble_key& scramble_key::operator=(scramble_key const& other)	
		{
			if (this != &other)	{
				value_ = other.value_;
			}
			return *this;
		}


		std::size_t scramble_key::size()	
		{
			return SCRAMBLE_KEY_LENGTH;
		}

		std::ostream& scramble_key::dump(std::ostream& os) const	
		{
			//	store and reset stream state
			boost::io::ios_flags_saver  ifs(os);

			if (os.good())
			{
				os
					<< std::hex
					<< std::setfill('0')
					;
				for (unsigned char ii : key())
				{
					os
						<< std::setw(2)
						<< int(ii)
						;
				}
			}
			return os;
		}

		std::string scramble_key::str() const	
		{
				std::ostringstream os;
				dump(os);
				return os.str();
		}

		scramble_key::key_type const& scramble_key::key() const	
		{
				return value_;
		}

		//	gcc grumps: "missing braces around initializer ..."
		//	Therefor are two brackets around the initializer list

		const scramble_key::key_type scramble_key::default_scramble_key_ =
		{{
			0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
			0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
			0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04,
			0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x00, 0x01,
		}};

		const scramble_key::key_type scramble_key::null_scramble_key_ =
		{{
			0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
			0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
			0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
			0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		}};

		//	global operations
		bool operator==(const scramble_key& x, const scramble_key& y)	
		{
			return x.equal(y);
		}

		bool operator<(const scramble_key& x, const scramble_key& y)	
		{
			return x.less(y);
		}

		bool operator!=(const scramble_key& x, const scramble_key& y)	
		{
			return !(x.equal(y));
		}

		bool operator> (const scramble_key& x, const scramble_key& y)	
		{
			return y.less(x);
		}

		bool operator<= (const scramble_key& x, const scramble_key& y)	
		{
			return !(y.less(x));
		}

		bool operator>=(const scramble_key& x, const scramble_key& y)	
		{
			return !(x.less(y));
		}

		void swap(scramble_key& x, scramble_key& y)	
		{
			x.swap(y);
		}
		
		scramble_key gen_random_sk()
		{
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<int> dis(std::numeric_limits<char>::min(), std::numeric_limits<char>::max());

			scramble_key::key_type key;
			std::generate(key.begin(), key.end(), [&dis, &gen]() {
				return dis(gen);
			});

			return scramble_key(key);
		}

	}	//	ipt
}	//	node

namespace cyng
{
	namespace traits
	{

#if defined(CYNG_LEGACY_MODE_ON)
		const char type_tag<node::ipt::scramble_key>::name[] = "sk";
#endif
	}	// traits	
}

namespace std
{
	size_t hash<node::ipt::scramble_key>::operator()(node::ipt::scramble_key const& sk) const noexcept
	{
		std::size_t seed = 0;
		for (auto c : sk.key()) {
			boost::hash_combine(seed, c);
		}
		return seed;
	}

	bool equal_to<node::ipt::scramble_key>::operator()(node::ipt::scramble_key const& sk1, node::ipt::scramble_key const& sk2) const noexcept
	{
		return sk1.key() == sk2.key();
	}
}