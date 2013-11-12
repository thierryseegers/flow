#if !defined(FLOW_GRAPH_H)
	 #define FLOW_GRAPH_H

#include "named.h"
#include "node.h"

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <thread>

//!\file graph.h
//!
//!\brief Defines the flow::graph class.

namespace flow
{

//!\brief Object that manages the connections and state of multiple nodes.
//!
//! When starting or stopping a graph, nodes are started and stopped in a fashion to minize build-up of packets.
class graph : public named
{
	typedef std::map<std::string, std::shared_ptr<node>> nodes_t;
	nodes_t d_producers, d_transformers, d_consumers;

	typedef std::map<std::string, std::unique_ptr<std::thread>> threads_t;
	threads_t d_threads;

	typedef std::map<std::string, std::map<size_t, std::pair<std::string, size_t>>> connections_t;
	connections_t connections;

public:
	//!\param name_r The name of this graph.
	graph(const std::string name_r = "graph") : named(name_r)
	{}

	virtual ~graph()
	{
		stop();
	}

	//!\brief Adds a node to the graph.
	//!
	//! The node will initially be disconnected and paused.
	//!
	//!\param node_p Pointer to the node to add.
	//!\param name_r Optional. New name to give the node.
	virtual void add(std::shared_ptr<node> node_p, const std::string& name_r = std::string())
	{
		if(!name_r.empty())
		{
			node_p->rename(name_r);
		}

		if(std::dynamic_pointer_cast<detail::transformer>(node_p))
		{
			d_transformers[node_p->name()] = node_p;
		}
		else if(std::dynamic_pointer_cast<detail::producer>(node_p))
		{
			d_producers[node_p->name()] = node_p;
		}
		else if(std::dynamic_pointer_cast<detail::consumer>(node_p))
		{
			d_consumers[node_p->name()] = node_p;
		}

		connections[node_p->name()];
	}

	//!\brief Removes a node from the graph.
	//!
	//!\param name_r The name of the node to remove.
	//!
	//!\return Node that was removed.
	virtual std::shared_ptr<node> remove(const std::string& name_r)
	{
		std::shared_ptr<node> p;
		nodes_t *n;
		nodes_t::iterator i;

		if(n = find(name_r, i))
		{
			i->second->sever();
			p = i->second;
			n->erase(i);
		}

		connections.erase(name_r);

		return p;
	}

	//!\brief Removes a node from the graph.
	//!
	//!\param sp_node The name of the node to remove.
	virtual void remove(const std::shared_ptr<node>& sp_node)
	{
		remove(sp_node->name());
	}

	//!\brief Connects two nodes' pins from the graph together.
	//!
	//!\param p_name_r Name of the producing node.
	//!\param p_pin The index of the producing node's output pin to connect.
	//!\param c_name_r Name of the consuming node.
	//!\param c_pin The index of the consuming node's input pin to connect.
	//!\param max_length The maximum length to give the pipe. Do not set or set to 0 for uncapped length.
	//!\param max_weight The maximum weight to give the pipe. Do not set or set to 0 for uncapped weight.
	//!
	//!\return False if the nodes had not yet been added to the graph.
	template<typename T>
	bool connect(const std::string& p_name_r, const size_t p_pin, const std::string& c_name_r, const size_t c_pin, const size_t max_length = 0, const size_t max_weight = 0)
	{
		nodes_t::iterator p, c;
		
		// Confirm these two nodes are in the graph.
		if(!find(p_name_r, p) || !find(c_name_r, c))
		{
			return false;
		}
		
		std::dynamic_pointer_cast<producer<T>>(p->second)->connect(p_pin, std::dynamic_pointer_cast<consumer<T>>(c->second).get(), c_pin, max_length, max_weight);

		connections[p_name_r][p_pin] = std::make_pair(c_name_r, c_pin);

		return true;
	}

	//!\brief Connects two nodes' pins from the graph together.
	//!
	//!\param sp_p The producing node.
	//!\param p_pin The index of the producing node's output pin to connect.
	//!\param sp_c The consuming node.
	//!\param c_pin The index of the consuming node's input pin to connect.
	//!\param max_length The maximum length to give the pipe. Do not set or set to 0 for uncapped length.
	//!\param max_weight The maximum weight to give the pipe. Do not set or set to 0 for uncapped weight.
	//!
	//!\return False if the nodes had not yet been added to the graph.
	template<typename T>
	bool connect(std::shared_ptr<flow::producer<T>> sp_p, const size_t p_pin, std::shared_ptr<flow::consumer<T>> sp_c, const size_t c_pin, const size_t max_length = 0, const size_t max_weight = 0)
	{
		nodes_t::iterator i;
		
		// Confirm these two nodes are in the graph.
		if(!find(sp_p->name(), i) || !find(sp_c->name(), i))
		{
			return false;
		}
		
		sp_p->connect(p_pin, sp_c.get(), c_pin, max_length, max_weight);

		connections[sp_p->name()][p_pin] = std::make_pair(sp_c->name(), c_pin);

		return true;
	}

