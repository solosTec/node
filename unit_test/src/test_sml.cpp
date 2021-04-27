#ifdef STAND_ALONE
#   define BOOST_TEST_MODULE unit_test
#endif

#include <boost/test/unit_test.hpp>

#include <smf/sml/parser.h>
#include <smf/sml/tokenizer.h>

#include <cyng/io/ostream.h>

#include <iostream>
#include <fstream>


BOOST_AUTO_TEST_SUITE(sml_suite)

BOOST_AUTO_TEST_CASE(tokenizer)
{
	std::ifstream ifile("..\\..\\unit_test\\assets\\smf--SML-202012T071425-power@solostec8bbb6264-bc4a4b92.sml", std::ios::binary);
	BOOST_CHECK(ifile.is_open());
	if (ifile.is_open())
	{
		//	dont skip whitespaces
		ifile >> std::noskipws;
		smf::sml::tokenizer p([](smf::sml::sml_type type, std::size_t size, cyng::buffer_t data) {

			std::cout
				<< smf::sml::get_name(type)
				<< '['
				<< size
				<< '/'
				<< data.size()
				<< ']'
				<< std::endl;

			});
		p.read(std::istream_iterator<char>(ifile), std::istream_iterator<char>());

	}

}

BOOST_AUTO_TEST_CASE(parser)
{
	std::ifstream ifile("..\\..\\unit_test\\assets\\smf--SML-202012T071425-power@solostec8bbb6264-bc4a4b92.sml", std::ios::binary);
	BOOST_CHECK(ifile.is_open());
	if (ifile.is_open())
	{
		//	dont skip whitespaces
		ifile >> std::noskipws;
		smf::sml::parser p;// ([](smf::sml::sml_type type, std::size_t size, cyng::buffer_t data) {

		//	std::cout 
		//		<< smf::sml::get_name(type) 
		//		<< '['
		//		<< size
		//		<< '/'
		//		<< data.size() 
		//		<< ']'
		//		<< std::endl;

		//	});
		p.read(std::istream_iterator<char>(ifile), std::istream_iterator<char>());

	}

}


BOOST_AUTO_TEST_SUITE_END()