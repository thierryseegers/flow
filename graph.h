#if !defined(FLOW_GRAPH_H)
	 #define FLOW_GRAPH_H

#include "named.h"
#include "node.h"

#include <boost/thread.hpp>

#include <memory>
#include <string>
#include <map>

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
	typedef std::map<std::string, std::shared_ptr<node> > nodes_t;
	nodes_t d_producers, d_transformers, d_consumers;

	typedef std::map<std::string, boost::thread*> threads_t;
	threads_t d_threads;

public:
	//!\param name_r The name of this graph.
	graph(const std::string& name_r = "graph") : named(name_r)
	{}

	virtual ~graph()
	{}

	//!\brief Adds a node to the graph.
	//!
	//! The node will be disconnected.
	virtual void add(std::shared_ptr<node> node_p)
	{
		if(dynamic_cast<transformer*>(node_p.get()))
		{
			d_transformers[node_p->name()] = node_p;
		}
		else if(dynamic_cast<producer*>(node_p.get()))
		{
			d_producers[node_p->name()] = node_p;
		}
		else if(dynamic_cast<consumer*>(node_p.get()))
		{
			d_consumers[node_p->name()] = node_p;
		}
	}

	//!\brief Removes a node from the graph.
	//!
	//!\param name_r The name of the node to remove.
	virtual std::shared_ptr<node> remove(const std::string& name_r)
	{
		std::shared_ptr<node> p;

		nodes_t *nodes_p;
		auto i = find(name_r, nodes_p);

		if(nodes_p)
		{
			// Disconnect all inpins.
			consumer *c_p = dynamic_cast<consumer*>(i->second.get());
			if(c_p)
			{
				for(size_t i = 0; i != c_p->ins(); ++i)
				{
					c_p->input(i).disconnect();
				}
			}

			// Disconnect all outpins.
			producer *p_p = dynamic_cast<producer*>(i->second.get());
			if(p_p)
			{
				for(size_t i = 0; i != p_p->outs(); ++i)
				{
					p_p->output(i).disconnect();
				}
			}

			p = i->second;
			nodes_p->erase(i);
		}

		return p;
	}

	//!\brief Conect two nodes from the graph together.
	//!
	//!\param p_name_r The name of the producing node.
	//!\param p_pin The index of the producing node's output pin to connect.
	//!\param c_name_r The name of the consuming node.
	//!\param c_pin The index of the consuming node's input pin to connect.
	virtual bool connect(const std::string& p_name_r, const size_t p_pin, const std::string& c_name_r, const size_t c_pin)
	{
		nodes_t *nodes_p;
		auto p_node_i = find(p_name_r, nodes_p);
		
		if(!nodes_p)
		{
			return false;
		}
		
		auto c_node_i = find(c_name_r, nodes_p);

		if(!nodes_p)
		{
			return false;
		}

		dynamic_cast<producer*>(p_node_i->second.get())->output(p_pin).connect(dynamic_cast<consumer*>(c_node_i->second.get())->input(c_pin));

		return true;
	}

	//!\brief Starts all nodes in the graph.
	//!
	//! To avoid packet build-up in pipes, pure consuming node are started first, transforming nodes second and pure producing nodes last.
	//! If a node is new to the graph, a thread is created for it.
	//! If a node was already present and had previously been started, node::start() is called.
	virtual void start()
	{
		auto start_f = [this](std::pair<const nodes_t::key_type, nodes_t::mapped_type>& i)
		{
			if(d_threads.find(i.first) == d_threads.end())
			{
				d_threads[i.first] = new boost::thread(boost::ref(*i.second));
			}
			else
			{
				i.second->start();
			}
		};

		std::for_each(d_consumers.begin(), d_consumers.end(), start_f);
		std::for_each(d_transformers.begin(), d_transformers.end(), start_f);
		std::for_each(d_producers.begin(), d_producers.end(), start_f);
	}

	//!\brief Pauses all nodes in the graph.
	//!
	//! To avoid packet build-up in pipes, pure producing node are paused first, transforming nodes second and pure consuming nodes last.
	virtual void pause()
	{
		auto pause_f = [this](std::pair<const nodes_t::key_type, nodes_t::mapped_type>& i)
		{
			i.second->pause();
		};

		std::for_each(d_producers.begin(), d_producers.end(), pause_f);
		std::for_each(d_transformers.begin(), d_transformers.end(), pause_f);
		std::for_each(d_consumers.begin(), d_consumers.end(), pause_f);
	}

	//!\brief Stops all nodes in the graph.
	//!
	//! node::stop() is called on all nodes.
	//!
	//!\param join If true, threads are joined before they are destroyed.
	virtual void stop(bool join = true)
	{
		auto stop_f = [this, join](std::pair<const nodes_t::key_type, nodes_t::mapped_type>& i)
		{
			i.second->stop();

			graph::threads_t::iterator j = d_threads.find(i.first);
			if(j != d_threads.end())
			{
				if(join)
				{
					j->second->join();
				}
				delete j->second;
				d_threads.erase(j);
			}
		};

		std::for_each(d_producers.begin(), d_producers.end(), stop_f);
		std::for_each(d_transformers.begin(), d_transformers.end(), stop_f);
		std::for_each(d_consumers.begin(), d_consumers.end(), stop_f);
	}

	//!\brief Finds a node in the graph.
	//!
	//!\param name_r The name of the node to find.
	//!
	//!\return A pointer to the node if found, 0 otherwise.
	virtual node* find(const std::string& name_r)
	{
		nodes_t *nodes_p;
		auto i = find(name_r, nodes_p);
		
		if(nodes_p)
		{
			return i->second.get();
		}

		return 0;
	}

private:
	virtual nodes_t::iterator find(const std::string& name_r, nodes_t*& nodes_pr)
	{
		nodes_pr = 0;

		auto i = d_producers.find(name_r);

		if(i != d_producers.end())
		{
			nodes_pr = &d_producers;
		}
		else if((i = d_transformers.find(name_r)) != d_transformers.end())
		{
			nodes_pr = &d_transformers;
		}
		else if((i = d_consumers.find(name_r)) != d_consumers.end())
		{
			nodes_pr = &d_consumers;
		}

		return i;
	}
};

}

#endif