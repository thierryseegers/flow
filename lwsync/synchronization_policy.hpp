#ifndef LWSYNC_SYNCHRONIZATION_POLICY_HPP
#define LWSYNC_SYNCHRONIZATION_POLICY_HPP

/** @file synchronization_policy.hpp
  * @brief definition of critical resource and monitor synchronization policies.
  * @author Vladimir Frolov
  *
  * Copyright (c) Vladimir Frolov, 2006-2008
  * Distributed under the Boost Software License, Version 1.0. (See
  * accompanying file LICENSE_1_0.txt or copy at
  * http://www.boost.org/LICENSE_1_0.txt)
  *
  */

#include <lwsync/detail/thread_facilities.hpp>

#include <exception>

/** @brief Lwsync library provides lock&wait based patterns to build synchronization
  * on concurrent environment.
  *
  * Lwsync library provides only two patterns to build lock&wait based synchronization
  * on concurrent environment. But in most cases it is fully enough to have those two patterns
  * to build lock&wait based concurrent application. All patterns can be mapped to any system
  * synchronization facilities. By default they are mapped on boost::recursive_mutex and
  * boost::condition ones so that Lwsync is portable by default if boost library is available.
  *
  * If boost library is not available LWSYNC_HAS_NO_BOOST macro must be defined and non-default
  * synchronization policies should be used.
  */
namespace lwsync
{

   /** @brief Critical resource synchronization policy.
     *
     * Resource synchronization policy class is designed to work with boost::mutex
     * but one can easy define synchronization policy to work with any synchronization
     * primitive including rwlock pattern or anything else.
     *
     * @param Lock_facility System primitive that can be used to 
     * synchronize threads.
     *
     * @note You cannot use resource synchronization policy to synchronize a monitor
     * pattern. See monitor_sync_policy.
     * @note By default methods const_lock and lock do the same thing - lock a mutex
     * instance. But one can use const_lock and lock methods with any implementation
     * of rwlock pattern for reading and writing.
     *
     * @note Resource synchronization policy reserves access to synchronized resource
     * for user purposes. User can log all resources which was synchronized. But default
     * synchronization policy doesn't use resource object to perform synchronization.
     */
   template<typename Lock_facility = lwsync::detail::default_mutex >
   class resource_sync_policy
   {
      /** Fake constructor.
        * @note has no implementation.
        */
      resource_sync_policy();

   public:
      /** Type of system primitive that will be used by critical_resource to synchronize
        * resource.
        */
      typedef Lock_facility locker_type;

      /** @name Synchronization static methods.
        * Will be invoked with locker_type object instance by resource_accessor
        * and const_resource_accessor to synchronize access to resource.
        */
      /*@{*/

      template<typename Resource>
      static void const_lock(locker_type* locker, const Resource*)
      {
         locker->lock();
      }

      template<typename Resource>
      static void lock(locker_type* locker, Resource*)
      {
         locker->lock();
      }

      template<typename Resource>
      static void const_unlock(locker_type* locker, const Resource*)
      {
         locker->unlock();
      }

      template<typename Resource>
      static void unlock(locker_type* locker, Resource*)
      {
         locker->unlock();
      }

      /*@}*/
   }; // class resource_sync_policy

   /** Provide facility from mutex and condition variable to be used to
     * synchronize monitor pattern.
     *
     * @param Lock_type System primitive that will be used to provide 
     * lock synchronization.
     * @param Condition_type System primitive that will be used to provide 
     * wait synchronization.
     *
     * @note monitor lock facility is designed to use mutex as Lock_type and 
     * condition variable as Condition_type. But one can easy define monitor
     * synchronization facility with rwlock and event patterns.
     */
   template<typename Lock_type = lwsync::detail::default_mutex,
            typename Condition_type = lwsync::detail::default_condition
           >
   class monitor_sync_facility
   {
      /** Fake constructor.
        * @note has no implementation.
        */
      monitor_sync_facility(const monitor_sync_facility&);

      /** Fake assignment operator.
        * @note has no implementation.
        */
      const monitor_sync_facility& operator=(const monitor_sync_facility&);

   public:
      /** @brief Used to organize mutal execution when monitor is woke up after
        * waiting for non-const access.
        * @note pthread re
        */
      typedef Lock_type locker_type;

      /** @brief Exception will be thrown from wait() routine of monitor object in case if
        * some thread set canceled state of monitor object.
        *
        * @note If monitor object is in canceled state any waiting request will throw this
        * exception until renew method will not be invoked.
        */
      class waiting_canceled : public std::exception
      {
      public:
         virtual const char* what() const throw()
         {
            return "lwsync::monitor_sync_facility::waiting_canceled";
         }
      }; // class wait_canceled

      /** @brief Used to organize waiting for non-const access to finish.
        */
      typedef Condition_type waiter_type;

      monitor_sync_facility()
         : _locker()
         , _waiter()
         , _canceled(false)
      {}

      /** @name Locking functionality.
        */
      /*@{*/

      void lock()
      {
         _locker.lock();
      }

      void unlock()
      {
         _locker.unlock();
      }
      /*@}*/

      /** @name Waiting functionality.
        */
      /*@{*/

      /** @brief Pthread requires mutex to wait for condition. Win32 threading routines 
        * don't have such restriction but note that mutex has to be locked in this case
        * manually after thread is woke up from waiting to guarantee synchronization for
        * checking of predicate.
        */
      void wait()
      {
         _waiter.wait(_locker);
      }

      /** @brief Notify that non-const access is finished and all waiters have to 
        * wake up.
        */
      void wake_up_notify()
      {
         _waiter.notify_all();
      }

