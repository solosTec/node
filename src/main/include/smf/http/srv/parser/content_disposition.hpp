/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_HTTP_CONTENT_DISPOSITION_TYPE_H
#define NODE_LIB_HTTP_CONTENT_DISPOSITION_TYPE_H

#include <smf/http/srv/parser/content_type.hpp>//	definition of param_container_t

namespace node	
{
	namespace http
	{
		//	Content-Disposition
		namespace rfc2183
		{
			enum values
			{
				cdv_inline,		//	displayed automatically, [RFC2183]
				cdv_attachment, //	user controlled display, [RFC2183]
				cdv_form_data, //	process as form response, [RFC7578]
				cdv_signal, //	tunneled content to be processed silently, [RFC3204]
				cdv_alert, //	"the body is a custom ring tone to alert the user",[RFC3261]
				cdv_icon, //	the body is displayed as an icon to the user, [RFC3261]
				cdv_render, //	the body should be displayed to the user, [RFC3261]
				cdv_recipient_list_history, //	"the body contains a list of URIs that indicates the recipients of the request",[RFC5364]
				cdv_session, //	"the body describes a communications session, for example, an RFC2327 SDP body",[RFC3261]
				cdv_aib, //	Authenticated Identity Body, [RFC3893]
				cdv_early_session, //	"the body describes an early communications session, for example, and[RFC2327] SDP body",[RFC3959]
				cdv_recipient_list, //	"The body includes a list of URIs to which URI - list services are to be applied.",[RFC5363]
				cdv_notification, //	"the payload of the message carrying this Content - Disposition header field value is an Instant Message Disposition Notification as requested in the corresponding Instant Message.",[RFC5438]
				cdv_by_reference, //	"The body needs to be handled according to a  reference to the body that is located in the same SIP message as the body.",[RFC5621]
				cdv_info_package, //	"The body contains information associated with an Info Package",[RFC6086]
				cdv_recording_session, //	"The body describes either metadata about the RS or the reason	for the metadata snapshot request as determined by the MIME value indicated in the Content - Type.",[RFC7866]
				cdv_initial,	//	unspecified
			};
		}


		/**
		 *	@see https://tools.ietf.org/html/rfc6266
		 *	@see http://www.iana.org/assignments/cont-disp/cont-disp.xhtml
		 * Example:
		 *  Content-Disposition: attachment;
		 *  		filename="EURO rates";
		 *  		filename*=utf-8''%e2%82%ac%20rates
		 *
		 *	@see https://tools.ietf.org/html/rfc6266#section-4.3
		 *	Therefore, when both "filename" and "filename*" are present in a single header field
		 *	value, recipients SHOULD pick "filename*" and ignore "filename".
		 */
		struct content_disposition
		{
			rfc2183::values	type_;
			//	disp-ext-parm
			param_container_t	params_;	//	example: filename="MyFile.jpg"

			inline content_disposition()
				: type_(rfc2183::cdv_initial)
				, params_()
			{}
		};
	}
}

#endif	
