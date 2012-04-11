#ifndef LWSYNC_DETAIL_THREAD_FACILITIES_HPP
#define LWSYNC_DETAIL_THREAD_FACILITIES_HPP

/** @file thread_facilities.hpp
  * @brief This file provides configuration for Lwsync library facilities.
  * @author Vladimir Frolov
  *
  * Copyright (c) Vladimir Frolov, 2006-2008
  * Distributed under the Boost Software License, Version 1.0. (See
  * accompanying file LICENSE_1_0.txt or copy at
  * http://www.boost.org/LICENSE_1_0.txt)
  *
  */

#if defined(LWSYNC_HAS_CPP11)

	#include <lwsync/detail/std_thread_facilities.hpp>

#elif !defined(LWSYNC_HAS_NO_BOOST)

   #include <lwsync/detail/boost_thread_facilities.hpp>

#else // defined(LWSYNC_HAS_BOOST)

   #include <lwsync/detail/fake_thread_facilities.hpp>

#endif // defined(LWSYNC_HAS_BOOST)

#endif // LWSYNC_DETAIL_THREAD_FACILITIES_HPP