      /*@}*/

      /** @name Cancelation methods.
        */
      /*@{*/

      /** @brief Set canceled state of monitor_sync_facility and cancel all threads 
        * waiting with monitor_sync_facility.
        * @note All threads which invoke waiting will be canceled until renew
        * method will not be invoked.
        */
      void cancel()
      {
         _canceled = true;
         wake_up_notify();
      }

      /** @brief Reset canceled state of monitor_sync_facility.
        */
      void renew()
      {
         _canceled = false;
      }

      bool check()
      {
         if (_canceled)
            throw waiting_canceled();
         return true;
      }
      /*@}*/

   private:
      locker_type _locker;
      waiter_type _waiter;
      bool _canceled;
   }; // class monitor_sync_facility

   /** @brief Monitor synchronization policy.
     *
     * Monitor synchronization policy class is designed to work with boost::mutex
     * but one can easy define synchronization policy to work with any synchronization
     * primitive including rwlock pattern or anything else.
     *
     * @param Lock_facility System primitive that can be used to 
     * synchronize threads.
     *
     * @note You cannot use resource synchronization policy to synchronize a monitor
     * pattern because monitor requires not only locking but also waiting operations.
     * @note By default methods const_lock and lock do the same thing - lock a mutex
     * instance. But one can use const_lock and lock methods to lock rwlock pattern 
     * for reading and writing.
     */
   template<typename Lock_facility>
   class monitor_sync_policy
   {
      /** Fake constructor.
        * @note has no implementation.
        */
      monitor_sync_policy();

   public:
      typedef Lock_facility locker_type;

      typedef typename Lock_facility::waiting_canceled waiting_canceled;

      /** @name Redefining of critical resource locking methods.
        *
        * @note only non-const unlock do notifying all threads to wake up,
        * so predicate will not re-evaluated after const access have finished.
        */
      /*@{*/

      template<typename Resource>
      static void const_lock(locker_type* locker, const Resource*)
      {
         locker->lock();
      }

      template<typename Resource>
      static void lock(locker_type* locker, Resource*)
      {
         locker->lock();
      }

      template<typename Resource>
      static void const_unlock(locker_type* locker, const Resource*)
      {
         locker->unlock();
      }

      /** Notify all waiting threads to wake up and recheck predicate after
        * non-const access is finished.
        */
      template<typename Resource>
      static void unlock(locker_type* locker, Resource*)
      {
         locker->wake_up_notify();
         locker->unlock();
      }

      /*@}*/

      /** @name Waiting methods.
        *
        * @note It is possible for predicate or waiter to throw an exception. For example
        * waiter can be used as thread cancellation point and throw an exception when thread
        * have canceled. So that it is necessary to unlock resource guard in case if 
        * exception is thrown. In this case user will get the exception instead of access
        * to resource.
        */
      /*@{*/

      /** @brief Wait while resource becomes in a state when predicate returns true.
        *
        * @note locker has to be unlocked in wait facility on a period of waiting.
        * When predicate returns true monitor pattern will escalate accessor from
        * within a scope to unlock it after access is finished. So that there is a
        * guarantee that predicate will be invoked with resource and user be able to
        * access this resource within one lock (resource will not be changed between
        * predicate returns true and one will be able to access it).
        *
        * @note Non-const version waiting method receives const version of resource to
        * prevent from changing resource inside a predicate and avoid livelock, when two 
        * threads are waiting for some monitor and the entire time are trying re-evaluate
        * state of resource after each other to detect possible changes because non-const
        * predicate invoked.
        */
      template<typename Resource, typename Predicate>
      static void wait(locker_type* locker, const Resource& resource, Predicate predicate)
      {
         monitor_sync_policy::lock(locker, &resource);

         try
         {
            while(locker->check() && !predicate(resource))
               locker->wait();
         }
         catch(...)
         {
            monitor_sync_policy::unlock(locker, &resource);
            throw;
         }
      }

      /** Wait while const resource becomes in a state when predicate returns true.
        * @note It is possible to implement waiting for const resource in case if system
        * wait facility doesn't require locker object to be locked for writing (or doesn't
        * require locker object at all). So that monitor pattern will invoke const_wait
        * in case if const access to resource is required after waiting. But 
        * monitor_sync_policy is designed to always use mutal execution to work with
        * Boost.Thread facilities.
        */
      template<typename Resource, typename Predicate>
      static void const_wait(locker_type* locker, const Resource& resource, Predicate predicate)
      {
         monitor_sync_policy::const_lock(locker, &resource);

         try
         {
            while(locker->check() && !predicate(resource))
               locker->wait();
         }
         catch(...)
         {
            monitor_sync_policy::const_unlock(locker, &resource);
            throw;
         }
      }
      /*@}*/

      /* @name Cancelation methods.
       * @note methods have to be invoked inside of accessor object life time because any
       * synchronization is done inside.
       */
      /*@{*/

      static void cancel(locker_type* locker)
      {
         locker->cancel();
      }

      static void renew(locker_type* locker)
      {
         locker->renew();
      }
      /*@}*/

   }; // class monitor_sync_policy

   /** Default synchronization policy for critical_resource pattern.
     */
   typedef resource_sync_policy<> default_resource_sync_policy;

   /** Default synchronization policy for monitor pattern.
     */
   typedef monitor_sync_policy<monitor_sync_facility<> > default_monitor_sync_policy;

} // namespace lwsync

#endif // LWSYNC_SYNCHRONIZATION_POLICY_HPP
