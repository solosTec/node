/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "transfer_data.h"
#include <smf/ipt/response.hpp>

#include <cyng/vm/generator.h>
#include <cyng/io/io_bytes.hpp>

#include <numeric>

namespace node
{
	transfer_data::transfer_data(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::controller& vm
		, std::uint32_t channel
		, std::uint32_t source
		, cyng::buffer_t const& data
		, std::uint16_t packet_size
		, ipt::bus_interface& bus
		, std::size_t tsk)
	: base_(*btp)
		, logger_(logger)
		, vm_(vm)
		, channel_(channel)
		, source_(source)
		, chuncks_(make_chuncks(data, packet_size))
		, bus_(bus)
		, task_(tsk)
		, seq_(0)
		, block_(0)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< ':'
			<< vm_.tag()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< channel_
			<< ':'
			<< source_
			<< " transfer "
			<< cyng::bytes_to_str(get_total_size(chuncks_)));

		BOOST_ASSERT(get_total_size(chuncks_) == data.size());
	}

	cyng::continuation transfer_data::run()
	{	
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< channel_
			<< ':'
			<< source_
			<< " - "
			<< cyng::bytes_to_str(get_total_size(chuncks_)));

		//
		//	ToDo: set timeout
		//

		return send_next_block()
			? cyng::continuation::TASK_CONTINUE
			: cyng::continuation::TASK_STOP
			;
	}

	void transfer_data::stop(bool shutdown)
	{
		//
		//	send completion info
		//
		base_.mux_.post(task_, 3u, cyng::tuple_factory(is_complete(), channel_, source_, block_));

		//
		//	remove from task db
		//
		vm_.async_run(cyng::generate_invoke("bus.remove.relation", base_.get_id()));

		//
		//	terminate task
		//
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is stopped");

	}

	//	slot 0
	cyng::continuation transfer_data::process(bool success
		, std::uint32_t channel
		, std::uint32_t source
		, std::uint8_t status
		, std::uint8_t block)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> bus "
			<< vm_.tag()
			<< " received response "
			<< success
			<< ", status: "
			<< +status
			<< ", block: "
			<< +block);

		return send_next_block()
			? cyng::continuation::TASK_CONTINUE
			: cyng::continuation::TASK_STOP
			;
	}

	//	slot 1
	cyng::continuation transfer_data::process(ipt::sequence_type seq)
	{
		//BOOST_ASSERT(seq_ == 0u);
		//BOOST_ASSERT(seq != 0u);
		seq_ = seq;
		return cyng::continuation::TASK_CONTINUE;
	}

	bool transfer_data::send_next_block()
	{
		if (is_complete()) {

			std::uint8_t const status_{ 0x40 };	//	set SYN flag

			//
			//	send data block
			//
			vm_.async_run({ cyng::generate_invoke("req.transfer.push.data", channel_, source_, status_, block_ + 1, chuncks_.at(block_))
				, cyng::generate_invoke("bus.store.relation", cyng::invoke("ipt.seq.push"), base_.get_id())
				, cyng::generate_invoke("stream.flush")
				, cyng::generate_invoke("log.msg.info", "send req.transfer.push.data(seq: ", cyng::invoke("ipt.seq.push"), ", channel: ", channel_, ")") });

			++block_;


			//
			//	continue
			//
			return true;
		}

		return false;
	}

	bool transfer_data::is_complete() const
	{
		return block_ < chuncks_.size();
	}


	std::vector<cyng::buffer_t> make_chuncks(cyng::buffer_t const& data, std::uint16_t packet_size)
	{
		//	determine number of sub-vectors
		auto const size = ((data.size() - 1) / packet_size) + 1u;
		std::vector<cyng::buffer_t> chunks;
		chunks.reserve(size);

		for (auto idx = 0u; idx < size; ++idx) {

			//
			//	prepare buffer
			//
			cyng::buffer_t buffer;
			buffer.reserve(packet_size);

			//
			//	calculate position
			//
			auto begin = std::next(data.cbegin(), idx * packet_size);
			auto end = (idx * packet_size + packet_size > data.size())
				? data.cend()
				: std::next(data.cbegin(), idx * packet_size + packet_size);

			//
			//	copy elements from the input range to the sub-vector
			//
			std::copy(begin, end, std::back_inserter(buffer));
			chunks.push_back(buffer);
		}
		return chunks;
	}

	std::size_t get_total_size(std::vector<cyng::buffer_t> const& v)
	{
		return std::accumulate(v.begin(), v.end(), 0ull, [](std::size_t a, cyng::buffer_t const& b) {
			return a + b.size();
			});
	}

}
