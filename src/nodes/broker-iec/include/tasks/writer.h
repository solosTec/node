/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_IEC_TASK_WRITER_H
#define SMF_IEC_TASK_WRITER_H

#include <db.h>

#include <smf/cluster/bus.h>
#include <smf/iec/parser.h>

#include <cyng/obj/intrinsics/eod.h>
#include <cyng/log/logger.h>
#include <cyng/task/task_fwd.h>
#include <cyng/store/key.hpp>

#include <fstream>

namespace smf {

	class writer
	{
		template <typename T >
		friend class cyng::task;

		using signatures_t = std::tuple<
			std::function<void(std::string)>,
			std::function<void(cyng::obis code, std::string value, std::string unit)>,
			std::function<void()>,
			std::function<void(cyng::eod)>
		>;

	public:
		writer(std::weak_ptr<cyng::channel>
			, cyng::logger
			, std::filesystem::path out);

		void stop(cyng::eod);

	private:
		void open(std::string);
		void store(cyng::obis code, std::string value, std::string unit);
		void commit();

	private:
		signatures_t sigs_;
		cyng::channel_weak channel_;
		cyng::logger logger_;
		std::filesystem::path const out_;
		std::ofstream ostream_;
	};

}

#endif
