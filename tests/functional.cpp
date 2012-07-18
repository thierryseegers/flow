#include "dummy_consumer.h"

#include "flow.h"

#include <memory>

using namespace std;

bool empty_graph()
{
	flow::graph g;

	return true;
}

bool graph_dummy_nodes()
{
	flow::graph g;

	g.add(make_shared<dummy_consumer>());

	g.start();

	return true;
}

int main(int argc, char* argv[])
{
	bool b = true;

	if(strcmp(argv[1], "empty_graph") == 0)
	{
		b = empty_graph();
	}
	else if(strcmp(argv[1], "graph_dummy_nodes") == 0)
	{
		b = graph_dummy_nodes();
	}

	return b ? 0 : 1;
}