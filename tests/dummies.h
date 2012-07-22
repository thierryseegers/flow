#if !defined(TESTS_DUMMIES_H)
	 #define TESTS_DUMMIES_H

#include "node.h"

template<typename T>
class dummy_producer : public flow::producer<T>
{
public:
	dummy_producer(size_t outs = 0) : flow::node("dummy_producer"), flow::producer<T>("dummy_producer", outs)
	{
		int i = 22;
	}

	~dummy_producer()
	{
		int i = 22;
	}

	virtual void produce()
	{
		int i = 22;
	}
};

template<typename P, typename C>
class dummy_transformer : public flow::transformer<P, C>
{
public:
	dummy_transformer(size_t ins = 0, size_t outs = 0) : flow::node("dummy_transformer"), flow::transformer<P, C>("dummy_transformer", ins, outs)
	{
		int i = 22;
	}

	~dummy_transformer()
	{
		int i = 22;
	}

	virtual void ready(size_t)
	{
		int i = 22;
	}
};

template<typename T>
class dummy_consumer : public flow::consumer<T>
{
public:
	dummy_consumer(size_t ins = 0) : flow::node("dummy_consumer"), flow::consumer<T>("dummy_consumer", ins)
	{
		int i = 22;
	}

	~dummy_consumer()
	{
		int i = 22;
	}

	virtual void ready(size_t)
	{
		int i = 22;
	}
};

#endif