	//!\brief Disconnects a node's pin.
	//!
	//!\param sp_p The node.
	//!\param p_pin The pin's index.
	template<typename T>
	void disconnect(std::shared_ptr<flow::producer<T>> sp_p, const size_t p_pin)
	{
		sp_p->disconnect(p_pin);

		connections[sp_p->name()][p_pin] = std::make_pair(std::string(), 0);
	}

	//!\brief Disconnects a node's pin.
	//!
	//!\param sp_c The node.
	//!\param c_pin The pin's index.
	template<typename T>
	void disconnect(std::shared_ptr<flow::consumer<T>> sp_c, const size_t c_pin)
	{
		sp_c->disconnect(c_pin);

		connections[sp_c->name()][c_pin] = std::make_pair(std::string(), 0);
	}

	//!\brief Starts all nodes in the graph.
	//!
	//! To avoid packet build-up in pipes, pure consuming node are started first, transforming nodes second and pure producing nodes last.
	//! If a node had been stopped earlier, a new thread is created for it.
	virtual void start()
	{
		auto start_f = [this](nodes_t::value_type& i)
		{
			i.second->transition(state::started);

			if(d_threads.find(i.first) == d_threads.end())
			{
//				d_threads[i.first] = std::unique_ptr<std::thread>(new std::thread(std::ref(*i.second)));
				d_threads[i.first] = std::unique_ptr<std::thread>(new std::thread([&i]{ i.second->operator()(); }));	// Remove this workaround for bug in VC++11 (bug #734305) when possible.
			}
		};

		for(auto& i : d_consumers){ start_f(i); }
		for(auto& i : d_transformers){ start_f(i); }
		for(auto& i : d_producers){ start_f(i); }
	}

	//!\brief Pauses all nodes in the graph.
	//!
	//! To avoid packet build-up in pipes, pure producing node are paused first, transforming nodes second and pure consuming nodes last.
	virtual void pause()
	{
		auto pause_f = [this](nodes_t::value_type& i)
		{
			i.second->transition(state::paused);
		};

		for(auto& i : d_producers){ pause_f(i); }
		for(auto& i : d_transformers){ pause_f(i); }
		for(auto& i : d_consumers){ pause_f(i); }
	}

	//!\brief Stops all nodes in the graph.
	//!
	//! node::stop() is called on all nodes.
	virtual void stop()
	{
		auto stop_f = [this](nodes_t::value_type& i)
		{
			i.second->transition(state::stopped);

			graph::threads_t::iterator j = d_threads.find(i.first);
			if(j != d_threads.end())
			{
				j->second->join();
				d_threads.erase(j);
			}
		};

		for(auto& i : d_producers){ stop_f(i); }
		for(auto& i : d_transformers){ stop_f(i); }
		for(auto& i : d_consumers){ stop_f(i); }
	}

	//!\brief Produces a dot syntax of the graph.
	//!
	//!\param o The output stream to output the syntax.
	virtual std::ostream& to_dot(std::ostream& o)
	{
		o << "digraph " << (name() == "graph" ? "graph1" : name()) << "\n{\n";
		o << "\trankdir = LR\n";
		o << "\tnode [shape = record, fontname = \"Helvetica\"]\n";
		o << "\tedge [color = \"midnightblue\", labelfontname = \"Courier\"]\n";

		for(auto& p : connections)
		{
			for(auto& i : p.second)
			{
				o << "\t" << p.first << " -> " << i.second.first << " [taillabel = \"" << i.first << "\", headlabel = \"" << i.second.second << "\"]\n";
			}
		}

		o << "}" << std::endl;

		return o;
	}

private:
	virtual nodes_t* find(const std::string& name_r, nodes_t::iterator& i)
	{
		i = d_producers.find(name_r);

		if(i != d_producers.end())
		{
			return &d_producers;
		}
		else if((i = d_transformers.find(name_r)) != d_transformers.end())
		{
			return &d_transformers;
		}
		else if((i = d_consumers.find(name_r)) != d_consumers.end())
		{
			return &d_consumers;
		}

		return nullptr;
	}
};

}

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
