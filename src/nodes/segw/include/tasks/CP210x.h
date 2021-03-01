/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_TASK_CP210X_H
#define SMF_SEGW_TASK_CP210X_H

#include <smf/hci/parser.h>

#include <cyng/task/task_fwd.h>
#include <cyng/log/logger.h>

namespace smf
{
	/**
	 * receiver and parser for CP210x host control protocol (HCP)
	 */
	class CP210x
	{
		template <typename T >
		friend class cyng::task;

		using signatures_t = std::tuple<
			std::function<void(cyng::eod)>,
			std::function<void(cyng::buffer_t)>
		>;

	public:
		CP210x(std::weak_ptr<cyng::channel>
			, cyng::logger);



	private:
		void stop(cyng::eod);

		/**
		 * message complete
		 */
		void put(hci::msg const&);

		/**
		 * parse incoming raw data
		 */
		void receive(cyng::buffer_t);

	private:
		signatures_t sigs_;
		std::weak_ptr<cyng::channel> channel_;

		/**
		 * global logger
		 */
		cyng::logger logger_;

		hci::parser parser_;

	};

}

#endif