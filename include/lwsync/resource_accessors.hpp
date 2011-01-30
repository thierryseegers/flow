#ifndef LWSYNC_RESOURCE_ACCESSOR_HPP
#define LWSYNC_RESOURCE_ACCESSOR_HPP

/** @file resource_accessors.hpp
  * @brief definition of critical resource accessors.
  * @author Vladimir Frolov
  *
  * Copyright (c) Vladimir Frolov, 2006-2008
  * Distributed under the Boost Software License, Version 1.0. (See
  * accompanying file LICENSE_1_0.txt or copy at
  * http://www.boost.org/LICENSE_1_0.txt)
  *
  */

namespace lwsync
{

   /** @brief Resource accessor allows non-const access to critical resource
     * by locking resource guard for writing.
     *
     * @param Resource Type of resource to guard.
     * @param Sync_policy Critical resource synchronization policy.
     */
   template<typename Resource, typename Sync_policy>
   class resource_accessor
   {
   public:
      typedef Resource resource_type;
      typedef Sync_policy resource_sync_policy;
      typedef typename resource_sync_policy::locker_type locker_type;

      /** @brief Handle resource and its synchronization guard.
        *
        * @param resource resource to synchronize access to.
        * @param guard locker to synchronize access with.
        * @param aqurie_guard if false, then resource accessor has to handle resource after
        * waiting for monitor, and guard object is already acquired.
        */
      resource_accessor(resource_type* resource, locker_type* guard, bool aqurie_guard = true)
         : _resource(resource)
         , _guard(guard)
      {
         if (aqurie_guard)
            resource_sync_policy::lock(_guard, _resource);
      }

      /** Copy constructor can be used to escalate resource accessor from 
        * within a scope.
        *
        * @note Copy constructor will work only if resource locker has recursive
        * implementation and can be locked several times by one thread. Otherwise
        * behavior is undefined.
        */
      resource_accessor(const resource_accessor& the_same)
         : _resource(the_same._resource)
         , _guard(the_same._guard)
      {
         resource_sync_policy::lock(_guard, _resource);
      }

      /** Unlock resource guard after resource access is finished.
        */
      ~resource_accessor()
      {
         resource_sync_policy::unlock(_guard, _resource);
      }

      /** @name Resource access methods.
        * Provide non-const access to resource after guard is locked.
        */
      /*@{*/

      resource_type& operator*()
      {
         return *_resource;
      }

      resource_type* operator->()
      {
         return _resource;
      }

      /** Provide explicit access to resource.
        */
      resource_type& resource()
      {
         return *_resource;
      }

      /*@}*/

   private:
      /** Resource accessor cannot be assigned to guarantee that guard will be
        * locked and unlocked only once by one instance of resource accessor.
        *
        * @note has no implementation.
        */
      const resource_accessor& operator=(const resource_accessor&);

      /** Pointer to guarded resource to provide access.
        */
      resource_type* _resource;
      /** Pointer to guard to synchronize access.
        */
      locker_type* _guard;
   }; // class resource_accessor

   /** @brief Const resource accessor allows const access to critical resource
     * by locking resource guard for reading.
     *
     * @param Resource Type of resource to guard.
     * @param Sync_policy Critical resource synchronization policy.
     */
   template<typename Resource, typename Sync_policy>
   class const_resource_accessor
   {
   public:
      typedef Resource resource_type;
      typedef Sync_policy resource_sync_policy;
      typedef typename resource_sync_policy::locker_type locker_type;

      /** @brief Handle const resource and its synchronization guard.
        * @param resource resource to synchronize access to.
        * @param guard locker to synchronize access with.
        * @param aqurie_guard if false, then resource accessor has to handle resource after
        * waiting for monitor, and guard object is already acquired.
        */
      const_resource_accessor(
            const resource_type* resource, 
            locker_type* guard, bool aqurie_guard = true
      )  : _resource(resource)
         , _guard(guard)
      {
         if (aqurie_guard)
            resource_sync_policy::const_lock(_guard, _resource);
      }

      /** Copy constructor can be used to escalate const resource accessor from 
        * within a scope.
        *
        * @note Copy constructor will work olny if resource locker has recursive
        * implementation and can be locked several times by one thread. Otherwise
        * behavior is undefined.
        */
      const_resource_accessor(const const_resource_accessor& the_same)
         : _resource(the_same._resource)
         , _guard(the_same._guard)
      {
         resource_sync_policy::const_lock(_guard, _resource);
      }

      /** Unlock const resource guard after const resource access is finished.
        */
      ~const_resource_accessor()
      {
         resource_sync_policy::const_unlock(_guard, _resource);
      }

      /** @name Resource const access methods.
        * Provide const access to resource after guard is locked.
        */
      /*@{*/

      const resource_type& operator*() const
      {
         return *_resource;
      }

      const resource_type* operator->() const
      {
         return _resource;
      }

      /** Provide explicit access to const resource.
        */
      const resource_type& resource() const
      {
         return *_resource;
      }
      /*@}*/


   private:
      /** Resource const accessor cannot be assigned to guarantee that guard will be
        * locked and unlocked only once by one instance of const resource accessor.
        *
        * @warning has no implementation.
        */
      const const_resource_accessor& operator=(const const_resource_accessor&);

      /** Pointer to guarded resource to provide access.
        */
      const resource_type* _resource;
      /** Pointer to guard to synchronize access.
        */
      locker_type* _guard;
   }; // class const_resource_accessor

} // namespace lwsync

#endif // LWSYNC_RESOURCE_ACCESSOR_HPP
