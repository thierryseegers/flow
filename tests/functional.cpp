#include "counted.h"
#include "dummies.h"
#include "manual.h"

#include "flow.h"
#include "samples/generic.h"
#include "samples/math.h"

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

bool reconnect(args_t args)
{
	size_t n = stoul(args["count"]);

	auto sp_pn = make_shared<produce_n<int>>(n);
	auto sp_tc = make_shared<flow::samples::generic::tee<int>>();
	auto sp_cc1 = make_shared<consumption_counter<int>>();
	auto sp_cc2 = make_shared<consumption_counter<int>>();

	{
		flow::graph g;

		g.add(sp_pn);
		g.add(sp_tc);
		g.add(sp_cc1, "consumption_counter_1");
		g.add(sp_cc2, "consumption_counter_2");

		// Run the graph with all nodes connected.
		g.connect<int>(sp_pn, 0, sp_tc, 0);
		g.connect<int>(sp_tc, 0, sp_cc1, 0);
		g.connect<int>(sp_tc, 1, sp_cc2, 0);

		g.start();

		this_thread::sleep_for(chrono::milliseconds(100));

		if(args["halt"] == "pause") g.pause();
		else if(args["halt"] == "stop") g.stop();

		if(sp_cc1->count(0) != n || sp_cc2->count(0) != n)
		{
			return false;
		}

		// Run the graph with only sp_cc1 disconnected.
		g.disconnect<int>(sp_cc1, 0);

		sp_pn->reset();
		sp_cc1->reset();
		sp_cc2->reset();

		if(args["halt"] != "nohalt") g.start();

		this_thread::sleep_for(chrono::milliseconds(100));

		if(args["halt"] == "pause") g.pause();
		else if(args["halt"] == "stop") g.stop();

		if(sp_cc1->count(0) != 0 || sp_cc2->count(0) != n)
		{
			return false;
		}

		// Run the graph with only sp_cc2 disconnected.
		g.disconnect<int>(sp_cc2, 0);
		g.connect<int>(sp_tc, 0, sp_cc1, 0);

		sp_pn->reset();
		sp_cc1->reset();
		sp_cc2->reset();
		
		if(args["halt"] != "nohalt") g.start();

		this_thread::sleep_for(chrono::milliseconds(100));

		if(args["halt"] == "pause") g.pause();
		else if(args["halt"] == "stop") g.stop();

		if(sp_cc1->count(0) != n || sp_cc2->count(0) != 0)
		{
			return false;
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

template<typename T>
bool add(args_t args)
{
	if(args["type"] == "int")
	{
		return add<int>(args);
	}
	else if(args["type"] == "string")
	{
		return add<string>(args);
	}

	return false;
}

template<>
bool add<int>(args_t args)
{
	{
		auto sp_pu1 = make_shared<pusher<int>>();
		auto sp_pu2 = make_shared<pusher<int>>();
		auto sp_a = make_shared<flow::samples::math::adder<int>>();
		auto sp_po = make_shared<popper<int>>();
	
		flow::graph g;
	
		g.add(sp_pu1, "pusher_1");
		g.add(sp_pu2, "pusher_2");
		g.add(sp_a);
		g.add(sp_po, "popper");

		g.connect<int>(sp_pu1, 0, sp_a, 0);
		g.connect<int>(sp_pu2, 0, sp_a, 1);
		g.connect<int>(sp_a, 0, sp_po, 0);

		g.start();

		size_t c = stoul(args["count"]);
		int n = 11;
		while(c)
		{
			--c;
			sp_pu1->push(n);
			sp_pu2->push(n);

			if(sp_po->pop()->data() != n * 2)
			{
				return false;
			}

			n += n;
		}
	}

	return true;
}

template<>
bool add<string>(args_t args)
{
	{
		auto sp_pu1 = make_shared<pusher<string>>();
		auto sp_pu2 = make_shared<pusher<string>>();
		auto sp_a = make_shared<flow::samples::math::adder<string>>();
		auto sp_po = make_shared<popper<string>>();
	
		flow::graph g;
	
		g.add(sp_pu1, "pusher_1");
		g.add(sp_pu2, "pusher_2");
		g.add(sp_a);
		g.add(sp_po, "popper");

		g.connect<string>(sp_pu1, 0, sp_a, 0);
		g.connect<string>(sp_pu2, 0, sp_a, 1);
		g.connect<string>(sp_a, 0, sp_po, 0);

		g.start();

		size_t c = stoul(args["count"]);
		string s = "ha";
		while(c)
		{
			--c;
			sp_pu1->push(s);
			sp_pu2->push(s);

			if(sp_po->pop()->data() != (s + s))
			{
				return false;
			}

			s += s;
		}
	}

	return true;
}

template<typename T>
bool const_add(args_t args)
{
	if(args["type"] == "int")
	{
		return const_add<int>(args);
	}
	else if(args["type"] == "string")
	{
		return const_add<string>(args);
	}

	return false;
}

template<>
bool const_add<int>(args_t args)
{
	{
		auto sp_pu = make_shared<pusher<int>>();
		auto sp_a = make_shared<flow::samples::math::const_adder<int>>(11);
		auto sp_po = make_shared<popper<int>>();
	
		flow::graph g;
	
		g.add(sp_pu, "pusher");
		g.add(sp_a);
		g.add(sp_po, "popper");

		g.connect<int>(sp_pu, 0, sp_a, 0);
		g.connect<int>(sp_a, 0, sp_po, 0);

		g.start();

		size_t c = stoul(args["count"]);
		int n = 11;
		while(c)
		{
			--c;

			sp_pu->push(n);

			if(sp_po->pop()->data() != n + 11)
			{
				return false;
			}

			n += n;
		}
	}

	return true;
}

template<>
bool const_add<string>(args_t args)
{
	{
		auto sp_pu = make_shared<pusher<string>>();
		auto sp_a = make_shared<flow::samples::math::const_adder<string>>("ho");
		auto sp_po = make_shared<popper<string>>();
	
		flow::graph g;
	
		g.add(sp_pu, "pusher");
		g.add(sp_a);
		g.add(sp_po, "popper");

		g.connect<string>(sp_pu, 0, sp_a, 0);
		g.connect<string>(sp_a, 0, sp_po, 0);

		g.start();

		size_t c = stoul(args["count"]);
		string s = "ho";
		while(c)
		{
			--c;

			sp_pu->push(s);

			if(sp_po->pop()->data() != s + "ho")
			{
				return false;
			}

			s += s;
		}
	}

	return true;
}

bool max_length(args_t args)
{
	size_t max_length = stoul(args["length"]);

	auto sp_pu = make_shared<pusher<int>>();
	auto sp_po = make_shared<popper<int>>();

	flow::graph g;

	g.add(sp_pu, "pusher");
	g.add(sp_po, "popper");

	g.connect<int>(sp_pu, 0, sp_po, 0, max_length);

	g.start();

	for(int i = 0; i != max_length + 1; ++i)
	{
		sp_pu->push(0);
	}

	for(int i = 0; i != max_length; ++i)
	{
		sp_po->pop();
	}

	if(sp_po->peek())
	{
		return false;
	}

	return true;
}

bool max_weight(args_t args)
{
	size_t max_weight = stoul(args["weight"]);

	auto sp_pu = make_shared<pusher<char>>();
	auto sp_po = make_shared<popper<char>>();

	flow::graph g;

	g.add(sp_pu, "pusher");
	g.add(sp_po, "popper");

	g.connect<char>(sp_pu, 0, sp_po, 0, 0, max_weight);

	g.start();

	for(int i = 0; i != max_weight + 1; ++i)
	{
		sp_pu->push('a');
	}

	for(int i = 0; i != max_weight; ++i)
	{
		sp_po->pop();
	}

	if(sp_po->peek())
	{
		return false;
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
	else if(strcmp(argv[1], "reconnect") == 0)
	{
		const char* types[] = { "halt", "count" };
		b = reconnect(make_args(types, &argv[2], argc - 2));
	}
	else if(strcmp(argv[1], "add_delay") == 0)
	{
		const char* types[] = { "" };
		b = add_delay(make_args(types, &argv[2], argc - 2));
	}
	else if(strcmp(argv[1], "add") == 0)
	{
		const char* types[] = { "type", "count" };
		b = add<void>(make_args(types, &argv[2], argc - 2));
	}
	else if(strcmp(argv[1], "const_add") == 0)
	{
		const char* types[] = { "type", "count" };
		b = const_add<void>(make_args(types, &argv[2], argc - 2));
	}
	else if(strcmp(argv[1], "max_length") == 0)
	{
		const char* types[] = { "length" };
		b = max_length(make_args(types, &argv[2], argc - 2));
	}
	else if(strcmp(argv[1], "max_weight") == 0)
	{
		const char* types[] = { "weight" };
		b = max_weight(make_args(types, &argv[2], argc - 2));
	}

	return b ? 0 : 1;
}
