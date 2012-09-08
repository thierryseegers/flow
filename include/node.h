#if !defined(FLOW_NODE_H)
	 #define FLOW_NODE_H

#include "named.h"
#include "packet.h"
#include "pipe.h"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>
#include <utility>

//!\file node.h
//!
//!\brief Defines the node base classes and the pin classes.

//!\namespace flow
//!
//!\brief All basic blocks to build a graph of packets streaming across nodes.
namespace flow
{

// Forward declarations.

template<typename T>
class outpin;

template<typename T>
class consumer;

template<typename T>
class producer;

class graph;

//!\namespace flow::state
//!
//!\brief Contains the different state values.
namespace state
{

//!\enum type
//!
//!\brief The state of a node.
enum type
{
	started,	//!< The node is in the started state.
	paused,		//!< The node is in the paused state.
	stopped		//!< The node is in the stopped state.
};

}

//!\brief Base class for a node's inlet or outlet.
//!
//! Pins are connected to one another through pipes.
//! Even when one pin is disconnected, the pipe remains attached to the remaining pin to mimize packet loss.
template<typename T>
class pin : public named
{
protected:
	std::shared_ptr<std::pair<pipe<T>, std::unique_ptr<std::mutex>>> d_pipe_sp; //!< Shared ownership of a pipe with the pin to which this pin is connected.

	friend class outpin<T>;

public:
	//!\param name_r The name of this pin. This will be typically generated from the name of the owning node.
	pin(const std::string& name_r) : named(name_r) {}

	//!\param o The pin to copy.
	pin(const pin<T>& o) : named(o.name()), d_pipe_sp(o.d_pipe_sp) {}

	virtual ~pin() {}

	//!\brief Disconnects this pin from its pipe.
	virtual void disconnect()
	{
		d_pipe_sp.reset();
	}
};

//!\brief Object that represents a node inlet.
//!
//! Nodes that consume packets, i.e. transformers and consumers, have at least one inpin.
template<typename T>
class inpin : public pin<T>
{
	std::condition_variable *d_transition_cv_p;
	std::mutex *d_transition_m_p;

	using pin<T>::d_pipe_sp;

	//!\brief Disconnect this inpin.
	virtual void disconnect()
	{
		{
			std::lock_guard<std::mutex> lg(*d_pipe_sp->second);
			d_pipe_sp->first.rename(d_pipe_sp->first.input()->name() + "_to_" + "nothing");
		}

		pin<T>::disconnect();
	}

	friend class consumer<T>;

public:
	//!\brief Constructor.
	//!
	//!\param name_r The name to give this node.
	//!\param transition_cv_p Pointer to the node's transition condition_variable.
	//!\param transition_m_p Pointer to the node's transition mutex.
	inpin(const std::string& name_r, std::condition_variable* transition_cv_p, std::mutex *transition_m_p)
		: pin<T>(name_r), d_transition_cv_p(transition_cv_p), d_transition_m_p(transition_m_p)
	{}

	virtual ~inpin() {}

	//!\brief Overrides named::rename.
	//!
	//! Ensures pipe is also renamed.
	//!
	//!\param name_r New name to give this inpin.
	virtual std::string rename(const std::string& name_r)
	{
		if(d_pipe_sp)
		{
			std::lock_guard<std::mutex> lg(*d_pipe_sp->second);
			if(d_pipe_sp->first.input())
			{
				d_pipe_sp->first.rename(d_pipe_sp->first.input()->name() + "_to_" + name_r);
			}
		}

		return named::rename(name_r);
	}

	//!\brief Check whether a packet is in the pipe.
	//!
	//!\return \c false if there is no pipe or the pipe is empty, \c true otherwise.
 	virtual bool peek() const
	{
		if(d_pipe_sp)
		{
			std::lock_guard<std::mutex> lg(*d_pipe_sp->second);
			return d_pipe_sp->first.length() != 0;
		}

		return false;
	}

