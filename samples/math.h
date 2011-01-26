#if !defined(FLOW_MATH_H)
	 #define FLOW_MATH_H

#include "node.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

//!\file math.h
//!
//!\brief Defines sample concrete node classes that perform mathematical operations on their inputs.

//!\namespace flow::samples::math
//!
//!\brief Concrete nodes that perform mathematical operations on their inputs.
namespace flow { namespace samples { namespace math {

//!\brief Concrete transformer that uses operator+= to sum input packets.
template<typename T>
class adder : public transformer
{
public:
	//!\brief Default constructor with two inputs.
	//!
	//!\param ins The number of inputs.
	//!\param name_r The name to give this node.
	adder(size_t ins = 2, const std::string& name_r = "adder") : node(name_r), transformer(name_r, ins, 1) {}

	virtual ~adder() {}

	//!\brief Implementation of consumer::ready().
	//!
	//! When at least one packet is ready at each inputs, one packet from each input is popped.
	//! They are then all summed and the sum is moved to the output.
	virtual void ready(size_t n)
	{
		// Assume a packet is ready at all inputs.
		bool all = true;

		// Confirm.
		for(size_t i = 0; i != ins() && all; ++i)
		{
			if(!input(i).peek())
			{
				all = false;
			}
		}

		// If it happens to be the case.
		if(all)
		{
			// Gather the terms in a container.
			std::vector<std::unique_ptr<packet> > terms(ins());

			for(size_t i = 0; i != ins(); ++i)
			{
				terms[i] = input(i).pop();
			}

			// Start the sum as equal to the first term.
			T sum(*reinterpret_cast<T*>(&terms[0]->data()[0]));

			// Add to the sum the value of all other packets.
			std::for_each(terms.begin() + 1, terms.end(), [&sum](const std::unique_ptr<packet>& packet_up_r){
				sum += *reinterpret_cast<T*>(&packet_up_r->data()[0]);
			});

			// Make a packet with the sum data.
			std::vector<unsigned char> data(sizeof sum);
			*reinterpret_cast<T*>(&data[0]) = sum;

			std::unique_ptr<packet> sum_up(new packet(std::move(data)));

			// Output it.
			output(0).push(std::move(sum_up));
		}
	}
};

//!\brief Concrete transformer that uses operator+= to add a constant value to input packets.
template<typename T>
class const_adder : public transformer
{
	const T d_addend;

public:
	//!\param addend The constan value to add to input packets.
	//!\param name_r The name to give to this node.
	const_adder(const T& addend, const std::string& name_r = "const_adder") : node(name_r), transformer(name_r, 1, 1), d_addend(addend) {}

	virtual ~const_adder() {}

	//!\brief Implementation of consumer::ready().
	virtual void ready()
	{
		std::unique_ptr<packet> packet_p = input(0).pop();

		T *i_p = (T*)&packet_p->data()[0];
		*i_p += d_addend;

		output(0).push(std::move(packet_p));
	}
};

}}}

#endif
