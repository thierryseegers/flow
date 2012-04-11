#ifndef LWSYNC_MONITOR_HPP
#define LWSYNC_MONITOR_HPP

/** @file monitor.hpp
  * @brief definition of monitor pattern.
  * @author Vladimir Frolov
  *
  * Copyright (c) Vladimir Frolov, 2006-2008
  * Distributed under the Boost Software License, Version 1.0. (See
  * accompanying file LICENSE_1_0.txt or copy at
  * http://www.boost.org/LICENSE_1_0.txt)
  *
  */

#include <utility>

#include <lwsync/critical_resource.hpp>

namespace lwsync
{

   /** @brief Implementation of a monitor pattern.
     *
     * Monitor is a kind of critical resource that allows one not only to synchronize
     * resource in concurrent environment but also allows to wait when resource
     * becomes to some defined state.
     *
     * @param Resource Type of resource to guard.
     * @param Sync_policy Critical resource synchronization policy.
     *
     * @note There is no necessity in partial template instantiation for references.
     * Monitor pattern just will inherit critical_resource<Resource&, Sync_policy> and all
     * will work well.
     *
     * @note critical resource pattern can be used with monitor synchronization
     * policy, but not vice versa. One can't instantiate monitor pattern with synchronization policy for
     * critical resource.
     */
   template<typename Resource,
            typename Sync_policy = default_monitor_sync_policy
           >
   class monitor : public critical_resource<Resource, Sync_policy>
   {
      typedef critical_resource<Resource, Sync_policy> inherited;
   
   public:
      // So g++ doesn't confuse with unistd.h/access().
      using critical_resource<Resource, Sync_policy>::access;

      /** @name Critical resource typedefs redefinintion.
        * Redefine all critical resource typedefs. Some compilers doesn't search
        * for names in base class in case if base class is template instantiated
        * with derived class template parameters so that redefinition is necessary.
        */
      /*@{*/
      typedef typename inherited::resource_type resource_type;
      typedef typename inherited::resource_sync_policy monitor_sync_policy;
      typedef typename monitor_sync_policy::locker_type locker_type;
      typedef typename inherited::const_accessor const_accessor;
      typedef typename inherited::accessor accessor;
      /*@}*/

      /** @brief exception which will be throwed by waiting routine if monitor object
        * becomes in canceled state.
        */
      typedef typename monitor_sync_policy::waiting_canceled waiting_canceled;

      monitor()
         : inherited()
      {}

      monitor(const resource_type& resource_copy)
         : inherited(resource_copy)
      {}

	  monitor(resource_type&& resource_ref)
         : inherited(std::move(resource_ref))
      {}

      monitor(const monitor& the_same)
         : inherited(the_same)
      {}

      const monitor& operator=(const monitor& the_same)
      {
         inherited::operator=(the_same);
         return (*this);
      }

      /** @name Monitor waiting methods.
        * Allow to access resource only after it becomes in defined state.
        *
        * @note accessors objects don't acquire lock because it is already
        * acquired by waiting system routine.
        */
      /*@{*/

      /** Explicitly wait for const resource.
        */
      template<typename Predicate>
      const_accessor const_wait_for(Predicate predicate)
      {
         monitor_sync_policy::const_wait(
               &inherited::direct_access_locker(),
               inherited::direct_access_resource(),
               predicate
            );
         return const_accessor(&inherited::direct_access_resource(),
                               &inherited::direct_access_locker(), false
                              );
      }

      template<typename Predicate>
      const_accessor wait_for(Predicate predicate) const
      {
         return const_wait_for();
      }

      /** @note Predicate will receive const reference to resource to prevent it from
        * changing and avoid livelock, when two threads are waiting for some monitor 
        * and the entire time are trying to re-evaluate state of resource after each
        * other to detect possible changes because non-const predicate invoked.
        */
      template<typename Predicate>
      accessor wait_for(Predicate predicate)
      {
         monitor_sync_policy::wait(
               &inherited::direct_access_locker(),
               inherited::direct_access_resource(),
               predicate
            );
         return accessor(&inherited::direct_access_resource(),
                         &inherited::direct_access_locker(), false
                        );
      }
      /*@}*/

      /** @name Wait for itself methods.
        *
        * If resource type is bool or resource can be converted to bool (for examle
        * resource is pointer or resource has operator bool() ),
        * monitor can wait until resource becomes in true state. But note that in case
        * if resource has operator bool() this operator has to be const and doesn't change
        * a resource. Internal predicate will use const reference to resource to convert it
        * to bool even in case if non-const waiting is invoked.
        */
      /*@{*/

      /** Directly wait for const resource.
        */
      const_accessor const_wait() const
      {
         monitor_sync_policy::const_wait(
               &inherited::direct_access_locker(),
               inherited::direct_access_resource(),
               wait_for_resource
            );
         return const_accessor(&inherited::direct_access_resource(),
                               &inherited::direct_access_locker(), false
                              );
      }

      const_accessor wait() const
      {
         return const_wait();
      }

      accessor wait()
      {
         monitor_sync_policy::wait(
               &inherited::direct_access_locker(),
               inherited::direct_access_resource(),
               wait_for_resource
            );
         return accessor(&inherited::direct_access_resource(),
                         &inherited::direct_access_locker(), false
                        );
      }
      /*@}*/

      /** @name Waiting cancelation methods.
        *
        * This methods can be used to cancel waiting for monitor or renew waiting status.
        * @note There is important difference between thread cancellation in pthread API and monitor
        * cancellation. cancel() method sets canceled state for monitor object but not for waiting
        * thread. So if someone is waiting for one monitor in some thread and waiting was canceled
        * the thread can successfully wait for some other monitor. But any thread will not be able to
        * wait for canceled monitor until renew() method is called. 
        */
      /*@{*/

      /** @brief Force monitor to reevaluate all waiting predicates.
        *
        * This method can be used in coupling with complex predicates when it is possible
        * that predicate internal state is changed and so that predicate has to be reevaluate.
        *
        * @note Touch method causes locker to be acquired and released.
        */
      void touch()
      {
         access();
      }

      /** @brief Set canceled state for monitor object and cause all threads which are
        * waiting for monitor to stop waiting with waiting_canceled exception.
        *
        * @note A thread can perform waiting for monitor only if monitor is not in
        * canceled state. If state of monitor object is canceled any attempt to invoke
        * wait() method will cause exception waiting_canceled.
        */
      void cancel()
      {
         accessor lock_guard = access();
         monitor_sync_policy::cancel(&inherited::direct_access_locker());
      }

      /** @brief return monitor object into non-canceled state.
        */
      void renew()
      {
         accessor lock_guard = access();
         monitor_sync_policy::renew(&inherited::direct_access_locker());
      }
      /*@}*/

   private:

      /** @brief One predicate is used to evaluate both const and non-const resources.
        */
      static bool wait_for_resource(const resource_type& resource)
      {
         return resource != 0;
      }

   }; // class monitor

} // namespace lwsync

#endif // LWSYNC_MONITOR_HPP
