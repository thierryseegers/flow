#if !defined(FLOW_GENERIC_H)
	 #define FLOW_GENERIC_H

#include "node.h"
#include "timer.h"

#include <boost/date_time.hpp>
#include <boost/thread.hpp>
#include <lwsync/monitor.hpp>

#include <functional>
#include <iostream>

//!\file generic.h
//!
//!\brief Defines sample concrete node classes that perform generic tasks.

//!\namespace flow::samples
//!
//!\brief Collection of concrete nodes.

//!\namespace flow::samples::generic
//!
//!\brief Concrete nodes that perform generic tasks.
namespace flow { namespace samples { namespace generic {

//!\brief Concrete producer that generates packets by calling a parameter functor.
template<typename T>
class generator : public producer
{
	std::function<T ()> d_gen_f;

	lwsync::monitor<bool> d_awaken_m;

public:
	//!\param timer_r Timer that will signal when to produce a packet.
	//!\param gen_f The functor to be called. The return value of this functor will be considered a packet.
	//!\param name_r The name to give this node.
	generator(timer& timer_r, const std::function<T ()>& gen_f, const std::string& name_r = "generator") : node(name_r), producer(name_r, 1), d_gen_f(gen_f), d_awaken_m(false)
	{
		timer_r.listen(std::bind(&generator::timer_fired, this));
	}

	virtual ~generator() {}

	//!\brief implementation of node::stop().
	virtual void stop()
	{
		producer::stop();

		*d_awaken_m.access() = true;
	}

	//!\brief The function to pass to the timer as the listener.
	virtual void timer_fired()
	{
		*d_awaken_m.access() = true;
	}

	//!\brief Implementation of producer::produce().
	//!
	//! This function waits until stop has been requested or the timer has fired.
	//! When the timer fires, the generator functor is called and its return value is moved to the pipe.
	virtual void produce()
	{
		*d_awaken_m.wait() = false;

		if(*d_state_m.const_access() != stop_requested)
		{
			std::vector<unsigned char> data(sizeof(T));
			*reinterpret_cast<T*>(&data[0]) = d_gen_f();

			std::unique_ptr<packet> packet_p(new packet(std::move(data)));

			output(0).push(std::move(packet_p));
		}
	}
};

//!\brief Concrete consumer that outputs packets to a parameter std::ostream.
template<typename T>
class ostreamer : public consumer
{
	std::ostream& d_o_r;

	boost::thread d_sleep_t;

	lwsync::monitor<bool> d_awaken_m;

public:
	//! This consumer has only one input.
	//!
	//!\param o_r Reference to the output stream.
	//!\param name_r The name to give this node.
	ostreamer(std::ostream& o_r, const std::string& name_r = "ostreamer") : node(name_r), consumer(name_r, 1), d_o_r(o_r), d_awaken_m(false) {}

	virtual ~ostreamer() {}

	//!\brief Implementation of node::stop().
	virtual void stop()
	{
		consumer::stop();

		*d_awaken_m.access() = true;
	}

	//!\brief Implmentation of producing::ready()
	//!
	//! If stop() has been called, it returns immediately.
	//! If a packet has no consumption time specified, the packet is streamed immediately.
	//! If a packet has a consumption time specified, and that time is:
	//! - in the future: this function sleeps until that time to stream out, unless stop has been called since, then it exits.
	//! - in the past: the packet is unused and lost.
	virtual void ready(size_t)
	{
		std::unique_ptr<packet> packet_p;

		while((packet_p = input(0).pop()) && (*d_state_m.const_access() != stop_requested))
		{
			using namespace boost::posix_time;

			if(packet_p->consumption_time().is_not_a_date_time())
			{
				// This packet has no set consumption time. Consume it immediately.
				d_o_r << *reinterpret_cast<T*>(&packet_p->data()[0]) << std::endl;
			}
			else if(packet_p->consumption_time() > microsec_clock::universal_time())
			{
				// This packet must be consumed at a set time.
				// Create a thread that sleeps the required delay and rings the alarm.
				d_sleep_t = boost::thread(std::mem_fun(&ostreamer::sleep), this, packet_p->consumption_time());

				// Wait until the sleep thread is done or stop was requested.
				*d_awaken_m.wait() = false;

				if(*d_state_m.const_access() != stop_requested)
				{
					d_o_r << *reinterpret_cast<T*>(&packet_p->data()[0]) << std::endl;
				}
			}
		}
	}

private:
	virtual void sleep(const boost::posix_time::ptime& time_r)
	{
		boost::thread::sleep(time_r);

		*d_awaken_m.access() = true;
	}
};

//!\brief Concrete transformer that clones one input packet to multiple output packets.
class tee : public transformer
{
public:
	//! The number of ouput pins specified is the number of clones this node will output.
	//!
	//!\param outs Number of output pins.
	//!\param name_r The name to give this node.
	tee(const size_t outs = 2, const std::string& name_r = "tee") : node(name_r), transformer(name_r, 1, outs) {}

	virtual ~tee() {}

	//!\brief Implmentation of producing::ready().
	//!
	//! The original input packet is moved to the first output pipe.
	//! It is also copied into the rest of the ouput pipes.
	virtual void ready(size_t)
	{
		std::unique_ptr<packet> packet_p;
			
		while(packet_p = input(0).pop())
		{
			for(size_t i = 1; i != outs(); ++i)
			{
				std::unique_ptr<packet> copy_p(new packet(*packet_p));
				output(i).push(std::move(copy_p));
			}

			output(0).push(std::move(packet_p));
		}
	}
};

//!\brief Concrete transformer that adds a delay to a packet consumption time.
class delay : public transformer
{
	boost::posix_time::time_duration d_offset;

public:
	//!\param offset_r The delay to add to the packets' consumption time.
	//!\param name_r The name to give this node.
	delay(const boost::posix_time::time_duration& offset_r, const std::string& name_r = "delay") : node(name_r), transformer(name_r, 1, 1), d_offset(offset_r) {}

	virtual ~delay() {}

	//!\brief Implementation of producing::ready().
	//!
	//! If a packet has no set consumption time, then its consumption is set to the time at which this node has received the packet plus the given delay.
	virtual void ready(size_t)
	{
		std::unique_ptr<packet> packet_p;

		while(packet_p = input(0).pop())
		{
			if(packet_p->consumption_time().is_not_a_date_time())
			{
				packet_p->consumption_time() = boost::posix_time::microsec_clock::universal_time() + d_offset;
			}
			else
			{
				packet_p->consumption_time() += d_offset;
			}

			output(0).push(std::move(packet_p));
		}
	}
};

}}}

#endif
