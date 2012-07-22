#if !defined(TESTS_COUNTED_H)
	 #define TESTS_COUNTED_H

#include "node.h"

template<typename T>
class produce_n : public flow::producer<T>
{
	size_t n;

public:
	produce_n(size_t n, size_t outs = 1) : flow::node("produce_n"), flow::producer<T>("produce_n", outs), n(n)
	{
		int i = 22;
	}

	~produce_n()
	{
		int i = 22;
	}

	virtual void produce()
	{
		if(n)
		{
			--n;

			for(size_t i = 0; i != flow::producer<T>::outs(); ++i)
			{
				std::unique_ptr<flow::packet<T>> packet_p(new flow::packet<T>(T()));

				flow::producer<T>::output(i).push(std::move(packet_p));
			}
		}
	}
};

template<typename T>
class transformation_counter : public flow::transformer<T, T>
{
public:
	std::vector<size_t> received;

	transformation_counter(size_t inouts = 1) : flow::node("transformation_counter"), flow::transformer<T, T>("transformation_counter", inouts, inouts), received(inouts, 0)
	{
		int i = 22;
	}

	~transformation_counter()
	{
		int i = 22;
	}

	virtual void ready(size_t i)
	{
		flow::producer<T>::output(i).push(std::move(flow::consumer<T>::input(i).pop()));
		++received[i];
	}
};

template<typename T>
class consumption_counter : public flow::consumer<T>
{
public:
	std::vector<size_t> received;

	consumption_counter(size_t ins = 1) : flow::node("consumption_counter"), flow::consumer<T>("consumption_counter", ins), received(ins, 0)
	{
		int i = 22;
	}

	~consumption_counter()
	{
		int i = 22;
	}

	virtual void ready(size_t i)
	{
		flow::consumer<T>::input(0).pop();

		++received[i];
	}
};

#endif