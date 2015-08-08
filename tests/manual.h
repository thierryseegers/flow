#if !defined(TESTS_MANUAL_H)
	 #define TESTS_MANUAL_H

#include "flow.h"

#include <memory>

template<typename T>
class pusher : public flow::producer<T>
{
public:
	pusher() : flow::node("pusher"), flow::producer<T>("pusher", 1)
	{}

	virtual ~pusher() {}

	virtual void produce() {}

	virtual void push(const T& t)
	{
		std::unique_ptr<flow::packet<T>> packet_p(new flow::packet<T>(t));

		flow::producer<T>::output(0).push(packet_p);
	}

	template<typename Duration>
	void push(const T& t, const Duration& d)
	{
		std::unique_ptr<flow::packet<T>> packet_p(new flow::packet<T>(t, d));

		flow::producer<T>::output(0).push(packet_p);
	}
};

template<typename T>
class popper : public flow::consumer<T>
{
	std::condition_variable d_incoming_cv;
	std::mutex d_incoming_m;

public:
	popper() : flow::node("popper"), flow::consumer<T>("popper", 1)
	{}

	virtual ~popper() {}

	virtual void ready(size_t)
	{
		d_incoming_cv.notify_one();
	}

	virtual std::unique_ptr<flow::packet<T>> pop()
	{
		std::unique_lock<std::mutex> ul(d_incoming_m);
		d_incoming_cv.wait(ul, [this](){ return this->flow::consumer<T>::input(0).peek(); });

		return flow::consumer<T>::input(0).pop();
	}

	virtual bool peek()
	{
		return flow::consumer<T>::input(0).peek();
	}
};

#endif