#if !defined(FUNCTIONAL_DUMMY_CONSUMER_H)
	 #define FUNCTIONAL_DUMMY_CONSUMER_H

#include "node.h"

class dummy_consumer : public flow::consumer<int>
{
public:
	dummy_consumer() : flow::node("dummy_consumer"), flow::consumer<int>("dummy_consumer", 0)
	{
	}

	~dummy_consumer()
	{
		int i = 22;
	}

	virtual void ready(size_t)
	{
	}
};

#endif