#if !defined(FLOW_TIMER_H)
	 #define FLOW_TIMER_H

#include <boost/date_time.hpp>
#include <boost/thread.hpp>
#include <lwsync/critical_resource.hpp>

#include <functional>
#include <vector>

//!\file timer.h
//!
//!\brief Defines the \ref flow::timer base class and the \ref flow::monotonous_timer concrete timer class.

namespace flow
{

//!\brief Base class for an object that notifies listeners at some interval.
class timer
{
	typedef lwsync::critical_resource<std::vector<std::function<void ()> > > listeners_cr_t;
	listeners_cr_t d_listeners_cr;

	lwsync::critical_resource<bool> d_stop_cr;

public:
	timer() : d_stop_cr(false) {}

	virtual ~timer() {}

	//!\brief Returns true if the timer is stopped.
	virtual bool stopped() const
	{
		return *d_stop_cr.const_access();
	}

	//!\brief Stop the timer.
	virtual void stop()
	{
		*d_stop_cr.access() = true;
	}

	//!\brief Adds a listener to this timer.
	//!
	//!\param listener A functor that will be called at interval.
	virtual void listen(const std::function<void ()>& listener)
	{
		d_listeners_cr.access()->push_back(listener);
	}

	//!\brief Returns a reference to listeners.
	virtual listeners_cr_t& listeners()
	{
		return d_listeners_cr;
	}

	//!\brief Execution function to be implemented be concrete timers.
	//!
	//! This function must return as soon as possible after the timer is stopped.
	virtual void operator()() = 0;
};

//!\brief Concrete timer that notifies listeners repeatedly at a set interval of time.
class monotonous_timer : public timer
{
	boost::posix_time::time_duration d_interval;

public:
	//!\param interval The time to wait between notifications.
	monotonous_timer(const boost::posix_time::time_duration& interval) : d_interval(interval) {}

	virtual ~monotonous_timer() {}

	//!\brief Implementation of timer::operator()().
	virtual void operator()()
	{
		while(!stopped())
		{
			{
				auto listeners_a = listeners().access();
				std::for_each(listeners_a->begin(), listeners_a->end(), std::mem_fun_ref(&std::function<void ()>::operator()));
			}
			
			//TODO: wait until time has expired OR timer is stopped.
			boost::thread::sleep(boost::get_system_time() + d_interval);
		}
	}
};

}

#endif
