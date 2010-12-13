#if !defined(FLOW_FLOW_H)
	 #define FLOW_FLOW_H

//!\file flow.h
//!
//!\brief flow public API. Convenience header that includes all necessary headers.

//!\mainpage flow
//!
//! flow is a headers-only <a href="http://en.wikipedia.org/wiki/C%2B%2B0x">C++0x</a> 
//! framework providing the building blocks for streaming data packets through a graph 
//! of data transforming nodes.
//!
//!\section principles Design Principles
//!
//! - use of unique_ptr
//! - explain how each node runs on its own thread
//! - data packet can have a consumption time
//!	- consumption time
//!
//!\section thirdparty Use of thirdparty libraries
//!
//! -# <a href="http://www.boost.org/">boost</a>: graph.h uses the boost 
//!		library to create threads.
//! -# <a href="http://www.codeproject.com/KB/threads/lwsync.aspx">lwsync</a>:
//!		node.h uses lwsync::critical_resource<T> and lwsync::monitor<T> to 
//!		perform  resource synchronization. This library internally uses 
//!		boost::thread by default. lwsync::critical_resource<T> was modified to 
//!		add a <a href="http://www2.research.att.com/~bs/C++0xFAQ.html#rval">move constructor</a>.
//!
//!\section history History
//!
//! Years ago I was part of a team developing audio software for an 
//! embedded platform. This team was part of a larger group with its other 
//! teams focused on other multimedia aspects.
//!
//! The software platform we used had a generic data streaming layer. 
//! Since all the software was written in C, so was that layer. 
//! At the time, it was evident to me that this kind of library could be 
//! elegantly written in C++. Alas, suggesting that the entire group use a 
//! different tool chain was out of the question. Suggesting that I, a young'un, 
//! rewrite this critical and widely use layer was also a lost cause.
//!
//! Fast forward to now and I'm itching to learn to use C++0x just like I learned 
//! C++ and I'm telling myslef: "Well, why the heck not!" I wanted to do it. 
//! I still do. I will!

#include "graph.h"
#include "named.h"
#include "node.h"
#include "packet.h"
#include "pipe.h"
#include "timer.h"

#endif