#if !defined(FLOW_GENERIC_H)
	 #define FLOW_GENERIC_H

#include "node.h"
#include "timer.h"

#include <lwsync/monitor.hpp>

#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>

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
class generator : public producer<T>
{
	std::function<T ()> d_gen_f;

	lwsync::monitor<bool> d_awaken_m;

public:
	//!\param timer_r Timer that will signal when to produce a packet.
	//!\param gen_f The functor to be called. The return value of this functor will be considered a packet.
	//!\param name_r The name to give this node.
	generator(timer& timer_r, const std::function<T ()>& gen_f, const std::string& name_r = "generator") : node(name_r), producer<T>(name_r, 1), d_gen_f(gen_f), d_awaken_m(false)
	{
		timer_r.listen(std::bind(&generator::timer_fired, this));
	}

	virtual ~generator() {}

	//!\brief implementation of node::stop().
	virtual void stop()
	{
		producer<T>::stop();

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

		if(node::state() == started)
		{
			std::unique_ptr<packet<T>> packet_p(new packet<T>(d_gen_f()));

			producer<T>::output(0).push(std::move(packet_p));
		}
	}
};

//!\brief Concrete consumer that outputs packets to a parameter std::ostream.
template<typename T>
class ostreamer : public consumer<T>
{
	std::ostream& d_o_r;

	std::condition_variable d_stopped_cv;
	std::mutex d_stopped_m;

public:
	//! This consumer has only one input.
	//!
	//!\param o_r Reference to the output stream.
	//!\param name_r The name to give this node.
	ostreamer(std::ostream& o_r, const std::string& name_r = "ostreamer") : node(name_r), consumer<T>(name_r, 1), d_o_r(o_r) {}

	virtual ~ostreamer() {}

	//!\brief Implementation of node::stop().
	virtual void stop()
	{
		consumer<T>::stop();

		d_stopped_cv.notify_one();
	}

	//!\brief Implementation of consumer::ready()
	//!
	//! If stop() has been called, it returns immediately.
	//! If a packet has no consumption time specified, the packet is streamed immediately.
	//! If a packet has a consumption time specified, and that time is:
	//! - in the future: this function sleeps until that time to stream out, unless stop has been called since, then it exits.
	//! - in the past: the packet is unused and lost.
	virtual void ready(size_t)
	{
		std::unique_ptr<packet<T>> packet_p;

		while((packet_p = consumer<T>::input(0).pop()) && (node::state() == started))
		{
			if(packet_p->consumption_time() == typename packet<T>::time_point_type())
			{
				// This packet has no set consumption time. Consume it immediately.
				d_o_r << packet_p->data() << std::endl;
			}
			else if(packet_p->consumption_time() > std::chrono::high_resolution_clock::now())
			{
				// This packet must be consumed at a set time.
				// Wait until then or until this node is stopped.
				std::unique_lock<std::mutex> l_stopped(d_stopped_m);
				d_stopped_cv.wait_until(l_stopped, packet_p->consumption_time());

				if(node::state() == started)
				{
					d_o_r << packet_p->data() << std::endl;
				}
			}
		}
	}
};

//!\brief Concrete transformer that clones one input packet to multiple output packets.
template<typename T>
class tee : public transformer<T, T>
{
public:
	//! The number of ouput pins specified is the number of clones this node will output.
	//!
	//!\param outs Number of output pins.
	//!\param name_r The name to give this node.
	tee(const size_t outs = 2, const std::string& name_r = "tee") : node(name_r), transformer<T, T>(name_r, 1, outs) {}

	virtual ~tee() {}

	//!\brief Implementation of consumer::ready().
	//!
	//! The original input packet is moved to the first output pipe.
	//! It is also copied into the rest of the ouput pipes.
	virtual void ready(size_t)
	{
		std::unique_ptr<packet<T>> packet_p;
			
		while(packet_p = consumer<T>::input(0).pop())
		{
			for(auto& out : producer<T>::outs())
			{
				std::unique_ptr<packet<T>> copy_p(new packet<T>(*packet_p));
				out.push(std::move(copy_p));
			}

			producer<T>::output(0).push(std::move(packet_p));
		}
	}
};

//!\brief Concrete transformer that adds a delay to a packet's consumption time.
template<typename T>
class delay : public transformer<T, T>
{
	std::chrono::milliseconds d_offset;

public:
	//!\param offset_r The delay to add to the packets' consumption time.
	//!\param name_r The name to give this node.
	template<typename Duration>
	delay(const Duration& offset_r, const std::string& name_r = "delay") : node(name_r), transformer<T, T>(name_r, 1, 1), d_offset(offset_r) {}

	virtual ~delay() {}

	//!\brief Implementation of consumer::ready().
	//!
	//! If a packet has no set consumption time, then its consumption is set to the time at which this node has received the packet plus the given delay.
	virtual void ready(size_t)
	{
		std::unique_ptr<packet<T>> packet_p;

		while(packet_p = consumer<T>::input(0).pop())
		{
			if(packet_p->consumption_time() == packet<T>::time_point_type())
			{
				packet_p->consumption_time() = std::chrono::high_resolution_clock::now() + d_offset;
			}
			else
			{
				packet_p->consumption_time() += d_offset;
			}

			producer<T>::output(0).push(std::move(packet_p));
		}
	}
};

}}}

#endif

/*
	(C) Copyright Thierry Seegers 2010-2012. Distributed under the following license:

	Boost Software License - Version 1.0 - August 17th, 2003

	Permission is hereby granted, free of charge, to any person or organization
	obtaining a copy of the software and accompanying documentation covered by
	this license (the "Software") to use, reproduce, display, distribute,
	execute, and transmit the Software, and to prepare derivative works of the
	Software, and to permit third-parties to whom the Software is furnished to
	do so, all subject to the following:

	The copyright notices in the Software and this entire statement, including
	the above license grant, this restriction and the following disclaimer,
	must be included in all copies of the Software, in whole or in part, and
	all derivative works of the Software, unless such copies or derivative
	works are solely in the form of machine-executable object code generated by
	a source language processor.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
	SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
	FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
	ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
	DEALINGS IN THE SOFTWARE.
*/
