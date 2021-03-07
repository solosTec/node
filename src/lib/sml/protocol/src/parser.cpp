#include <smf/sml/parser.h>

#ifdef _DEBUG_SML
#include <iostream>
#include <cyng/io/ostream.h>
#include <cyng/io/serialize.h>
#endif

#include <cyng/obj/factory.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/buffer_cast.hpp>

#include <boost/assert.hpp>

#ifdef OPTIONAL
#undef OPTIONAL
#endif

namespace smf {
	namespace sml {


		parser::parser()
			: tokenizer_(std::bind(&parser::next, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
			, stack_()
		{}

		void parser::next(sml_type type, std::size_t size, cyng::buffer_t data) {

			switch (type) {

			case sml_type::BINARY:
				BOOST_ASSERT(size == data.size());
				push(cyng::make_object(data));
				break;
			case sml_type::BOOLEAN:
				BOOST_ASSERT(data.size() == 1);
				if (!data.empty()) {
					//	boolean
					push(cyng::make_object(data.at(0) != 0));
				}
				break;
			case sml_type::INTEGER:
				BOOST_ASSERT(size == data.size());
				push_integer(data);
				break;
			case sml_type::UNSIGNED:
				BOOST_ASSERT(size == data.size());
				push_unsigned(data);
				break;
			case sml_type::LIST:
				if (size == 0) {
					//
					//	empty list as empty tuple
					//
					push(cyng::tuple_factory());
				}
				else {
					stack_.push(list(size));
				}
				break;
			case sml_type::OPTIONAL:
				push(cyng::make_object());
				break;
			case sml_type::EOM:
				break;

			default:
				break;
			}
		}

		void parser::push(cyng::object&& obj) {
			BOOST_ASSERT(!stack_.empty());
			if (!stack_.empty()) {

#ifdef _DEBUG_SML
				std::cout << "push " << std::string(2 * stack_.size(), '.') << ": " << cyng::io::to_typed(obj) << std::endl;
#endif

				if (stack_.top().push(obj)) {
					//
					//	SML list complete: reduce list
					//
					auto const tmp = stack_.top().values_;

#ifdef _DEBUG_SML
					std::cout << "reduce [" << stack_ .size() << "]: "<< tmp << std::endl;
#endif

					stack_.pop();
					push(cyng::make_object(tmp));	//	recursion (!)

				}
			}
		}

		void parser::push_integer(cyng::buffer_t const& data) {
			switch (data.size()) {
			case 1:
				push(cyng::make_object(cyng::to_numeric_be<std::int8_t>(data)));
				break;
			case 2:
				push(cyng::make_object(cyng::to_numeric_be<std::int16_t>(data)));
				break;
			case 3: case 4:
				push(cyng::make_object(cyng::to_numeric_be<std::int32_t>(data)));
				break;
			case 5: case 6: case 7: case 8:
				push(cyng::make_object(cyng::to_numeric_be<std::int64_t>(data)));
				break;
			default:
				BOOST_ASSERT_MSG(false, "invalid length for data type INTEGER");
				break;
			}
		}

		void parser::push_unsigned(cyng::buffer_t const& data) {
			switch (data.size()) {
			case 1:
				push(cyng::make_object(cyng::to_numeric_be<std::uint8_t>(data)));
				break;
			case 2:
				push(cyng::make_object(cyng::to_numeric_be<std::uint16_t>(data)));
				break;
			case 3: case 4:
				push(cyng::make_object(cyng::to_numeric_be<std::uint32_t>(data)));
				break;
			case 5: case 6: case 7: case 8:
				push(cyng::make_object(cyng::to_numeric_be<std::uint64_t>(data)));
				break;
			default:
				BOOST_ASSERT_MSG(false, "invalid length for data type UNSIGNED");
				break;
			}
		}

		parser::list::list(std::size_t size)
			: size_(size)
			, values_()
		{}

		bool parser::list::push(cyng::object obj) {
			BOOST_ASSERT(values_.size() < size_);
			values_.push_back(obj);
			return values_.size() == size_;

		}

		std::size_t parser::list::pos() const {
			return size_ - values_.size();
		}

		
	}
}
