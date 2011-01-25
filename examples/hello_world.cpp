#include "flow.h"
#include "samples/math.h"
#include "samples/generic.h"

#include <iostream>
#include <string>

using namespace std;

string hello()
{
	return "Hello";
}

string space()
{
	return ", ";
}

string world()
{
	return "world!";
}

int main()
{
	// Create a timer that will fire every three seconds.
	flow::monotonous_timer mt(boost::posix_time::seconds(3));

	// Instantiate a graph. It starts out empty.
	flow::graph g;

	// Include in the graph three generators, one with each function defined above.
	g.add(unique_ptr<flow::producer>(new flow::samples::generic::generator<string>(mt, hello, "g1")));
	g.add(unique_ptr<flow::producer>(new flow::samples::generic::generator<string>(mt, space, "g2")));
	g.add(unique_ptr<flow::producer>(new flow::samples::generic::generator<string>(mt, world, "g3")));
	
	// Include an adder with three inputs.
	g.add(unique_ptr<flow::transformer>(new flow::samples::math::adder<string>(3, "a1")));

	// Include a consumer that just prints the data packets to std::cout.
	g.add(unique_ptr<flow::consumer>(new flow::samples::generic::ostreamer<string>(cout, "o1")));

	// Connect all three generators to the adder.
	g.connect("g1", 0, "a1", 0);
	g.connect("g2", 0, "a1", 1);
	g.connect("g3", 0, "a1", 2);

	// Connect the adder to the ostreamer.
	g.connect("a1", 0, "o1", 0);

	// Start the timer on its own thread so it doesn't block us here.
	boost::thread mt_t(boost::ref(mt));

	// Start the graph! Now we should see "Hello, world!" printed to the standard output every three seconds.
	g.start();

	// Wait for some input. And when we get it...
	char c;
	cin >> c;

	// ... pause the graph.
	g.pause();

	// Wait some more...
	cin >> c;

	// ...and re-start the graph.
	g.start();

	// Wait some more...
	cin >> c;

	// ...and stop the graph completely.
	g.stop();

	// Stop the timer.
	mt.stop();
	mt_t.join();

	return 0;
}
