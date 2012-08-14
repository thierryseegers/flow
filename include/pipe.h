#if !defined(FLOW_PIPE_H)
	 #define FLOW_PIPE_H

#include "named.h"
#include "packet.h"

#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <utility>

//!\file pipe.h
//!
//!\brief Defines the \ref flow::pipe class.

namespace flow
{

template<typename T>
class inpin;

template<typename T>
class outpin;

//!\brief Carries packets from node to node on a FIFO basis.
//!
//! Packets will accumulate in pipes if the node at the consuming end does not consume them fast enough.
//! If packet accumulation is expected but memory usage is a concern, length and weight can be specified.
//! Packet accumulation should be intermittent rather than constant or memory usage will grow at the rate of accumulation.
//! A graph that produces more data than it consumes is unbalanced and should be modified.
template<typename T>
class pipe : public named
{
	std::deque<std::unique_ptr<packet<T>>> d_packets;

	outpin<T> *d_input_p;
	inpin<T> *d_output_p;

	size_t d_max_length;
	size_t d_max_weight;

	size_t d_weight;

	std::mutex d_mutex;

public:
	//!\brief Constructor for a new pipe.
	//!
	//!\param name_r Name of this pipe. This will be typically generated from the names of the producing and consuming nodes.
	//!\param output_p The output pin of the producing node.
	//!\param input_p The input pin of the consuming node.
	//!\param max_length The maximum number of nodes this pipe will carry.
	//!\param max_weight The maximum number of bytes this pipe will carry.
	pipe(const std::string& name_r, outpin<T> *output_p, inpin<T> *input_p, const size_t max_length = 0, const size_t max_weight = 0)
		: named(name_r), d_input_p(output_p), d_output_p(input_p), d_max_length(max_length), d_max_weight(max_weight), d_weight(0)
	{}

	//!\brief Move constructor.
	pipe(pipe&& pipe_rr) : named(std::move(pipe_rr)), d_packets(std::move(pipe_rr.d_packets)), d_input_p(std::move(pipe_rr.d_input_p)), d_output_p(std::move(pipe_rr.d_output_p)),
		d_max_length(std::move(pipe_rr.d_max_length)), d_max_weight(std::move(pipe_rr.d_max_weight)), d_weight(std::move(pipe_rr.d_weight))
	{}

	virtual ~pipe() {}

	virtual std::unique_lock<std::mutex> lock()
	{
		return std::unique_lock<std::mutex>(d_mutex);
	}

	//!\brief Pointer to the producing node's output pin.
	virtual outpin<T>* input() const
	{
		return d_input_p;
	}

	//!\brief Pointer to the consuming node's input pin.
	virtual inpin<T>* output() const
	{
		return d_output_p;
	}

	//!\brief The pipe's current length. The number of bytes in the pipe.
	virtual size_t length() const
	{
		return d_packets.size();
	}

	//!\brief The maximum number of packets this pipe will carry.
	virtual size_t max_length() const
	{
		return d_max_length;
	}

	//!\brief The pipe's current weight. The sum of all bytes of all packets in the pipe.
	virtual size_t weight() const
	{
		return d_weight;
	}

	//!\brief The maximum number of bytes this pipe will carry.
	virtual size_t max_weight() const
	{
		return d_max_weight;
	}

	//!\brief Sets the maximum length.
	virtual size_t cap_length(const size_t max_length)
	{
		size_t previous_cap = d_max_length;
		d_max_length = max_length;
		return previous_cap;
	}

	//!\brief Sets the maximum weight.
	virtual size_t cap_weight(const size_t max_weight)
	{
		size_t previous_cap = d_max_weight;
		d_max_weight = max_weight;
		return previous_cap;
	}
	
	//!\brief Discards all packets.
	virtual size_t flush()
	{
		size_t s = d_packets.size();
		d_packets.clear();

		return s;
	}

	//!\brief Queue a packet in the pipe.
	//!
	//! Used by the producer node to move a packet it produced to the pipe.
	//! If the packet does not fit in the pipe, it will not be moved.
	//!
	//!\param packet_p A pointer to the packet.
	//!				   If this call is unsuccessful, packet_p will still point to the packet after the call.
	//!
	//!\return true if the packet was successfully moved to the pipe, false otherwise.
	virtual bool push(std::unique_ptr<packet<T>> packet_p)
	{
		if(d_max_length && (d_packets.size() == d_max_length)) return false;
		if(d_max_weight && (d_weight + packet_p->size() > d_max_weight)) return false;

		d_weight += packet_p->size();
		d_packets.push_back(std::move(packet_p));

		return true;
	}
	
	//!\brief Extracts a packet from the pipe.
	//!
	//! Used by the consuming node to move a packet from the pipe to consume.
	//!
	//!\return A pointer to the next packet in the pipe, empty pointer if there is no packet.
	virtual std::unique_ptr<packet<T>> pop()
	{
		if(!d_packets.size()) return std::unique_ptr<packet<T>>();

		std::unique_ptr<packet<T>> packet_p(std::move(d_packets.front()));
		
		d_packets.pop_front();
		d_weight -= packet_p->size();

		return packet_p;
	}
};

}

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