	//!\brief Extracts a packet from the pipe.
	//!
	//!\return The next packet to be consumed if the inpin is connected to a pipe and the pipe is not empty, empty pointer otherwise.
	virtual std::unique_ptr<packet<T>> pop()
	{
		if(d_pipe_sp)
		{
			std::lock_guard<std::mutex> lg(*d_pipe_sp->second);
			return d_pipe_sp->first.pop();
		}

		return std::unique_ptr<packet<T>>();
	}

	//!\brief Notifies this pin that a packet has been queued to the pipe.
	//!
	//! When a producing node has moved a packet to the pipe, that node's outpin will call this function on the connected inpin.
	//! If this inpin's owning node state is flow::started, it touches the state signal the node there is a packet to be consumed.
	virtual void incoming()
	{
		std::unique_lock<std::mutex> ul(*d_transition_m_p);
		d_transition_cv_p->notify_one();
	}
};

//!\brief Object that represents a node outlet.
//!
//! Nodes that produce packets, i.e. producers and transformers, have at least one outpin.
template<typename T>
class outpin : public pin<T>
{
	using pin<T>::d_pipe_sp;
	
	//!\brief Disconnect this outpin.
	virtual void disconnect()
	{
		{
			std::lock_guard<std::mutex> lg(*d_pipe_sp->second);
			d_pipe_sp->first.rename(std::string("nothing") + "_to_" + d_pipe_sp->first.output()->name());
		}

		pin<T>::disconnect();
	}

	//!\brief Connect this outpin to an inpin with a pipe.
	//!
	//! If this output pin is already connected to a pipe, it will be disconnected.
	//! If the input pin is already connected to a pipe, it will be reused.
	//!
	//!\param inpin_r The inpin to which to connect this outpin.
	//!\param max_length The maximum length to give the pipe.
	//!\param max_weight The maximum weight to give the pipe.
	virtual void connect(inpin<T>& inpin_r, const size_t max_length = 0, const size_t max_weight = 0)
	{
		// Disconnect this outpin from it's pipe, if it has one.
		if(d_pipe_sp)
		{
			disconnect();
		}

		if(inpin_r.pin<T>::d_pipe_sp)
		{
			// The inpin already has a pipe, connect this outpin to it.
			std::lock_guard<std::mutex> lg(*inpin_r.pin<T>::d_pipe_sp->second);
			
			pipe<T> &inpin_pipe = inpin_r.pin<T>::d_pipe_sp->first;

			//... but first, disconnects it from it's other output pin.
			if(inpin_pipe.input())
			{
				inpin_pipe.input()->disconnect();
			}

			inpin_pipe.rename(pin<T>::name() + "_to_" + inpin_pipe.name());

			// Overwrite the pipe's parameters with new ones.
			inpin_pipe.cap_length(max_length);
			inpin_pipe.cap_length(max_weight);
		}
		else
		{
			// The inpin has no pipe, make a new one.
			pipe<T> p(pin<T>::name() + "_to_" + inpin_r.pin<T>::name(), this, &inpin_r, max_length, max_weight);
			d_pipe_sp = inpin_r.pin<T>::d_pipe_sp = std::make_shared<std::pair<pipe<T>, std::unique_ptr<std::mutex>>>(std::make_pair(std::move(p), std::unique_ptr<std::mutex>(new std::mutex()))); 
		}
	}

	friend class producer<T>;

public:
	//!\param name_r The name to give this outpin.
	outpin(const std::string& name_r) : pin<T>(name_r) {}

	virtual ~outpin() {}

	//!\brief Overrides named::rename.
	//!
	//! Ensures pipe is also renamed.
	//!
	//!\param name_r New name to give this outpin.
	virtual std::string rename(const std::string& name_r)
	{
		if(d_pipe_sp)
		{
			std::lock_guard<std::mutex> lg(*d_pipe_sp->second);
			if(d_pipe_sp->first.output())
			{
				d_pipe_sp->first.rename(name_r + "_to_" + d_pipe_sp->first.output()->name());
			}
		}

		return named::rename(name_r);
	}

