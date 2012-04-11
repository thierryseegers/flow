#ifndef LWSYNC_DETAIL_STD_THREAD_FACILITIES_HPP
#define LWSYNC_DETAIL_STD_THREAD_FACILITIES_HPP

/** @file std_thread_facilities.hpp
  * @brief This file provides facilities based on the C++11 STL for Lwsync library.
  * @author Thierry Seegers
  *
  * Copyright (c) Thierry Seegers, 2012
  * Distributed under the Boost Software License, Version 1.0. (See
  * accompanying file LICENSE_1_0.txt or copy at
  * http://www.boost.org/LICENSE_1_0.txt)
  *
  */

#include <mutex>
#include <condition_variable>

namespace lwsync
{

   namespace detail
   {
      
      typedef std::condition_variable_any default_condition;
      typedef std::recursive_mutex default_mutex;

   } // namespace detail

} // namespace lwsync

#endif // LWSYNC_DETAIL_STD_THREAD_FACILITIES_HPP
