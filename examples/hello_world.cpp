#include "flow.h"
#include "samples/math.h"
#include "samples/generic.h"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

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
	flow::monotonous_timer mt(chrono::seconds(3));

	// Instantiate a graph. It starts out empty.
	flow::graph g;

	// Include in the graph three generators, one with each function defined above.
	auto sp_g1 = make_shared<flow::samples::generic::generator<string>>(mt, hello, "g1");
	auto sp_g2 = make_shared<flow::samples::generic::generator<string>>(mt, space, "g2");
	auto sp_g3 = make_shared<flow::samples::generic::generator<string>>(mt, world, "g3");
	g.add(sp_g1);
	g.add(sp_g2);
	g.add(sp_g3);

	// Include an adder with three inputs.
	auto sp_a1 = make_shared<flow::samples::math::adder<string>>(3, "a1");
	g.add(sp_a1);

	// Include a consumer that just prints the data packets to std::cout.
	auto sp_o1 = make_shared<flow::samples::generic::ostreamer<string>>(cout, "o1");
	g.add(sp_o1);

	// Connect all three generators to the adder.
	g.connect<string>(sp_g1, 0, sp_a1, 0);
	g.connect<string>(sp_g2, 0, sp_a1, 1);
	g.connect<string>(sp_g3, 0, sp_a1, 2);

	// Connect the adder to the ostreamer.
	g.connect<string>(sp_a1, 0, sp_o1, 0);

	// Start the timer on its own thread so it doesn't block us here.
	thread mt_t(ref(mt));

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
