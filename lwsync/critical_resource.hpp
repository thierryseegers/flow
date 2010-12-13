#ifndef LWSYNC_CRITICAL_RESOURCE_HPP
#define LWSYNC_CRITICAL_RESOURCE_HPP

/** @file critical_resource.hpp
  * @brief definition of critical resource pattern.
  * @author Vladimir Frolov
  *
  * Copyright (c) Vladimir Frolov, 2006-2008
  * Distributed under the Boost Software License, Version 1.0. (See
  * accompanying file LICENSE_1_0.txt or copy at
  * http://www.boost.org/LICENSE_1_0.txt)
  *
  */

#include <lwsync/resource_accessors.hpp>
#include <lwsync/synchronization_policy.hpp>

namespace lwsync
{

   /** @brief Implementation of a critical resource pattern.
     *
     * Critical resource automatically locks guard which is associated with
     * resource object when access to the resource object is required and
     * automatically unlocks it when access is finished.
     *
     * @param Resource Type of resource object.
     * @param Sync_policy Critical resource synchronization policy.
     */
   template<typename Resource,
            typename Sync_policy = default_resource_sync_policy
           >
   class critical_resource
   {
   public:
      typedef Resource resource_type;
      typedef Sync_policy resource_sync_policy;
      typedef typename resource_sync_policy::locker_type locker_type;

      typedef resource_accessor<resource_type, resource_sync_policy> accessor;
      typedef const_resource_accessor<resource_type, resource_sync_policy> const_accessor;

      /** @brief Initialize resource object with its default constructor.
        */
      critical_resource()
         : _resource()
         , _guard()
      {}

      /** @brief Initialize resource object with its copy constructor.
        */
      critical_resource(const resource_type& resource_copy)
         : _resource(resource_copy)
         , _guard()
      {}

	  critical_resource(resource_type&& resource_ref)
         : _resource(move(resource_ref))
         , _guard()
      {}

      /** @brief Provide copy of the critical resource. Resource will be initialized with
        * copy constructor.
        */
      critical_resource(const critical_resource& the_same)
         : _resource(the_same.copy())
         , _guard()
      {}
      
      /** Assign handled resource.
        *
        * @note resource_type::operator=() will be used to assign resource.
        */
      const critical_resource& operator=(const critical_resource& the_same)
      {
         if (this != &the_same)
         {
            resource_type resource_copy = the_same.copy();
            this->access().resource() = resource_copy;
         }
         return (*this);
      }

      /** @name Methods to access resource instance.
        */
      /*@{*/

      /** Explicit const access to non-const resource is required when rwlock
        * pattern is used to synchronize accesses.
        */
      const_accessor const_access() const
      {
         return const_accessor(&_resource, &_guard);
      }

      const_accessor access() const
      {
         return const_access();
      }

      accessor access()
      {
         return accessor(&_resource, &_guard);
      }

      /** Get copy of resource without handling accessors.
        *
        * @note It is deadlock-unsafe to use more than one critical resource in one
        * expression or more than one accessors in a scope. But in many cases it is
        * enough to use copy of resource and it doesn't require to lock more that one
        * guard at the same time. So copy() member of different critical resources can
        * be used many times in one expression without menace of deadlock.
        */
      resource_type copy() const
      {
         return const_access().resource();
      }
      /*@}*/

      /** @name Implicit resource access methods.
        *
        * @note Can be used in the following constructions:
        * @code
        * typedef critical_resource<my_class> my_critical;
        * my_critical my_critical_object;
        *
        * {
        *    // Equal to my_critical::accessor access = my_critical_object.access();
        *    my_critical::accessor access = my_critical_object;
        * }
        *
        * {
        *    // Equal to my_critical::accessor access = my_critical_object.const_access();
        *    my_critical::const_accessor access = my_critical_object;
        * }
        * @endcode
        */
      /*@{*/

      operator accessor()
      {
         return this->access();
      }

      operator const_accessor() const
      {
         return this->const_access();
      }

      /*@}*/

   protected:
   
      /** @name Direct accessors.
        * This methods allow monitor to inherit critical resource and get
        * direct access to resource and locker to wait until resource becomes
        * in defined state. This methods can serve for providing different kinds
        * of critical resource such as monitor.
        */
      /*@{*/
      resource_type& direct_access_resource()
      {
         return _resource;
      }

      const resource_type& direct_access_resource() const
      {
         return _resource;
      }

      locker_type& direct_access_locker() const
      {
         return _guard;
      }
      /*@}*/

   private:
      /** Resource object is critical and can be accessed from many threads
        * so all accesses has to be synchronized.
        */
      resource_type _resource;

      /** Guard object is used to synchronize all accesses (const and non-const) to
        * critical resource.
        * @note guard object is mutable because const resource has also to be 
        * synchronized within const methods.
        */
      mutable locker_type _guard;
   }; // class critical_resource

   /** @brief Reference specialization can provide synchronization
     * for existing external non-synchronized resource.
     *
     * @param Resource Original type of resource to guard.
     * @param Sync_policy Critical resource synchronization policy.
     */
   template<typename Resource, typename Sync_policy>
   class critical_resource<Resource&, Sync_policy>
   {
   private:
      /** Default constructor for reference specialization is prohibited
        * because it cannot be constructed only from existing non-synchronized
        * resource.
        *
        * @note has no implementation.
        */
      critical_resource();

      /** Copy constructor for reference specialization is prohibited
        * because synchronization for such a resource will be broken.
        * Only one instance of guard can be used to synchronize external
        * resource.
        *
        * @note has no implementation.
        */
      critical_resource(const critical_resource&);

      /** operator=() for reference specialization is prohibited
        * because synchronization for such a resource will be broken.
        * Only one instance of guard can be used to synchronize existing
        * resource.
        *
        * @note has no implementation.
        */
      const critical_resource& operator=(const critical_resource&);

   public:
      typedef Resource resource_type;
      typedef Sync_policy resource_sync_policy;
      typedef typename resource_sync_policy::locker_type locker_type;

      typedef resource_accessor<resource_type, resource_sync_policy> accessor;
      typedef const_resource_accessor<resource_type, resource_sync_policy> const_accessor;

      /** Initialize reference to existing resource.
        */
      critical_resource(resource_type& existing_resource)
         : _resource(existing_resource)
         , _guard()
      {}
      
      ~critical_resource() {};

      /** @name Provide accessors to resource reference.
        * Method to access reference of resource instance.
        */
      /*@{*/

      /** Explicit const access to non-const resource is required when rwlock
        * pattern is used to synchronize accesses.
        */
      const_accessor const_access() const
      {
         return const_accessor(&_resource, &_guard);
      }

      const_accessor access() const
      {
         return const_access();
      }

      accessor access()
      {
         return accessor(&_resource, &_guard);
      }

      /** It is posible to make copy from existing resource as well.
        */
      resource_type copy() const
      {
         return const_access().resource();
      }
      /*@}*/

      /** @name Implicit resource access methods.
        */
      /*@{*/

      operator accessor()
      {
         return this->access();
      }

      operator const_accessor() const
      {
         return this->const_access();
      }

      /*@}*/

   protected:

      /** @name Direct accessors.
        */
      /*@{*/
      resource_type& direct_access_resource()
      {
         return _resource;
      }

      const resource_type& direct_access_resource() const
      {
         return _resource;
      }

      locker_type& direct_access_locker() const
      {
         return _guard;
      }
      /*@}*/

   private:
      resource_type& _resource;

      mutable locker_type _guard;
   }; // class critical_resource

} // namespace lwsync

#endif // LWSYNC_CRITICAL_RESOURCE_HPP
