#include "counted.h"
#include "dummies.h"
#include "manual.h"

#include "flow.h"
#include "samples/generic.h"

#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <thread>

using namespace std;

typedef map<string, string> args_t;

args_t make_args(const char* types[], char* values[], int c)
{
	args_t args;

	while(c)
	{
		--c;

		args[types[c]] = values[c];
	}

	return args;
}

bool empty(args_t args)
{
	{
		flow::graph g;

		if(args["start"] == "start") g.start();
	}

	return true;
}

bool unconnected(args_t args)
{
	{
		flow::graph g;

		g.add(make_shared<dummy_producer<int>>());
		g.add(make_shared<dummy_transformer<int, int>>());
		g.add(make_shared<dummy_consumer<int>>());

		if(args["start"] == "start") g.start();
	}

	return true;
}

bool connected(args_t args)
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

		if(args["start"] == "start") g.start();
	}

	return true;
}

bool count(args_t args)
{
	size_t n = stoul(args["count"]);

	auto sp_pn = make_shared<produce_n<int>>(n);
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

		this_thread::sleep_for(chrono::milliseconds(100));
	}

	return sp_tc->count(0) == n && sp_cc->count(0) == n;
}

bool restart(args_t args)
{
	{
		auto sp_pn = make_shared<produce_n<int>>(3);
		auto sp_cc = make_shared<consumption_counter<int>>();
	
		flow::graph g;
	
		g.add(sp_pn);
		g.add(sp_cc);

		g.connect<int>(sp_pn, 0, sp_cc, 0);

		size_t c = stoul(args["count"]);
		++c;
		while(c)
		{
			--c;

			g.start();

			this_thread::sleep_for(chrono::milliseconds(100));

			if(args["halt"] == "pause")
			{
				g.pause();
			}
			else
			{
				g.stop();
			}

			if(sp_cc->count(0) != 3)
			{
				return false;
			}

			sp_pn->reset();
			sp_cc->reset();
		}
	}

	return true;
}

bool tee(args_t args)
{
	{
		auto sp_pu = make_shared<pusher<int>>();
		auto sp_t= make_shared<flow::samples::generic::tee<int>>();
		auto sp_po1 = make_shared<popper<int>>();
		auto sp_po2 = make_shared<popper<int>>();

		flow::graph g;

		g.add(sp_pu, "pusher_1");
		g.add(sp_t);
		g.add(sp_po1, "popper_1");
		g.add(sp_po2, "popper_2");

		g.connect<int>(sp_pu, 0, sp_t, 0);
		g.connect<int>(sp_t, 0, sp_po1, 0);
		g.connect<int>(sp_t, 1, sp_po2, 0);

		g.start();

		size_t c = stoul(args["count"]);
		int n = 11;
		while(c)
		{
			--c;

			sp_pu->push(n);
			
			if(sp_po1->pop()->data() != n || sp_po2->pop()->data() != n)
			{
				return false;
			}

			n += n;
		}
	}

	return true;
}

bool add_delay(args_t args)
{
	{
		auto sp_pu = make_shared<pusher<int>>();
		auto sp_d= make_shared<flow::samples::generic::delay<int>>(chrono::seconds(1));
		auto sp_po = make_shared<popper<int>>();
	
		flow::graph g;
	
		g.add(sp_pu, "pusher");
		g.add(sp_d);
		g.add(sp_po, "popper");

		g.connect<int>(sp_pu, 0, sp_d, 0);
		g.connect<int>(sp_d, 0, sp_po, 0);

		g.start();

		// Test with a prior set consumption time.
		auto before = chrono::high_resolution_clock::now();
		sp_pu->push(11, before);

		auto after = sp_po->pop()->consumption_time();
		if(after - before < chrono::seconds(1))
		{
			return false;
		}

		// Test with no prior set consumption time.
		before = chrono::high_resolution_clock::now();
		sp_pu->push(11);

		after = sp_po->pop()->consumption_time();
		if(after - before < chrono::seconds(1))
		{
			return false;
		}
	}

	return true;
}

int main(int argc, char* argv[])
{
	bool b = false;

	if(strcmp(argv[1], "empty") == 0)
	{
		const char* types[] = { "start" };
		b = empty(make_args(types, &argv[2], argc - 2));
	}
	else if(strcmp(argv[1], "unconnected") == 0)
	{
		const char* types[] = { "start" };
		b = unconnected(make_args(types, &argv[2], argc - 2));
	}
	else if(strcmp(argv[1], "connected") == 0)
	{
		const char* types[] = { "start" };
		b = connected(make_args(types, &argv[2], argc - 2));
	}
	else if(strcmp(argv[1], "count") == 0)
	{
		const char* types[] = { "count" };
		b = count(make_args(types, &argv[2], argc - 2));
	}
	else if(strcmp(argv[1], "restart") == 0)
	{
		const char* types[] = { "halt", "count" };
		b = restart(make_args(types, &argv[2], argc - 2));
	}
	else if(strcmp(argv[1], "tee") == 0)
	{
		const char* types[] = { "count" };
		b = tee(make_args(types, &argv[2], argc - 2));
	}
	else if(strcmp(argv[1], "add_delay") == 0)
	{
		const char* types[] = { "" };
		b = add_delay(make_args(types, &argv[2], argc - 2));
	}

	return b ? 0 : 1;
}
