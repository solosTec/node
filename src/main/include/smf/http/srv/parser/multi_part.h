/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_HTTP_MULTI_PART_PARSER_H
#define NODE_LIB_HTTP_MULTI_PART_PARSER_H

#include <smf/http/srv/parser/content_type.hpp>
#include <cyng/log.h>
#include <cyng/intrinsics/sets.h>
#include <cyng/intrinsics/buffer.h>
#include <boost/beast/core.hpp>
#include <boost/uuid/uuid.hpp>

namespace node
{
	namespace http
	{
		/**
		 * parser for HTTP uploads and forms
		 *	@see http://tools.ietf.org/html/rfc1867
		 */
		class multi_part_parser
		{
		private:
			/**
			 * part of a multipart/form-data
			 */
			struct multi_part
			{

				multi_part();

				std::size_t				size_;
				cyng::buffer_t			data_;
				param_container_t		meta_;
				mime_content_type		type_;
			};

			friend 	void clear(typename multi_part_parser::multi_part& req);

		public:
			multi_part_parser(std::function<void(cyng::vector_t&&)> cb
				, cyng::logging::log_ptr
				, std::uint64_t&
				, boost::beast::string_view target
				, boost::uuids::uuid tag);

			/**
			 * Put next available character into state machine
			 *
			 * @return true if complete
			 */
			bool parse(char c);

			template < typename I >
			void parse(I start, I end)
			{
				std::for_each(start, end, [this](char c) {
					this->parse(c);
				});
			}

			/**
			 * Start new upload/form
			 */
			void reset(std::string const& boundary);

		private:
			/**
			 * The parameters "filename" and "filename*" differ only in that "filename*" uses the encoding
			 * defined in RFC 5987. When both "filename" and "filename*" are present in a single header field value,
			 * "filename*" is preferred over "filename" when both are present and understood.
			 */
			std::string lookup_filename(param_container_t const& phrases);

		private:
			//	chunk states (multipart)
			enum chunk_enum
			{
				chunk_init_,	//	read the first line containing the boundary
				chunk_boundary_,
				chunk_boundary_cr_,
				chunk_boundary_nl_,
				chunk_boundary_end_,
				chunk_header_,
				chunk_header_nl_,
				chunk_data_start_,
				chunk_data_,
				chunk_esc1_,
				chunk_esc2_,
				chunk_esc3_,

			}	state_;


			void update_column(char c);
			chunk_enum chunk_init(char c);
			chunk_enum chunk_boundary(char c);
			chunk_enum chunk_boundary_cr(char c);
			chunk_enum chunk_boundary_end(char c);
			chunk_enum chunk_boundary_nl(char c);
			chunk_enum chunk_header(char c);
			chunk_enum chunk_header_nl(char c);
			chunk_enum chunk_data(char c);
			chunk_enum chunk_esc3(char c);

			/**
			 * completion callback
			 */
			std::function<void(cyng::vector_t&&)> cb_;

			/**
			 * threadsafe logger class
			 */
			cyng::logging::log_ptr	logger_;

			/**
			 * Contains upload data
			 */
			multi_part	upload_;

			/**
			 * Each multipart entry is enveloped into a unique
			 * boundary string.
			 */
			std::string boundary_;

			/**
			 * buffer for incoming boundary and attribute data
			 */
			std::string chunck_;

			/**
			 * Specified content size
			 */
			std::uint64_t&	content_size_;

			/**
			 * Contains all HTTP header data
			 */
			boost::beast::string_view target_;

			/**
			 * session tag
			 */
			boost::uuids::uuid tag_;

			/**
			 * Totally bytes already uploaded
			 */
			std::uint32_t	upload_size_;

			/**
			 * calculation of upload progress
			 */
			std::uint32_t progress_;

			/**
			 * current column
			 */
			std::size_t column_;
		};

	}
}
#endif // NODE_LIB_HTTP_MULTI_PART_PARSER_H
