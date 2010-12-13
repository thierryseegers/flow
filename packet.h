#if !defined(FLOW_PACKET_H)
	 #define FLOW_PACKET_H

#include <boost/date_time.hpp>

#include <vector>

namespace flow
{

//!\brief Object that carries data from node to node through a pipe.
//!
//! A packet contains data in the form of a vector of unsigned chars.
//! Associated with it is an optional time of consumption.
//! A consumer node will wait before consuming the data in this packet if it arrives to the node early.
//! If the packet arrives too late, the consumer node will discard it.
class packet
{
	std::vector<unsigned char> d_data;

	boost::posix_time::ptime d_consumption_time;

public:
	//!\brief Data will be moved, not copied. Time of consumption is optional.
	//!
	//!\param data Bytes to be moved.
	//!\param consumption_time The time at which a consumer node should consume the data.
	packet(const std::vector<unsigned char>&& data, const boost::posix_time::ptime& consumption_time = boost::posix_time::not_a_date_time)
		: d_data(std::move(data)), d_consumption_time(consumption_time) {}

	virtual ~packet() {}

	//!\brief Returns the number of bytes in this packet.
	virtual size_t size() const { return d_data.size(); }

	//!\brief Reference to the data this packet is carrying.
	virtual std::vector<unsigned char>& data() { return d_data; }

	//!\brief Reference to the time of consumption.
	virtual boost::posix_time::ptime& consumption_time() { return d_consumption_time; }
};

}

#endif
