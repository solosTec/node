#include <smf/sml/tokenizer.h>

#ifdef _DEBUG_SML
#include <iostream>
#endif

#include <boost/assert.hpp>

namespace smf {
	namespace sml {

		tokenizer::tokenizer(cb f)
			: cb_(f)
			, state_(state::START)
			, length_(0)
			, type_(sml_type::UNKNOWN)
			, data_()
			, crc_(crc_init())
		{}

		void tokenizer::put(char c) {

			switch (state_) {
			case state::START:
				state_ = start(c);
				break;
			case state::LENGTH:
				state_ = state_length(c);
				break;
			case state::LIST:
				state_ = state_list(c);
				break;
			case state::DATA:
				state_ = state_data(c);
				break;
			default:
				BOOST_ASSERT_MSG(false, "illegal parser state");
				break;
			}
			crc_ = crc_update(crc_, c);
		}

		tokenizer::state tokenizer::start(char c) {

			BOOST_ASSERT(c != 27);	//	no esc
			data_.clear();

			switch (c) {
			case 0:
				crc_ = crc_finalize(crc_);
#ifdef _DEBUG_SML
				std::cout << "EOM: " << crc_ << std::endl;
#endif
				cb_(sml_type::EOM, data_.size(), data_);
				return state::START;
			case 1:
				cb_(sml_type::OPTIONAL, data_.size(), data_);
				return state_;
			default:
				return start_data(c);
			}

			return state_;
		}

		tokenizer::state tokenizer::start_data(char c) {

			//
			//	element size
			//
			length_ = (c & 0x0f);

			//	0x70: 0111 0000
			char const high = (c & 0x70) >> 4;
			switch (static_cast<sml_type>(high)) {
			case sml_type::BINARY:
				BOOST_ASSERT(length_ != 0);
				type_ = sml_type::BINARY;
				return ((c & 0x80) == 0x80) 
					? state::LENGTH
					: state::DATA
					;
			case sml_type::BOOLEAN:	
				BOOST_ASSERT(length_ - 1 == 1);
				type_ = sml_type::BOOLEAN;
				return state::START;
			case sml_type::INTEGER:
				BOOST_ASSERT(length_ - 1 < 9);
				type_ = sml_type::INTEGER;
				return state::DATA;
			case sml_type::UNSIGNED:
				BOOST_ASSERT(length_ - 1 < 9);
				type_ = sml_type::UNSIGNED;
				return state::DATA;
			case sml_type::LIST:
				if (length_ == 0) {
					//	empty list as empty tuple
#ifdef _DEBUG_SML
					std::cout << "{empty}" << std::endl;
#endif
					cb_(sml_type::LIST, 0, data_);
				}
				else if ((c & 0x80) == 0x80) {
					//	more length data
					return state::LIST;
				}
#ifdef _DEBUG_SML
				std::cout << "list [" << length_ << "]" << std::endl;
#endif
				data_.push_back(c);
				cb_(sml_type::LIST, length_, data_);
				break;
			case sml_type::OPTIONAL:
				BOOST_ASSERT_MSG(false, "invalid type");
				break;
			case sml_type::EOM:
				BOOST_ASSERT_MSG(false, "unexpected EOM");
				break;

			default:
				break;
			}

			return state_;
		}

		tokenizer::state tokenizer::state_data(char c) {
			length_--;
			data_.push_back(c);
#ifdef _DEBUG_SML
			if (length_ == 1) {
				std::cout << get_name(type_) << "[" << data_.size() << "]" << std::endl;
			}
#endif
			if (length_ == 1) {
				cb_(type_, data_.size(), data_);
				return state::START;
			}

			return state_;
		}



		tokenizer::state tokenizer::state_length(char c) {
			//
			//	calculate length.
			//	4 bits are used to encode the length info.
			//
			length_ *= 16;
			length_ |= (c & 0x0f);
#ifdef _DEBUG_SML
			std::cout << "length: " << length_ << std::endl;
#endif
			if ((c & 0x80) != 0x80) {
				cb_(sml_type::LIST, length_, data_);
				return state::DATA;
			}
			return state_;
		}
		tokenizer::state tokenizer::state_list(char c) {
			length_ *= 16;
			length_ |= (c & 0x0f);
			--length_;
#ifdef _DEBUG_SML
			std::cout << "list: " << length_ << std::endl;
#endif
			if ((c & 0x80) != 0x80) {
				cb_(sml_type::LIST, length_, data_);
				return state::DATA;
			}
			return state_;
		}		
	}
}
