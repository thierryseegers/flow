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

		flow::producer<T>::output(0).push(std::move(packet_p));
	}
};

template<typename T>
class popper : public flow::consumer<T>
{
public:
	popper() : flow::node("popper"), flow::consumer<T>("popper", 1)
	{}

	virtual ~popper() {}

	virtual void ready(size_t) {}

	virtual std::unique_ptr<flow::packet<T>> pop()
	{
		return flow::consumer<T>::input(0).pop();
	}
};

#endif