	//!\brief Moves a packet to the pipe.
	//!
	//! Attempts to move the packet on the pipe.
	//! If the pipe has reached capacity, the call to pipe::push will fail and packet_p will remain valid.
	//!
	//!\return true if the packet was successfully moved to the pipe, false otherwise.
	virtual bool push(std::unique_ptr<packet<T>> packet_p)
	{
		if(!d_pipe_sp) return false;

		inpin<T>* inpin_p = 0;
		{
			std::lock_guard<std::mutex> lg(*d_pipe_sp->second);
			if(d_pipe_sp->first.push(std::move(packet_p)))
			{
				inpin_p = d_pipe_sp->first.output();
			}
		}

		if(inpin_p)
		{
			inpin_p->incoming();
		}

		return inpin_p == 0;
	}
};

//!\brief Base class common to all nodes.
class node : public named
{
	state::type d_state_a; //!< The state of this node.

	//!\brief Changes this node's state.
	//!
	//!\param s The new state.
	virtual void transition(state::type s)
	{
		std::unique_lock<std::mutex> ul(d_transition_m);

		d_state_a = s;

		// Notify the concrete class.
		switch(s)
		{
		case state::started: started(); break;
		case state::paused: paused(); break;
		case state::stopped: stopped(); break;
		default: break;
		}

		// Notify the execution loop.
		d_transition_cv.notify_one();
	}

	friend class graph;

protected:
	std::condition_variable d_transition_cv;	//!< The condition variable to monitor the node's state.
	std::mutex d_transition_m;					//!< The mutex to lock when waiting on d_transition_cv.

	//!\brief Disconnect all pins.
	virtual void sever() = 0;

public:
	//! Constructor.
	//!
	//!\param name_r The name to give this node.
	node(const std::string& name_r) : named(name_r), d_state_a(state::paused)
	{}

	//!\brief Move constructor.
	node(node&& node_rr) : named(std::move(node_rr)), d_state_a(node_rr.d_state_a)
	{}
	
	virtual ~node() {}

	//!\brief Returns the node's state
	virtual state::type state() const
	{
		return d_state_a;
	}
	
	//!\brief Indicates the node has been started.
	//!
	//! Concrete nodes can override this function to be notified of the state transition.
	virtual void started() {}

	//!\brief Indicates the node has been paused.
	//!
	//! Concrete nodes can override this function to be notified of the state transition.
	virtual void paused() {}

	//!\brief Indicates the node has been stopped.
	//!
	//! Concrete nodes can override this function to be notified of the state transition.
	virtual void stopped() {}

	//!\brief The node's execution function.
	//!
	//! When the node is started, this function is called.
	//! It will exit when the node is stopped.
	virtual void operator()() = 0;
};

//!\cond
namespace detail
{

// These base classes help flow::graph distinguish between types of nodes without having to know their template types.

class producer
{
public:
	virtual ~producer() {}
};

class transformer
{
public:
	virtual ~transformer() {}
};

class consumer
{
public:
	virtual ~consumer() {}
};

}
//!\endcond

template<typename T>
class consumer;

//!\brief Base class from which concrete pure producers derive.
//!
//! Concrete transformers should derive from flow::transformer.
template<typename T>
class producer : public virtual node, public detail::producer
{
	typedef std::vector<outpin<T>> outputs_t;
	outputs_t d_outputs;

protected:
	//!\brief Connect this producer to a consumer.
	//!
	//!\param p_pin The index of this node's output pin.
	//!\param consumer_p Pointer to the consumer node to conenct to.
	//!\param c_pin The index of the consumer node's input pin.
	virtual void connect(size_t p_pin, consumer<T>* consumer_p, size_t c_pin)
	{
		output(p_pin).connect(consumer_p->input(c_pin));
	}

