#if !defined(FUNCTIONAL_DUMMY_CONSUMER_H)
	 #define FUNCTIONAL_DUMMY_CONSUMER_H

#include "node.h"

class dummy_consumer : public flow::consumer
{
public:
	dummy_consumer() : flow::node("dummy_consumer"), flow::consumer("dummy_consumer", 0)
	{
	}

	~dummy_consumer()
	{
		int i = 22;
	}

	virtual void stop()
	{
		consumer::stop();
	}

	virtual void ready(size_t)
	{
	}
};

#endif