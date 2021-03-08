/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/ipt/scramble_key.h>
#include <iomanip>
#include <sstream>
#include <random>
#include <algorithm>
#include <boost/io/ios_state.hpp>

namespace smf {
	namespace ipt {
		scramble_key::scramble_key()
			: key_(null_scramble_key_)
		{}

		scramble_key::scramble_key(key_type const& key)
			: key_(key)
		{}

		void scramble_key::swap(scramble_key& other)	
		{
			key_.swap(other.key_);
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
				return key_;
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
}	