	//!\brief Disconnect an outpin of this producer.
	//!
	//!\param pin Index of the pin to disconnect.
	virtual void disconnect(size_t pin)
	{
		output(pin).disconnect();
	}

	//!\brief Disconnect all pins.
	virtual void sever()
	{
		for(auto& outpin : d_outputs)
		{
			outpin.disconnect();
		}
	}

	//!\brief The node's execution function.
	//!
	//! This is the implementation of node::operator()().
	//! Nodes that are pure producers should use this function as their execution function.
	virtual void operator()()
	{
		state::type s(state());

		while(s != state::stopped)
		{
			if(s == state::paused)
			{
				std::unique_lock<std::mutex> ul(d_transition_m);
				d_transition_cv.wait(ul, [&s, this](){ return (s = this->state()) != state::paused; });
			}
			else
			{
				s = state();
			}

			if(s == state::started)
			{
				produce();
			}
		}
	}

	friend class graph;

public:
	//!\param name_r The name to give this node.
	//!\param outs Numbers of output pins.
	producer(const std::string& name_r, const size_t outs) : node(name_r)
	{
		for(size_t i = 0; i != outs; ++i)
		{
			d_outputs.push_back(outpin<T>(name_r + "_out" + static_cast<char>('0' + i)));
		}
	}

	virtual ~producer() {}

	//!\brief Returns the number of output pins.
	virtual size_t outs() const { return d_outputs.size(); }

	//!\brief Returns a reference to an outpin pin.
	//!
	//!\param n The index of the output pin.
	virtual outpin<T>& output(const size_t n) { return d_outputs[n]; }

	//!\brief Returns a const reference to an outpin pin.
	//!
	//!\param n The index of the output pin.
	virtual const outpin<T>& output(const size_t n) const { return d_outputs[n]; }

	//!\brief Returns a reference to the container of output pins.
	virtual outputs_t& outputs() { return d_outputs; }

	//!\brief Returns a const reference to the container of output pins.
	virtual const outputs_t& outputs() const { return d_outputs; }

	//!\brief Overrides named::rename.
	//!
	//! Ensure pins are also renamed.
	//!
	//!\param name_r New name to give this node.
	//!
	//!\return The node's previous name.
	virtual std::string rename(const std::string& name_r)
	{
		for(size_t i = 0; i != outs(); ++i)
		{
			output(i).outpin<T>::rename(name_r + "_out" + static_cast<char>('0' + i));
		}

		return named::rename(name_r);
	}

	//!\brief Producing function.
	//!
	//! This function is called from the operator()() execution function.
	//! It is the function that the concrete classes of producing nodes implement.
	//! The body of this function should push packets on its outpins.
	virtual void produce() = 0;
};

//!\brief Base class from which concrete pure consumers derive.
//!
//! Concrete transformers should derive from flow::transformer.
template<typename T>
class consumer : public virtual node, public detail::consumer
{
	typedef std::vector<inpin<T>> inputs_t;
	inputs_t d_inputs;

protected:
	//!\brief Disconnect an inpin of this consumer.
	//!
	//!\param pin Index of the pin to disconnect.
	virtual void disconnect(size_t pin)
	{
		input(pin).disconnect();
	}
	
	//!\brief Disconnect all pins.
	virtual void sever()
	{
		for(auto& inpin : d_inputs)
		{
			inpin.disconnect();
		}
	}

	//!\brief Tests whether there are any packets at any of the inpins.
	virtual bool incoming()
	{
		for(auto& inpin : d_inputs)
		{
			if(inpin.peek())
			{
				return true;
			}
		}

		return false;
	}

