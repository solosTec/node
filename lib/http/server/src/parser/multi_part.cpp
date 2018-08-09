/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/http/srv/parser/multi_part.h>
#include <smf/http/srv/parser/content_disposition.hpp>
#include <smf/http/srv/parser/content_parser.h>
#include <cyng/vm/generator.h>
#include <boost/algorithm/string.hpp>

namespace node
{
	namespace http
	{
		multi_part_parser::multi_part_parser(std::function<void(cyng::vector_t&&)> cb
			, cyng::logging::log_ptr logger
			, std::uint32_t& content_size
			, boost::beast::string_view target
			, boost::uuids::uuid tag)
		: state_(chunk_init_)
			, cb_(cb)
			, logger_(logger)
			, upload_()
			, boundary_()
			, chunck_()
			, content_size_(content_size)
			, target_(target)
			, tag_(tag)
			, upload_size_(0)
			, progress_(0)
		{}

		void multi_part_parser::reset(std::string const& boundary)
		{
#ifdef _DEBUG
			CYNG_LOG_TRACE(logger_, "reset multi-part lexer");
#endif

			//
			//	reset temporary data
			//
			clear(upload_);

			CYNG_LOG_TRACE(logger_, "boundary: " << boundary);
			boundary_ = boundary;

			//
			//	reset upload size
			//
			upload_size_ = 0;
			chunck_.clear();

		}


