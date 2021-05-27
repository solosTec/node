#include <smf/sml/parser.h>

#ifdef _DEBUG_SML
#include <iostream>
#include <cyng/io/ostream.h>
#include <cyng/io/serialize.h>
#endif

#include <cyng/obj/factory.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/buffer_cast.hpp>
#include <cyng/obj/value_cast.hpp>
#include <cyng/obj/numeric_cast.hpp>
#include <cyng/obj/container_cast.hpp>

#include <boost/assert.hpp>

#ifdef OPTIONAL
#undef OPTIONAL
#endif

namespace smf {
	namespace sml {


		parser::parser(cb f)
			: tokenizer_(std::bind(&parser::next, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
			, cb_(f)
			, stack_()
			, last_type_(msg_type::UNKNOWN)
		{
			//std::cout << cyng::make_object(cyng::make_buffer("hell\ao")) << std::endl;;
		}

		bool parser::is_closed() const {
			return (last_type_ == msg_type::CLOSE_REQUEST)
				|| (last_type_ == msg_type::CLOSE_RESPONSE)
				;
		}

		void parser::next(sml_type type, std::size_t size, cyng::buffer_t data) {

			switch (type) {

			case sml_type::BINARY:
				BOOST_ASSERT(size == data.size());
				push(cyng::make_object(std::move(data)));
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
				finalize();
				break;

			default:
				break;
			}
		}

		void parser::push(cyng::object&& obj) {
			BOOST_ASSERT(!stack_.empty());
			if (!stack_.empty()) {

#ifdef _DEBUG_SML
				std::cout << "push " << std::string(2 * stack_.size(), '.') << ": ";
				if (obj.rtti().tag() == cyng::TC_BUFFER) {
					auto const buf = cyng::value_cast(obj, cyng::buffer_t());
					if (cyng::is_ascii(buf)) {
						std::cout << std::string(buf.begin(), buf.end()) << std::endl;
					}
					else {
						std::cout << cyng::io::to_typed(obj) << std::endl;
					}
				}
				else {
					std::cout << cyng::io::to_typed(obj) << std::endl;
				}
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
			else {
#ifdef _DEBUG_SML
				std::cout << "empty " << std::string(2 * stack_.size(), '.') << ": " << cyng::io::to_typed(obj) << std::endl;
#endif

			}
		}

		void parser::push_integer(cyng::buffer_t const& data) {
			switch (data.size()) {
			case 1:
				push(cyng::make_object(cyng::to_numeric_le<std::int8_t>(data)));
				break;
			case 2:
				push(cyng::make_object(cyng::to_numeric_le<std::int16_t>(data)));
				break;
			case 3: case 4:
				push(cyng::make_object(cyng::to_numeric_le<std::int32_t>(data)));
				break;
			case 5: case 6: case 7: case 8:
				push(cyng::make_object(cyng::to_numeric_le<std::int64_t>(data)));
				break;
			default:
				BOOST_ASSERT_MSG(false, "invalid length for data type INTEGER");
				break;
			}
		}

		void parser::push_unsigned(cyng::buffer_t const& data) {
			switch (data.size()) {
			case 1:
				push(cyng::make_object(cyng::to_numeric_le<std::uint8_t>(data)));
				break;
			case 2:
				push(cyng::make_object(cyng::to_numeric_le<std::uint16_t>(data)));
				break;
			case 3: case 4:
				push(cyng::make_object(cyng::to_numeric_le<std::uint32_t>(data)));
				break;
			case 5: case 6: case 7: case 8:
				push(cyng::make_object(cyng::to_numeric_le<std::uint64_t>(data)));
				break;
			default:
				BOOST_ASSERT_MSG(false, "invalid length for data type UNSIGNED");
				break;
			}
		}

		void parser::finalize() {

			//BOOST_ASSERT(stack_.size() == 1);
			if (!stack_.empty()) {
				BOOST_ASSERT(stack_.top().values_.size() == 5);
#ifdef _DEBUG_SML
				std::cout << "msg complete " << stack_.top().values_.size() << std::endl;
#endif
				if (stack_.top().values_.size() == 5) {
					//std::cout << "msg complete " << stack_.top().values_ << std::endl;

					auto const trx = cyng::to_buffer(stack_.top().values_.front());
					stack_.top().values_.pop_front();

					auto const group_no = cyng::numeric_cast<std::uint8_t>(stack_.top().values_.front(), 0);
					stack_.top().values_.pop_front();
					auto const abort_on_error = cyng::numeric_cast<std::uint8_t>(stack_.top().values_.front(), 0);
					stack_.top().values_.pop_front();

					auto const choice = cyng::container_cast<cyng::tuple_t>(stack_.top().values_.front());
					stack_.top().values_.pop_front();
					BOOST_ASSERT(choice.size() == 2);
					if ((choice.size() == 2)) {
						last_type_ = to_msg_type(cyng::numeric_cast<std::uint16_t>(choice.front(), 0));
						auto const body = cyng::container_cast<cyng::tuple_t>(choice.back());


						auto const crc = cyng::numeric_cast<std::uint16_t>(stack_.top().values_.front(), 0);

						// 
						//
						//	callback
						//	std::function<void(std::string, std::uint8_t, std::uint8_t, msg_type, cyng::tuple_t, std::uint16_t)>;
						// 
						cb_((trx.size() == 1) ? std::to_string(trx.at(0)) : std::string(trx.begin(), trx.end())
							, group_no
							, abort_on_error
							, last_type_
							, body
							, crc);
					}
				}

				//
				//	clear stack
				//
				std::stack<list>().swap(stack_);
			}
		}

		//
		//	list
		//
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