	//!\brief The node's execution function.
	//!
	//! This is the implementation of node::operator()().
	//! Nodes that are consumers should use this function as their execution function.
	virtual void operator()()
	{
		state::type s(state());
		
		while(s != state::stopped)
		{
			bool p = false;

			if(s == state::paused)
			{
				std::unique_lock<std::mutex> ul(d_transition_m);
				d_transition_cv.wait(ul, [&s, this](){ return (s = this->state()) != state::paused; });
			}
			else if(s == state::started)
			{
				std::unique_lock<std::mutex> ul(d_transition_m);
				d_transition_cv.wait(ul, [&s, &p, this](){ return ((s = this->state()) != state::started) || (p = this->incoming()); });
			}

			if(p)
			{
				for(size_t i = 0; i != ins(); ++i)
				{
					if(input(i).peek())
					{
						ready(i);
					}
				}
			}
		}
	}

	friend class graph;

public:
	//!\param name_r The name to give this node.
	//!\param ins Numbers of input pins.
	consumer(const std::string& name_r, const size_t ins) : node(name_r)
	{
		for(size_t i = 0; i != ins; ++i)
		{
			d_inputs.push_back(inpin<T>(name_r + "_in" + static_cast<char>('0' + i), &d_transition_cv, &d_transition_m));
		}
	}

	virtual ~consumer() {}

	//!\brief Returns the number of input pins.
	virtual size_t ins() const { return d_inputs.size(); }

	//!\brief Returns a reference to an input pin.
	//!
	//!\param n The index of the input pin.
	virtual inpin<T>& input(const size_t n) { return d_inputs[n]; }

	//!\brief Returns a const reference to an input pin.
	//!
	//!\param n The index of the input pin.
	virtual const inpin<T>& input(const size_t n) const { return d_inputs[n]; }

	//!\brief Returns a reference to the container of input pins.
	virtual inputs_t& inputs() { return d_inputs; }

	//!\brief Returns a const reference to the container of input pins.
	virtual const inputs_t& inputs() const { return d_inputs; }

	//!\brief Overrides named::rename.
	//!
	//! Ensure pins are also renamed.
	//!
	//!\param name_r New name to give this node.
	virtual std::string rename(const std::string& name_r)
	{
		for(size_t i = 0; i != ins(); ++i)
		{
			input(i).inpin<T>::rename(name_r + "_in" + static_cast<char>('0' + i));
		}

		return named::rename(name_r);
	}

	//!\brief Consuming function.
	//!
	//! This function is called from the operator()() execution function.
	//! It is the function that the concrete classes of consuming nodes implement.
	//! This function signals that a packet is ready at an input node.
	//!
	//! param n The index of the input pin at which a packet has arrived.
	virtual void ready(size_t n) = 0;
};

//!\brief Base class from which concrete transformers derive.
template<typename C, typename P>
class transformer : public consumer<C>, public producer<P>, public detail::transformer
{
	virtual void produce() {}

protected:
	//!\brief Disconnect all pins.
	virtual void sever()
	{
		consumer<C>::sever();

		producer<P>::sever();
	}

	//!\brief Implementation of node::operator()().
	virtual void operator()() { consumer<C>::operator()(); }
	
public:
	//!\param name_r The name to give this node.
	//!\param ins Numbers of input pins.
	//!\param outs Numbers of output pins.
	transformer(const std::string& name_r, const size_t ins, const size_t outs) : node(name_r), consumer<C>(name_r, ins), producer<P>(name_r, outs)
	{}

	virtual ~transformer() {}

	//!\brief Overrides named::rename.
	//!
	//! Ensures pins are also renamed.
	//!
	//!\param name_r New name to give this node.
	virtual std::string rename(const std::string& name_r)
	{
		for(size_t i = 0; i != producer<P>::outs(); ++i)
		{
			producer<P>::output(i).outpin<P>::rename(name_r + "_out" + static_cast<char>('0' + i));
		}

		for(size_t i = 0; i != consumer<C>::ins(); ++i)
		{
			consumer<C>::input(i).inpin<C>::rename(name_r + "_in" + static_cast<char>('0' + i));
		}

		return named::rename(name_r);
	}

	//!\brief consumer::ready() function to be implemented by concrete class.
	virtual void ready(size_t n) = 0;
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