		bool multi_part_parser::parse(char c)
		{
			//
			//	update total upload size
			//
			upload_size_++;

			if (upload_size_ > content_size_)
			{
				//CYNG_LOG_ERROR(logger_, "more data received then specified "
				//	<< cyy::bytes(upload_size_)
				//	<< " - "
				//	<< cyy::bytes(content_size_)
				//	<< " = "
				//	<< (upload_.data_.size() - content_size_)
				//	<< " bytes");

			}

			std::uint32_t progress = ((100u * upload_size_) / content_size_);
			if (progress != progress_)
			{
				progress_ = progress;

				cb_(cyng::generate_invoke("http.upload.progress"
					, tag_
					, upload_size_
					, content_size_
					, progress_));
			}

			//	manage upload state 
			switch (state_)
			{
			case chunk_init_:
				if (c == '\r')
				{
					//	skip
				}
				else if (c == '\n')
				{
					state_ = chunk_header;
				}
				else
				{
					boundary_ += c;
				}
				break;
			case chunk_boundary:

				if (upload_size_ < 4)
				{
					BOOST_ASSERT_MSG(c == '-', "not a boundary");
				}

				//	store boundary in buffer
				chunck_.push_back(c);

				//	ignore trailing "-" signs (hyphens)
				if (boost::algorithm::equals(chunck_, boundary_))
				{
					state_ = chunk_boundary_cr;
				}
				break;

			case chunk_boundary_cr:
				switch (c)
				{
				case '-':
					state_ = chunk_boundary_end;
					break;
				default:
					if (c != '\r')
					{
						CYNG_LOG_ERROR(logger_, "CR expected");
					}
					state_ = chunk_boundary_nl;
					break;
				}
				break;

			case chunk_boundary_end:
				//	gracefull end of upload
				if (c != '-')
				{
					CYNG_LOG_ERROR(logger_, "- expected");
				}
				else
				{
					if (upload_size_ + 2 == content_size_)
					{
						CYNG_LOG_TRACE(logger_, "end of upload: " << target_);

						//	send HTTP response
						cb_(cyng::generate_invoke("http.upload.complete"
							, tag_
							, true	//	OK
							, upload_size_
							, std::string(target_.begin(), target_.end())));

					}
					else
					{
						CYNG_LOG_WARNING(logger_, "upload incomplete");

						//	send HTTP response
						cb_(cyng::generate_invoke("http.upload.complete"
							, tag_
							, false	//	error
							, upload_size_
							, std::string(target_.begin(), target_.end())));
					}
				}
				state_ = chunk_boundary;
				//	complete
				return true;

			case chunk_boundary_nl:
				//if (!is_nl_(c))
				if (c != '\n')
				{
					CYNG_LOG_ERROR(logger_, "NL expected");
				}

				//	create new part
				state_ = chunk_header;
				chunck_.clear();
				clear(upload_);
				break;

			case chunk_header:
				if (c == '\r')
				{

					//	header line complete
					if (chunck_.empty())
					{
						state_ = chunk_data_start;
					}
					else
					{
						CYNG_LOG_TRACE(logger_, "chunk line: "
							<< chunck_);

						state_ = chunk_header_nl;
					}
				}
				else
				{

					//	read header into buffer
					chunck_.push_back(c);
				}
				break;

			case chunk_header_nl:
				if (boost::algorithm::starts_with(chunck_, "Content-Disposition:"))
				{

					boost::algorithm::replace_first(chunck_, "Content-Disposition:", "");
					content_disposition cd;
					bool b = parse_content_disposition(chunck_, cd);
					if (b)
					{
						//	get variable name
						const std::string var_name = lookup_param(cd.params_, "name");
						CYNG_LOG_TRACE(logger_, "variable name: "
							<< var_name);

						//	get filename
						const std::string filename = lookup_filename(cd.params_);
						if (!filename.empty())
						{
							CYNG_LOG_TRACE(logger_, "upload file: "
								<< filename);
						}

						switch (cd.type_)
						{
						case rfc2183::cdv_inline:
							//	displayed automatically, [RFC2183]
							CYNG_LOG_WARNING(logger_, "inlines not supported yet: "
								<< chunck_);
							break;
						case rfc2183::cdv_attachment:
							//	user controlled display, [RFC2183]
							CYNG_LOG_WARNING(logger_, "attachments not supported yet: "
								<< chunck_);
							break;
						case rfc2183::cdv_form_data:
							//	process as form response, [RFC7578]
							upload_.meta_.assign(cd.params_.begin(), cd.params_.end());
							break;

						default:
							break;
							//	unknown
							CYNG_LOG_ERROR(logger_, "unknown content type: "
								<< chunck_);
						}
					}
					else
					{
						CYNG_LOG_ERROR(logger_, "parsing content disposition failed: "
							<< chunck_);

					}
				}
				else if (boost::algorithm::starts_with(chunck_, "Content-Type:"))
				{
					//	get MIME type
					boost::algorithm::replace_first(chunck_, "Content-Type:", "");
					const bool b = get_http_mime_type(chunck_, upload_.type_);
					if (!b)
					{
						CYNG_LOG_ERROR(logger_, "parsing content type failed: "
							<< chunck_);

					}
					state_ = chunk_boundary_nl;
				}
				else
				{
					CYNG_LOG_ERROR(logger_, "unknown chunk attribute: "
						<< chunck_);
				}

				if (c == '\r')
				{
					CYNG_LOG_ERROR(logger_, "CR expected");
				}

				//	next state
				state_ = chunk_header;
				chunck_.clear();
				break;

			case chunk_data_start:
				if (c == '\r')
				{
					CYNG_LOG_ERROR(logger_, "CR expected");
				}
				state_ = chunk_data;
				break;

			case chunk_data:
			{
				//	detect boundary
				if (c == '-')
				{
					chunck_.clear();
					chunck_.push_back(c);
					state_ = chunk_esc1;
				}
				else
				{

					//	save data into memory
					//	or write on disk
					upload_.data_.push_back(c);
				}

				//	upload is running
			}
			break;

			case chunk_esc1:
				chunck_.push_back(c);
				if (c == '-')
				{
					state_ = chunk_esc2;
				}
				else
				{
					BOOST_ASSERT_MSG(chunck_.size() == 2, "wrong chunk size");
					upload_.data_.insert(upload_.data_.end(), chunck_.begin(), chunck_.end());
					state_ = chunk_data;
				}
				break;

			case chunk_esc2:
				chunck_.push_back(c);
				if (c == '-')
				{
					state_ = chunk_esc3;
				}
				else
				{
					BOOST_ASSERT_MSG(chunck_.size() == 3, "wrong chunk size");
					upload_.data_.insert(upload_.data_.end(), chunck_.begin(), chunck_.end());
					state_ = chunk_data;
				}
				break;

			case chunk_esc3:
				chunck_.push_back(c);
				if (chunck_.size() == boundary_.size())
				{
					if (boost::algorithm::starts_with(chunck_, boundary_))
					{
						std::string var_name = lookup_param(upload_.meta_, "name");

						CYNG_LOG_INFO(logger_, "upload of ["
							<< var_name
							<< "] completed");

						std::string filename = lookup_filename(upload_.meta_);

						//
						//	invoke callback
						//
						if (filename.empty() && (upload_.data_.size() > 2))
						{
							//	remove <CR><NL> tail
							upload_.data_.pop_back();
							upload_.data_.pop_back();

							CYNG_LOG_TRACE(logger_, var_name
								<< " = "
								<< std::string(upload_.data_.begin(), upload_.data_.end()));

							cb_(cyng::generate_invoke("http.upload.var"
								, tag_
								, var_name
								, std::string(upload_.data_.begin(), upload_.data_.end())
							));
						}
						else
						{

							cb_(cyng::generate_invoke("http.upload.data"
								, tag_
								, var_name
								, filename
								, to_str(upload_.type_)
								//, upload_.type_
								, upload_.data_
								//, cyy::factory(request_.uri_.path_)
							));
						}

						//	next boundary
						state_ = chunk_boundary_cr;
					}
					else
					{
						//	restore data
						upload_.data_.insert(upload_.data_.end(), chunck_.begin(), chunck_.end());
						//	continue read data stream
						state_ = chunk_data;
					}
				}
				break;

			default:
				CYNG_LOG_FATAL(logger_, "illegal upload state: "
					<< state_);
				break;
			}

			//
			//	stay in multipart state
			//
			return false;
		}

		std::string multi_part_parser::lookup_filename(param_container_t const& phrases)
		{
			//	The parameters "filename" and "filename*" differ only in that "filename*" uses the encoding 
			//	defined in RFC 5987. When both "filename" and "filename*" are present in a single header field value, 
			//	"filename*" is preferred over "filename" when both are present and understood.

			const std::string filename = lookup_param(phrases, "filename*");
			return (filename.empty())
				? lookup_param(phrases, "filename")
				: filename
				;
		}

		multi_part_parser::multi_part::multi_part()
			: size_(0)
			, data_()
			, meta_()
			, type_()
		{}

		void clear(multi_part_parser::multi_part& req)
		{
			multi_part_parser::multi_part r;
			std::swap(req, r);
		}

	}
}
