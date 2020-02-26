/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#include "print_build_info.h"
#include <NODE_project_info.h>
#include <CYNG_project_info.h>
#include <boost/config.hpp>
#include <boost/version.hpp>
#include <boost/asio/version.hpp>
#include <boost/predef.h>
#include <ctime>
#include <chrono>

//
//  Because of the macros __DATE__ and __TIME__ the binary
//  depends on the time it was build. We have to tell GCC that is no
//  problem so far.
//
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdate-time"
#endif

namespace node 
{
	int print_build_info(std::ostream& os)
	{
		os
		<< "configured at : "
		<< NODE_BUILD_DATE
		<< " UTC"
		<< std::endl

		<< "last built at : "
		<< __DATE__
		<< " "
		<< __TIME__
		<< std::endl

		<< "Platform      : "
		<< NODE_PLATFORM
		<< std::endl

		<< "Compiler      : "
		<< BOOST_COMPILER
#if BOOST_OS_WINDOWS
		<< " ("
		<< (_MSC_FULL_VER / 10000000)		//	major version
		<< '.'
		<< ((_MSC_FULL_VER / 100000) % 100)	//	minor version
		<< '.'
		<< (_MSC_FULL_VER % 100000)	//	patch level
		<< ")"
#endif
		<< std::endl

		<< "StdLib        : "
		<< BOOST_STDLIB
		<< std::endl

		<< "BOOSTLib      : v"
		<< (BOOST_VERSION / 100000)		//	major version
		<< '.'
		<< (BOOST_VERSION / 100 % 1000)	//	minor version
		<< '.'
		<< (BOOST_VERSION % 100)	//	patch level
		<< " ("
        << NODE_BOOST_VERSION
		<< ")"
		<< std::endl

		<< "Boost.Asio    : v"
		<< (BOOST_ASIO_VERSION / 100000)
		<< '.'
		<< (BOOST_ASIO_VERSION / 100 % 1000)
		<< '.'
		<< (BOOST_ASIO_VERSION % 100)
		<< std::endl

		<< "CyngLib       : v"
		<< CYNG_VERSION
		<< " ("
		<< CYNG_BUILD_DATE
		<< ")"
		<< std::endl

#ifdef NODE_SSL_INSTALLED
		<< "SSL/TSL       : v"
        << NODE_SSL_VERSION
#else
		<< "SSL/TSL       : not supported"
#endif
        << std::endl

		<< "build type    : "
#if BOOST_OS_WINDOWS
#ifdef _DEBUG
		<< "Debug"
#else
		<< "Release"
#endif
#else
		<< NODE_BUILD_TYPE
#endif
		<< std::endl

        << "address model : "
        << NODE_ADDRESS_MODEL
        << " bit"
        << std::endl
		<< std::endl

		<< "features      : C++2a"
		<< std::endl
		
			
#ifdef __CPP_SUPPORT_P1099R5
		<< "P1099R5: using enums" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0409R2
		<< "P0409R2: Allow lambda-capture [=, this]" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0306R4
		<< "P0306R4: __VA_OPT__" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0329R4
		<< "P0329R4: Designated initializers" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0428R2
		<< "P0428R2: template-parameter-list for generic lambdas" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0683R1
		<< "P0683R1: Default member initializers for bit-fields" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0702R1
		<< "P0702R1: Initializer list constructors in class template argument deduction" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0704R1
		<< "P0704R1: const&-qualified pointers to members" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0734R0
		<< "P0734R0: Concepts" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0315R4
		<< "P0315R4: Lambdas in unevaluated contexts" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0515R3
		<< "P0515R3: Three-way comparison operator" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0588R1
		<< "P0588R1: Simplifying implicit lambda capture" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0614R1
		<< "P0614R1: init-statements for range-based for" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0624R2
		<< "P0624R2: Default constructible and assignable stateless lambdas" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0641R2
		<< "P0641R2: const mismatch with defaulted copy constructor" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0692R1
		<< "P0692R1: Access checking on specializations" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0846R0
		<< "P0846R0: ADL and function templates that are not visible" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0859R0
		<< "P0859R0: Less eager instantiation of constexpr functions" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0479R5
		<< "P0479R5: Attributes [[likely]] and [[unlikely]]" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0634R3
		<< "P0634R3: Make typename more optional" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0780R2
		<< "P0780R2: Pack expansion in lambda init-capture" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0840R2
		<< "P0840R2: Attribute [[no_unique_address]]" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0542R5
		<< "P0542R5: Contracts" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0722R3
		<< "P0722R3: Destroying operator delete" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0732R2
		<< "P0732R2: Class types in non-type template parameters" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0892R2
		<< "P0892R2: explicit(bool)" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0941R2
		<< "P0941R2: Integrating feature-test macros" << std::endl
#endif
#ifdef __CPP_SUPPORT_P1008R1
		<< "P1008R1: Prohibit aggregates with user-declared constructors" << std::endl
#endif
#ifdef __CPP_SUPPORT_P1064R0
		<< "P1064R0: constexpr virtual function" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0482R6
		<< "P0482R6: char8_t" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0595R2
		<< "P0595R2: std::is_constant_evaluated()" << std::endl
#endif
#ifdef __CPP_SUPPORT_P1002R1
		<< "P1002R1: constexpr try-catch blocks" << std::endl
#endif
#ifdef __CPP_SUPPORT_P1073R3
		<< "P1073R3: Immediate function" << std::endl
#endif
#ifdef __CPP_SUPPORT_P1094R2
		<< "P1094R2: Nested inline namespaces" << std::endl
#endif
#ifdef __CPP_SUPPORT_P1327R1
		<< "P1327R1: constexpr dynamic_cast and polymorphic typeid" << std::endl
#endif
#ifdef __CPP_SUPPORT_P1330R0
		<< "P1330R0: Changing the active member of a union inside constexpr" << std::endl
#endif
#ifdef __CPP_SUPPORT_P1091R3
		<< "P1091R3: Structured binding extensions" << std::endl
#endif
#ifdef __CPP_SUPPORT_P1041R4
		<< "P1041R4: Stronger Unicode requirements" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0960R3
		<< "P0960R3: Parenthesized initialization of aggregates" << std::endl
#endif
#ifdef __CPP_SUPPORT_P1103R3
		<< "P1103R3: Modules" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0912R5
		<< "P0912R5: Coroutines" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0463R1
		<< "P0463R1: std::endian" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0550R2
		<< "P0550R2: std::remove_cvref" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0674R1
		<< "P0674R1: Extending std::make_shared to support arrays" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0020R6
		<< "P0020R6: Floating point atomic" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0053R7
		<< "P0053R7: Synchronized buffered ostream" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0202R3
		<< "P0202R3: constexpr for <algorithm> and <utility>" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0415R1
		<< "P0415R1: More constexpr for <complex>" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0457R2
		<< "P0457R2: String prefix and suffix checking" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0515R3
		<< "P0515R3: Library support for operator<=> <compare>" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0653R2
		<< "P0653R2: Utility to convert a pointer to a raw pointer" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0718R2
		<< "P0718R2: Atomic shared_ptr and weak_ptr" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0122R7
		<< "P0122R7: std::span" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0355R7
		<< "P0355R7: Calendar and timezone" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0754R2
		<< "P0754R2: <version>" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0019R8
		<< "P0019R8: std::atomic_ref" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0556R3
		<< "P0556R3: Integral power-of-2 operations" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0476R2
		<< "P0476R2: std::bit_cast()" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0722R3
		<< "P0722R3: std::destroying_delete" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0898R3
		<< "P0898R3: Concepts library" << std::endl
#endif
#ifdef __CPP_SUPPORT_P1209R0
		<< "Consistent container erasure" << std::endl
#endif
#ifdef __CPP_SUPPORT_P1143R2
		<< "P1143R2: constinit" << std::endl
#endif


//  C++17 features
// 	g++ -std=c++17
// 	cl /std:c++latest
// 
		<< "features      : C++17"
		<< std::endl

#ifdef __CPP_SUPPORT_N3291
		<< "N3291: reverse_copy" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3911
		<< "N3911: std::void_t" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3915
		<< "N3915: apply() call a function with arguments from a tuple" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3922
		<< "N3922: New auto rules for direct-list-initialization" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3928
		<< "N3928: static_assert with no message" << std::endl
#endif
#ifdef __CPP_SUPPORT_N4051
		<< "N4051: typename in a template template parameter" << std::endl
#endif
#ifdef __CPP_SUPPORT_N4086
		<< "N4086: Removing trigraphs" << std::endl
#endif
#ifdef __CPP_SUPPORT_N4230
		<< "N4230: Nested namespace definition" << std::endl
#endif
#ifdef __CPP_SUPPORT_N4266
		<< "N4230: Attributes for namespaces and enumerators" << std::endl
#endif
#ifdef __CPP_SUPPORT_N4267
		<< "N4267: u8 character literals" << std::endl
#endif
#ifdef __CPP_SUPPORT_N4268
		<< "N4268: Allow constant evaluation for all non-type template arguments" << std::endl
#endif
#ifdef __CPP_SUPPORT_N4295
		<< "N4295: Fold Expressions" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0036R0
		<< "P0036R0: Unary fold expressions and empty parameter packs" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0012R1
		<< "P0012R1: Make exception specifications part of the type system" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0017R1
		<< "P0017R1: Aggregate initialization of classes with base classes" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0061R1
		<< "P0061R1: __has_include in preprocessor conditionals" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0136R1
		<< "P0136R1: New specification for inheriting constructors (DR1941 et al)" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0018R3
		<< "P0018R3: Lambda capture of *this" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0138R2
		<< "P0138R2: Direct-list-initialization of enumerations" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0170R1
		<< "P0170R1: constexpr lambda expressions" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0184R0
		<< "P0184R0: Differing begin and end types in range-based for" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0188R1
		<< "P0188R1: [[fallthrough]] attribute" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0189R1
		<< "P0189R1: [[nodiscard]] attribute" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0212R1
		<< "P0212R1: [[maybe_unused]] attribute" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0245R1
		<< "P0245R1: Hexadecimal floating-point literals" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0028R4
		<< "P0028R4: Using attribute namespaces without repetition" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0035R4
		<< "P0035R4: Dynamic memory allocation for over-aligned data" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0091R3
		<< "P0091R3: Class template argument deduction" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0127R2
		<< "P0127R2: Non-type template parameters with auto type" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0135R1
		<< "P0135R1: Guaranteed copy elision" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0137R1
		<< "P0137R1: Replacement of class objects containing reference members" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0145R3
		<< "P0145R3: Stricter expression evaluation order" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0217R3
		<< "P0217R3: Structured Bindings (like auto [...] = f())" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0283R2
		<< "P0283R2: Ignore unknown attributes" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0292R2
		<< "P0292R2: constexpr if statements" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0305R1
		<< "P0305R1: init-statements for if and switch" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0386R2
		<< "P0386R2: Inline variables" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0003R5
		<< "P0003R5: Removing Deprecated Exception Specifications from C++17" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0195R2
		<< "P0195R2: Pack expansions in using-declarations" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0522R0
		<< "P0522R0: DR: Matching of template template-arguments excludes compatible templates" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3921
		<< "N3921 : std::string_view" << std::endl
#endif
#ifdef __CPP_SUPPORT_N4259
		<< "N4259 : std::uncaught_exceptions()" << std::endl
#endif
#ifdef __CPP_SUPPORT_N4387
		<< "N4387 : Improving std::pair and std::tuple" << std::endl
#endif
#ifdef __CPP_SUPPORT_N4391
		<< "N4391 : Library Fundamentals 2 TS - make_array" << std::endl
#endif
#ifdef __CPP_SUPPORT_N4508
		<< "N4508 : std::shared_mutex (untimed)" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0024R2
		<< "P0024R2: Standardization of Parallelism TS" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0154R1
		<< "P0154R1: Hardware interference size" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0218R1
		<< "P0218R1: File system library" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0220R1
		<< "P0220R1: std::any / std::optional" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0226R1
		<< "P0226R1: Mathematical special functions" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0063R3
		<< "P0063R3: C++17 should refer to C11 instead of C99" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0083R3
		<< "P0083R3: Splicing Maps and Sets" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0088R3
		<< "P0088R3: std::variant" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0067R5
		<< "P0067R5: Elementary string conversions" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0298R3
		<< "P0298R3: std::byte" << std::endl
#endif
#ifdef __CPP_SUPPORT_P0156R0
		<< "P0156R0: std::scoped_lock" << std::endl
#endif


// 	C++14 features
// 	g++ -std=c++14
// 	cl /std:c++latest

		<< "features      : C++14"
		<< std::endl

#ifdef __CPP_SUPPORT_N3323
		<< "N3323: Tweaked wording for contextual conversions" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3472
		<< "N3472: Binary literals" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3638
		<< "N3638: decltype(auto), Return type deduction for normal functions" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3648
		<< "N3648: Initialized/Generalized lambda captures (init-capture)" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3649
		<< "N3649: Generic (polymorphic) lambda expressions" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3651
		<< "N3651: Variable templates" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3652
		<< "N3652: Extended constexpr" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3653
		<< "N3653: Member initializers and aggregates (NSDMI)" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3664
		<< "N3664: Clarifying memory allocation (avoiding/fusing allocations)" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3760
		<< "N3760: [[deprecated]] attribute" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3778
		<< "N3778: Sized deallocation" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3781
		<< "N3781: Single quote as digit separator" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3302
		<< "N3302: constexpr for <complex>" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3462
		<< "N3462: std::result_of and SFINAE" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3469
		<< "N3469: constexpr for <chrono>" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3470
		<< "N3470: constexpr for <array>" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3471
		<< "N3471: constexpr for <initializer_list>, <utility> and <tuple>" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3545
		<< "N3545: Improved std::integral_constant" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3642
		<< "N3642: User-defined literals for <chrono> and <string>" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3644
		<< "N3644: Null forward iterators" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3654
		<< "N3654: std::quoted" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3657
		<< "N3657: Heterogeneous associative lookup" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3658
		<< "N3658: std::integer_sequence" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3658
		<< "N3658: integer_sequence, index_sequence, make_index_sequence etc., to allow tag dispatch on integer packs." << std::endl
#endif
#ifdef __CPP_SUPPORT_N3659
		<< "N3659: std::shared_timed_mutex" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3668
		<< "N3668: std::exchange" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3669
		<< "N3669: fixing constexpr member functions without const" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3670
		<< "N3670: std::get<T>()" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3671
		<< "N3671: Dual-Range std::equal, std::is_permutation, std::mismatch" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3656
		<< "N3656: make_unique (Revision 1)" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3857
		<< "N3857: std::promise" << std::endl
#endif

//
//   C++11 features
//

		<< "features      : C++11"
		<< std::endl

#ifdef __CPP_SUPPORT_N2341
		<< "N2341: alignas" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2341
		<< "N2341: alignof" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2427
		<< "N2427: Atomic operations" << std::endl
#endif
#ifdef __CPP_SUPPORT_N1984
		<< "N1984: auto" << std::endl
#endif
#ifdef __CPP_SUPPORT_N1653
		<< "N1653: C99 preprocessor" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2235
		<< "N2235: constexpr" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2343
		<< "N2343: decltype" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2346
		<< "N2346: Defaulted and deleted functions" << std::endl
#endif
#ifdef __CPP_SUPPORT_N1986
		<< "N1986: Delegating constructors" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2437
		<< "N2437: Explicit conversion operators" << std::endl
#endif
#ifdef __CPP_SUPPORT_N1791
		<< "N1791: Extended friend declarations" << std::endl
#endif
#ifdef __CPP_SUPPORT_N1987
		<< "N1987: extern template" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2764
		<< "N2764: Forward enum declarations" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2540
		<< "N2540: Inheriting constructors" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2672
		<< "N2672: Initializer lists" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2550
		<< "N2550: Lambda expressions" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2657
		<< "N2657: Local and unnamed types as template parameters" << std::endl
#endif
#ifdef __CPP_SUPPORT_N1811
		<< "N1811: long long" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2535
		<< "N2535: Inline namespaces" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2249
		<< "N2249: New character types" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2541
		<< "N2541: Trailing function return types" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2431
		<< "N2431: nullptr" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2442
		<< "N2442: Unicode/Raw string string literals" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2765
		<< "N2765: User-defined literals" << std::endl
#endif
#ifdef __CPP_SUPPORT_N1757
		<< "N1757: Right angle brackets" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2118
		<< "N2118: RValue references" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2844
		<< "N2844: RValue references v2.0" << std::endl
#endif
#ifdef __CPP_SUPPORT_N1720
		<< "N1720: static_assert" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2347
		<< "N2347: Strongly-typed enum" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2258
		<< "N2258: Template aliases" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2659
		<< "N2659: Thread-local storage" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2544
		<< "N2544: Unrestricted unions" << std::endl
#endif
#ifdef __CPP_SUPPORT_N1836
		<< "N1836: Type traits" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2242
		<< "N2242: Variadic templates" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2930
		<< "N2930: Range-for loop" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2761
		<< "N2761: Attributes" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2439
		<< "N2439: ref-qualifiers" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2756
		<< "N2756: Non-static data member initializers" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2660
		<< "N2660: Dynamic initialization and destruction with concurrency (magic statics)" << std::endl
#endif
#ifdef __CPP_SUPPORT_N3050
		<< "N3050: noexcept" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2670
		<< "N2670: Garbage Collection and Reachability-Based Leak Detection" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2670
		<< "N2670: Garbage Collection and Reachability-Based Leak Detection (library support)" << std::endl
#endif
#ifdef __CPP_SUPPORT_N2071
		<< "N2071: Money, Time, and hexfloat I/O manipulators" << std::endl
#endif
		;

        return EXIT_SUCCESS;
	}
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
