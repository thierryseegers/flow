#ifndef LWSYNC_DETAIL_FAKE_THREAD_FACILITIES_HPP
#define LWSYNC_DETAIL_FAKE_THREAD_FACILITIES_HPP

/** @file fake_thread_facilities.hpp
  * @brief This file provides fake facilities for Lwsync library in case if are not available.
  * @author Vladimir Frolov
  *
  * Copyright (c) Vladimir Frolov, 2006-2008
  * Distributed under the Boost Software License, Version 1.0. (See
  * accompanying file LICENSE_1_0.txt or copy at
  * http://www.boost.org/LICENSE_1_0.txt)
  *
  */

#include <exception>

namespace lwsync
{

   namespace detail
   {

      class has_no_implementation : public std::exception
      {
      public:
         has_no_implementation() {}
         const char* what() const throw() { return "Feature has no implementation.";}
      }; // class has_no_implementation

      class default_condition
      {
      public:
         default_condition()
         {
            throw has_no_implementation();
         }

         template<typename Locker_type>
         void wait(Locker_type)
         {
            throw has_no_implementation();
         }

         void notify_all()
         {
            throw has_no_implementation();
         }
      }; // class default_condition

      class default_mutex
      {
         default_mutex(const default_mutex&);
         const default_mutex& operator=(const default_mutex&);

      public:
         default_mutex()
         {
            throw has_no_implementation();
         }

         void lock()
         {
            throw has_no_implementation();
         }

         void unlock()
         {
            throw has_no_implementation();
         }

      }; // class default_mutex

   } // namespace detail

} // namespace lwsync

#endif // LWSYNC_DETAIL_FAKE_THREAD_FACILITIES_HPP
