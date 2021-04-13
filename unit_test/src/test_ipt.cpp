#ifdef STAND_ALONE
#   define BOOST_TEST_MODULE unit_test
#endif

#include <boost/test/unit_test.hpp>

#include <smf/ipt/parser.h>
#include <smf/ipt/serializer.h>

#include <cyng/io/ostream.h>

#include <iostream>
#include <fstream>


BOOST_AUTO_TEST_SUITE(ipt_suite)

void foo(cyng::buffer_t b) {
	std::cout << b.size() << std::endl;
	b.clear();
}

BOOST_AUTO_TEST_CASE(scrambler)
{
	auto b = cyng::make_buffer("hello, world! 1234567890");
	foo(std::move(b));
	BOOST_REQUIRE(b.empty());

	auto b2 = cyng::make_buffer("hello, world! 1234567890");
	auto b3 = std::move(b2);
	BOOST_REQUIRE(b2.empty());

	auto const sk = smf::ipt::gen_random_sk();

	smf::ipt::parser p(sk
		, [](smf::ipt::header const&, cyng::buffer_t&&) {

		}
		, [](cyng::buffer_t&& data) {
			std::string str(data.begin(), data.end());
			std::cout << str << std::endl;
		});

	smf::ipt::serializer s(sk);

	auto const data = s.scramble(cyng::make_buffer("hello, world! 1234567890"));
	auto const descrambled = p.read(data.begin(), data.end());

	BOOST_REQUIRE_EQUAL(data.size(), descrambled.size());
	if (data.size() == descrambled.size()) {
		for (std::size_t idx = 0; idx < data.size(); ++idx) {
			BOOST_REQUIRE_EQUAL(data.at(idx), descrambled.at(idx));
		}
	}
}


BOOST_AUTO_TEST_SUITE_END()