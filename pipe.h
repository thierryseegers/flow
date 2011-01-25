#if !defined(FLOW_PIPE_H)
	 #define FLOW_PIPE_H

#include "named.h"
#include "packet.h"

#include <memory>
#include <queue>
#include <string>
#include <utility>

//!\file pipe.h
//!
//!\brief Defines the \ref flow::pipe class.

namespace flow
{

class inpin;
class outpin;

//!\brief Carries packets from node to node on a FIFO basis.
//!
//! Packets will accumulate in pipes if the node at the consuming end does not consume them fast enough.
//! If packet accumulation is expected but memory usage is a concern, length and weight can be specified.
//! Packet accumulation should be intermittent rather than constant or memory usage will grow at the rate of accumulation.
//! A graph that produces more data than it consumes is unbalanced and should be modified.
class pipe : public named
{
	std::deque<std::unique_ptr<packet> > d_packets;

	outpin *d_input_p;
	inpin *d_output_p;

	size_t d_max_length;
	size_t d_max_weight;

	size_t d_weight;

public:
	//!\brief Constructor for a new pipe.
	//!
	//!\param name_r Name of this pipe. This will be typically generated from the names of the producing and consuming nodes.
	//!\param output_p The output pin of the producing node.
	//!\param input_p The input pin of the consuming node.
	//!\param max_length The maximum number of nodes this pipe will carry.
	//!\param max_weight The maximum number of bytes this pipe will carry.
	pipe(const std::string& name_r, outpin *output_p, inpin *input_p, const size_t max_length = 0, const size_t max_weight = 0)
		: named(name_r), d_input_p(output_p), d_output_p(input_p), d_max_length(max_length), d_max_weight(max_weight), d_weight(0)
	{}

	//!\brief Move constructor.
	pipe(pipe&& pipe_rr) : named(pipe_rr), d_packets(std::move(pipe_rr.d_packets)), d_input_p(std::move(pipe_rr.d_input_p)), d_output_p(std::move(pipe_rr.d_output_p)),
		d_max_length(std::move(pipe_rr.d_max_length)), d_max_weight(std::move(pipe_rr.d_max_weight)), d_weight(std::move(pipe_rr.d_weight))
	{}

	virtual ~pipe() {}

	//!\brief Pointer to the producing node's output pin.
	virtual outpin* input() const
	{
		return d_input_p;
	}

	//!\brief Pointer to the consuming node's input pin.
	virtual inpin* output() const
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
	virtual bool push(std::unique_ptr<packet> packet_p)
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
	//!\return A pointer to the next packet in the pipe. nullptr if there is no packet.
	virtual std::unique_ptr<packet> pop()
	{
		if(!d_packets.size()) return nullptr;

		std::unique_ptr<packet> packet_p(std::move(d_packets.front()));
		
		d_packets.pop_front();
		d_weight -= packet_p->size();

		return packet_p;
	}
};


}

#endif
