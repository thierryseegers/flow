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
class adder : public transformer<T, T>
{
public:
	//!\brief Default constructor with two inputs.
	//!
	//!\param ins The number of inputs.
	//!\param name_r The name to give this node.
	adder(size_t ins = 2, const std::string& name_r = "adder") : node(name_r), transformer<T, T>(name_r, ins, 1) {}

	virtual ~adder() {}

	//!\brief Implementation of consumer::ready().
	//!
	//! When at least one packet is ready at each inputs, one packet from each input is popped.
	//! They are then all summed and the sum is moved to the output.
	virtual void ready(size_t n)
	{
		// Check that a packet is ready at all inputs. If not, return and try again when another packet arrives.
		for(size_t i = 0; i != consumer<T>::ins(); ++i)
		{
			if(!consumer<T>::input(i).peek())
			{
				return;
			}
		}

		// Gather the terms in a container.
		std::vector<std::unique_ptr<packet<T>>> terms;

		for(auto& inpin : consumer<T>::inputs())
		{
			terms.emplace_back(move(inpin.pop()));
		}

		// Start the sum as equal to the first term.
		T sum(terms[0]->data());

		// Add to the sum the value of all other packets.
		std::for_each(terms.begin() + 1, terms.end(), [&sum](const std::unique_ptr<packet<T>>& packet_up_r){
			sum += packet_up_r->data();
		});

		// Make a packet with the sum data.
		std::unique_ptr<packet<T>> sum_up(new packet<T>(sum));

		// Output it.
		producer<T>::output(0).push(sum_up);
	}
};

//!\brief Concrete transformer that uses operator+= to add a constant value to input packets.
template<typename T>
class const_adder : public transformer<T, T>
{
	const T d_addend;

public:
	//!\param addend The constan value to add to input packets.
	//!\param name_r The name to give to this node.
	const_adder(const T& addend, const std::string& name_r = "const_adder") : node(name_r), transformer<T, T>(name_r, 1, 1), d_addend(addend) {}

	virtual ~const_adder() {}

	//!\brief Implementation of consumer::ready().
	virtual void ready(size_t)
	{
		std::unique_ptr<packet<T>> packet_p = consumer<T>::input(0).pop();

		packet_p->data() += d_addend;

		producer<T>::output(0).push(packet_p);
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
