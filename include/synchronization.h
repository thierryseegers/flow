// I take no credit for the code in this file.
// It is a lean, C++11-focused, version of some code of the lwsync library found at http://www.codeproject.com/Articles/15095/Lock-Wait-Synchronization-in-C.
//
// Original copyright notice:
//
// Copyright (c) Vladimir Frolov, 2006-2008
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(SYNCHRONIZATION_H)
	 #define SYNCHRONIZATION_H

#include <algorithm>
#include <mutex>

//!\cond

namespace synchronization
{

namespace detail
{

template<typename Mutex = std::mutex>
class resource_sync_policy
{
public:
    typedef Mutex mutex_type;

    template<typename Resource>
    static void const_lock(mutex_type* locker, const Resource*)
    {
        locker->lock();
    }

    template<typename Resource>
    static void lock(mutex_type* locker, Resource*)
    {
        locker->lock();
    }

    template<typename Resource>
    static void const_unlock(mutex_type* locker, const Resource*)
    {
        locker->unlock();
    }

    template<typename Resource>
    static void unlock(mutex_type* locker, Resource*)
    {
        locker->unlock();
    }
};

}

typedef detail::resource_sync_policy<> default_resource_sync_policy;

template<typename Resource, typename Mutex>
class resource_accessor
{
public:
    typedef Resource resource_type;
    typedef Mutex mutex_type;
	typedef typename std::unique_lock<mutex_type> locker_type;

    resource_accessor(resource_type* resource, mutex_type* guard)
        : resource_(resource)
		, guard_(guard)
		, locker_(*guard)
    {}

    resource_accessor(const resource_accessor& the_same)
        : resource_(the_same.resource)
        , guard_(the_same.guard)
		, locker_(*guard)
    {}

    ~resource_accessor()
    {}

    resource_type& operator*()
    {
        return *resource_;
    }

    resource_type* operator->()
    {
        return resource_;
    }

    resource_type& resource()
    {
        return *resource_;
    }

private:
    const resource_accessor& operator=(const resource_accessor&);

    resource_type *resource_;
	mutex_type *guard_;
    
	locker_type locker_;
};

template<typename Resource, typename Mutex>
class const_resource_accessor
{
public:
    typedef Resource resource_type;
    typedef Mutex mutex_type;
	typedef typename std::unique_lock<mutex_type> locker_type;

    const_resource_accessor(const resource_type* resource, mutex_type* guard)
		: resource_(resource)
        , guard_(guard)
		, locker_(*guard_)
    {}

    const_resource_accessor(const const_resource_accessor& the_same)
        : resource_(the_same.resource_)
        , guard_(the_same.guard)
		, locker_(*guard_)
    {}

    ~const_resource_accessor()
    {}

    const resource_type& operator*() const
    {
        return *resource_;
    }

    const resource_type* operator->() const
    {
        return resource_;
    }

    const resource_type& resource() const
    {
        return *resource_;
    }

private:
    const const_resource_accessor& operator=(const const_resource_accessor&);

    const resource_type* resource_;
    mutex_type *guard_;

	locker_type locker_;
};

template<typename Resource, typename Mutex = std::mutex>
class critical_resource
{
public:
    typedef Resource resource_type;
    typedef Mutex mutex_type;

    typedef resource_accessor<resource_type, mutex_type> accessor;
    typedef const_resource_accessor<resource_type, mutex_type> const_accessor;

    critical_resource()
        : resource_()
    {}

    critical_resource(const resource_type& resource)
        : resource_(resource)
    {}

	critical_resource(resource_type&& resource)
        : resource_(std::move(resource))
    {}

    critical_resource(const critical_resource& the_same)
        : resource_(the_same.copy())
    {}
      
    const critical_resource& operator=(const critical_resource& the_same)
    {
        if (this != &the_same)
        {
			resource_type resource_copy = the_same.copy();
			this->access().resource() = resource_copy;
        }

        return *this;
    }

    const_accessor const_access() const
    {
        return const_accessor(&resource_, &guard_);
    }

    const_accessor access() const
    {
        return const_access();
    }

    accessor access()
    {
        return accessor(&resource_, &guard_);
    }
    
	resource_type copy() const
    {
        return const_access().resource();
    }
    
	operator accessor()
    {
        return this->access();
    }

    operator const_accessor() const
    {
        return this->const_access();
    }

private:
    resource_type resource_;

    mutable mutex_type guard_;
};

}

//!\endcond

#endif