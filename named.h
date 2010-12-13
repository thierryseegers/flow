#if !defined(FLOW_NAMED_H)
	 #define FLOW_NAMED_H

#include <string>

namespace flow
{

//!\brief Base class for objects that are named.
//!
//! This class is used to name all objects in the flow namespaces.
//! The names of objects are then used to reference them.
//! Names can also help debugging node connections since pipe and pin names are typically generated from the names of the nodes they connect.
class named
{
	std::string d_name;

public:
	//!\brief Constructor the takes a name.
	//!
	//!\param name_r The name of this object.
	named(const std::string& name_r) : d_name(name_r) {}
	
	//!\brief Move constructor.
	named(named&& name_rr) : d_name(std::move(name_rr.d_name)) {}

	virtual ~named() {}

	//!\brief A const reference to this object's name.
	virtual const std::string& name() const { return d_name; }

	//!\brief Sets this object's name to a new name.
	//!
	//!\return This object's former name.
	virtual std::string rename(const std::string& name_r) { std::string old = d_name; d_name = name_r; return old; }
};

}

#endif