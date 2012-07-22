#include "counted.h"
#include "dummies.h"

#include "flow.h"

#include <cstring>
#include <iostream>
#include <map>
#include <memory>
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

		this_thread::sleep_for(chrono::seconds(1));
	}

	return sp_tc->received[0] == n && sp_cc->received[0] == n;
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

	return b ? 0 : 1;
}
