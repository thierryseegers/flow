#include "counted.h"
#include "dummies.h"

#include "flow.h"

#include <cstring>
#include <iostream>
#include <memory>
#include <thread>

using namespace std;

bool empty_graph()
{
	{
		flow::graph g;

		g.start();
	}

	return true;
}

bool graph_dummy_nodes_unconnected()
{
	{
		flow::graph g;

		g.add(make_shared<dummy_producer<int>>());
		g.add(make_shared<dummy_transformer<int, int>>());
		g.add(make_shared<dummy_consumer<int>>());

		g.start();
	}

	return true;
}

bool graph_dummy_nodes_connected()
{
	{
		flow::graph g;

		auto sp_dp = make_shared<dummy_producer<int>>(1);
		auto sp_dt = make_shared<dummy_transformer<int, int>>(1, 1);
		auto sp_dc = make_shared<dummy_consumer<int>>(1);
		g.add(sp_dp);
		g.add(sp_dt);
		g.add(sp_dc);

		g.connect<int>(sp_dp, 0, sp_dt, 0);
		g.connect<int>(sp_dt, 0, sp_dc, 0);

		g.start();
	}

	return true;
}

bool just_one()
{
	auto sp_pn = make_shared<produce_n<int>>(1);
	auto sp_tc = make_shared<transformation_counter<int>>();
	auto sp_cc = make_shared<consumption_counter<int>>();

	{
		flow::graph g;

		g.add(sp_pn);
		g.add(sp_tc);
		g.add(sp_cc);

		g.connect<int>(sp_pn, 0, sp_tc, 0);
		g.connect<int>(sp_tc, 0, sp_cc, 0);

		g.start();

		this_thread::sleep_for(chrono::seconds(1));
	}

	return sp_tc->received[0] == 1 && sp_cc->received[0] == 1;
}

int main(int argc, char* argv[])
{
	bool b = true;

	if(strcmp(argv[1], "empty_graph") == 0)
	{
		b = empty_graph();
	}
	else if(strcmp(argv[1], "graph_dummy_nodes_unconnected") == 0)
	{
		b = graph_dummy_nodes_unconnected();
	}
	else if(strcmp(argv[1], "graph_dummy_nodes_connected") == 0)
	{
		b = graph_dummy_nodes_connected();
	}
	else if(strcmp(argv[1], "just_one") == 0)
	{
		b = just_one();
	}

	return b ? 0 : 1;
}
