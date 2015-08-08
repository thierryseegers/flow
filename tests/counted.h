#if !defined(TESTS_COUNTED_H)
	 #define TESTS_COUNTED_H

#include "node.h"

template<typename T>
class produce_n : public flow::producer<T>
{
	size_t n, r;

public:
	produce_n(size_t n, size_t outs = 1) : flow::node("produce_n"), flow::producer<T>("produce_n", outs), n(n), r(n)
	{}

	virtual ~produce_n()
	{}

	virtual void produce()
	{
		if(n)
		{
			--n;

			for(auto& outpin : flow::producer<T>::outputs())
			{
				std::unique_ptr<flow::packet<T>> packet_p(new flow::packet<T>(T()));

				outpin.push(packet_p);
			}
		}
	}

	virtual void reset()
	{
		n = r;
	}
};

template<typename T>
class transformation_counter : public flow::transformer<T, T>
{
	std::vector<size_t> received;

public:
	transformation_counter(size_t inouts = 1) : flow::node("transformation_counter"), flow::transformer<T, T>("transformation_counter", inouts, inouts), received(inouts, 0)
	{}

	virtual ~transformation_counter()
	{}

	virtual void ready(size_t i)
	{
        auto p(flow::consumer<T>::input(i).pop());
		flow::producer<T>::output(i).push(p);
		++received[i];
	}

	virtual size_t count(size_t i)
	{
		return received[i];
	}

	virtual void reset()
	{
		received.assign(received.size(), 0);
	}
};

template<typename T>
class consumption_counter : public flow::consumer<T>
{
	std::vector<size_t> received;

public:
	consumption_counter(size_t ins = 1) : flow::node("consumption_counter"), flow::consumer<T>("consumption_counter", ins), received(ins, 0)
	{}

	virtual ~consumption_counter()
	{}

	virtual void ready(size_t i)
	{
		flow::consumer<T>::input(0).pop();
		++received[i];
	}

	virtual size_t count(size_t i)
	{
		return received[i];
	}

	virtual void reset()
	{
		received.assign(received.size(), 0);
	}
};

#endif