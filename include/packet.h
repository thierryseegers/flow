#if !defined(FLOW_PACKET_H)
	 #define FLOW_PACKET_H

#include <chrono>
#include <vector>

//!\file packet.h
//!
//!\brief Defines the flow::packet class.

namespace flow
{

//!\brief Object that carries data from node to node through a pipe.
//!
//! Associated with a packet is an optional time of consumption.
//! A consumer node ought to wait before consuming the data in this packet if it arrives to the node early.
//! If the packet arrives too late, the consumer node ought to discard it.
template<typename T>
class packet
{
public:
	//!\brief Convenience typedef for time_point type used.
	typedef std::chrono::time_point<std::chrono::high_resolution_clock> time_point_type;

private:
	T d_data;

	time_point_type d_consumption_time;

public:
	//!\brief Constructor.
	//!
	//!\param data Data to be put in the packet.
	//!\param consumption_time The time at which a consumer node should consume the data. Optional.
	packet(T data, const time_point_type& consumption_time = time_point_type())
		: d_data(std::move(data)), d_consumption_time(consumption_time) {}

	virtual ~packet() {}

	//!\brief Returns the number of bytes in this packet.
	static size_t size() { return sizeof(T); }

	//!\brief Reference to the data this packet is carrying.
	virtual T& data() { return d_data; }

	//!\brief Reference to the data this packet is carrying.
	virtual const T& data() const { return d_data; }

	//!\brief Reference to the time of consumption.
	virtual time_point_type& consumption_time() { return d_consumption_time; }

    //!\brief Reference to the time of consumption.
    virtual const time_point_type& consumption_time() const { return d_consumption_time; }
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

