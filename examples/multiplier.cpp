#include "flow.h"
#include "samples/generic.h"

#include <boost/thread.hpp>

#include <ctime>
#include <functional>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

using namespace std;

// This class takes its inputs (in terms of T), multiplies them, then outputs the multiplication expression including the product as a string.
// For example, given the inputs of 3 and 4, it outputs the string "3 * 4 = 12".
template<typename T>
class multiplication_expressifier : public flow::transformer
{
public:
	multiplication_expressifier(size_t ins = 2, const string& name_r = "multiplication_expressifier") : node(name_r), transformer(name_r, ins, 1) {}

	virtual ~multiplication_expressifier() {}

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
			vector<unique_ptr<flow::packet>> terms;

			for(size_t i = 0; i != ins(); ++i)
			{
				terms.emplace_back(move(input(i).pop()));
			}

			// Start the product as equal to the first term.
			T product(*reinterpret_cast<T*>(&terms[0]->data()[0]));

			// Multiply by the value of all other packets.
			for_each(terms.begin() + 1, terms.end(), [&product](const unique_ptr<flow::packet>& packet_up_r){
				product *= *reinterpret_cast<T*>(&packet_up_r->data()[0]);
			});

			// Using a stringstream, aggregate all the factors and the product to form the multiplication expression.
			stringstream ss;
			ss << *reinterpret_cast<T*>(&(terms[0]->data()[0]));
			for_each(terms.begin() + 1, terms.end(), [&ss](const unique_ptr<flow::packet>& packet_up_r){
				ss << " * " << *reinterpret_cast<T*>(&packet_up_r->data()[0]);
			});
			ss << " = " << product;

			// This now looks like "a * b [* x] = p".
			string expression = ss.str();

			// Make a packet with the expression.
			vector<unsigned char> data(sizeof(string));
			new(&data[0]) string;
			*reinterpret_cast<string*>(&data[0]) = expression;

			unique_ptr<flow::packet> p(new flow::packet(move(data)));

			// Output it.
			output(0).push(move(p));
		}
	}
};

int main()
{
	// Create a timer that will fire every three seconds.
	flow::monotonous_timer mt(boost::posix_time::seconds(3));

	// Instantiate a graph. It starts out empty.
	flow::graph g;

	// Instantiate a random number generator with a uniform distribution of the number 0 to 10.
	random_device rd;
	default_random_engine engine(rd());
	uniform_int_distribution<size_t> uniform(0, 10);
	//function<int ()> generator = bind(uniform, ref(engine));
	auto generator = [&engine, &uniform](){ return uniform(engine); };

	// Create two generators.
	g.add(make_shared<flow::samples::generic::generator<int>>(mt, generator, "g1"));
	g.add(make_shared<flow::samples::generic::generator<int>>(mt, generator, "g2"));
	
	// Include a multiplication_expressifier with two inputs.
	// We specify its inputs to be ints, but its output will always be a string.
	g.add(make_shared<multiplication_expressifier<int>>(2, "me1"));

	// Include a consumer that just prints the data packets to cout.
	g.add(make_shared<flow::samples::generic::ostreamer<string>>(cout, "o1"));

	// Connect the two generators to the multiplication_expressifier.
	g.connect("g1", 0, "me1", 0);
	g.connect("g2", 0, "me1", 1);

	// Connect the multiplication_expressifier to the ostreamer.
	g.connect("me1", 0, "o1", 0);

	// Start the timer on its own thread so it doesn't block us here.
	boost::thread mt_t(boost::ref(mt));

	// Start the graph! Now we should see multiplication expressions printed to the standard output.
	g.start();

	// Wait for some input. And when we get it...
	char c;
	cin >> c;

	// ...stop the graph completely.
	g.stop();

	// Stop the timer.
	mt.stop();
	mt_t.join();

	return 0;
}
