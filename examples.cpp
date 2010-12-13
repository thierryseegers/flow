#include "flow.h"
#include "graph.h"
#include "samples/math.h"
#include "samples/generic.h"
#include "timer.h"

#include <boost/thread.hpp>
#include <lwsync/critical_resource.hpp>

#include <iostream>
#include <string>

using namespace std;

string hello()
{
	return "hello";
}

string space()
{
	return " ";
}

string world()
{
	return "world";
}

int main()
{
	flow::monotonous_timer mt(boost::posix_time::seconds(3));

	typedef decltype(hello()) data_t;
/*
	flow::samples::generic::generator<data_t> g1(mt, hello), g2(mt, space), g3(mt, world);
	flow::samples::math::adder<data_t> a1(3);
	flow::samples::generic::delay d(boost::posix_time::seconds(10));
	flow::samples::generic::ostreamer<data_t> o(cout);

	g1.output(0).connect(a1.input(0));
	g2.output(0).connect(a1.input(1));
	g3.output(0).connect(a1.input(2));

	a1.output(0).connect(d.input(0));
	d.output(0).connect(o.input(0));

	boost::thread mt_t(boost::ref(mt)),
				  o_t(boost::ref(o)),
				  d_t(boost::ref(d)),
				  a1_t(boost::ref(a1)),
				  g1_t(boost::ref(g1)),
				  g2_t(boost::ref(g2)),
				  g3_t(boost::ref(g3));

	char c;
	cin >> c;

	g3.stop();
	g2.stop();
	g1.stop();
	a1.stop();
	d.stop();
	o.stop();

	g3_t.join();
	g2_t.join();
	g1_t.join();
	a1_t.join();
	d_t.join();
	o_t.join();
*/
	flow::graph g;

	g.add(unique_ptr<flow::producer>(new flow::samples::generic::generator<data_t>(mt, hello, "g1")));
	g.add(unique_ptr<flow::producer>(new flow::samples::generic::generator<data_t>(mt, space, "g2")));
	g.add(unique_ptr<flow::producer>(new flow::samples::generic::generator<data_t>(mt, world, "g3")));
	
	g.add(unique_ptr<flow::transformer>(new flow::samples::math::adder<data_t>(3, "a1")));

//	g.add(unique_ptr<flow::transformer>(new flow::samples::generic::delay(boost::posix_time::seconds(10), "d1")));

	g.add(unique_ptr<flow::consumer>(new flow::samples::generic::ostreamer<data_t>(cout, "o1")));


	g.connect("g1", 0, "a1", 0);
	g.connect("g2", 0, "a1", 1);
	g.connect("g3", 0, "a1", 2);

//	g.connect("a1", 0, "d1", 0);
//	g.connect("d1", 0, "o1", 0);
	g.connect("a1", 0, "o1", 0);

	boost::thread mt_t(boost::ref(mt));
	g.start();

	char c;
	cin >> c;

	g.pause();

	cin >> c;

	g.start();

	cin >> c;

	g.stop();

	mt.stop();
//	mt_t.join();

	return 0;
}