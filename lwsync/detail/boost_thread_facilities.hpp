#ifndef LWSYNC_DETAIL_BOOST_THREAD_FACILITIES_HPP
#define LWSYNC_DETAIL_BOOST_THREAD_FACILITIES_HPP

/** @file boost_thread_facilities.hpp
  * @brief This file provides facilities based on Boost.Thread for Lwsync library.
  * @author Vladimir Frolov
  *
  * Copyright (c) Vladimir Frolov, 2006-2008
  * Distributed under the Boost Software License, Version 1.0. (See
  * accompanying file LICENSE_1_0.txt or copy at
  * http://www.boost.org/LICENSE_1_0.txt)
  *
  */

#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/condition.hpp>

namespace lwsync
{

   namespace detail
   {

      typedef boost::condition default_condition;
      typedef boost::recursive_mutex default_mutex;

   } // namespace detail

} // namespace lwsync

#endif // LWSYNC_DETAIL_BOOST_THREAD_FACILITIES_HPP
