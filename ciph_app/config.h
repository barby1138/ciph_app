// config.h
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "config.h"
#include <hyperon/gen_attributes.h>
#include <hyperon/element.h>
#include <hyperon/properties.h>
#include <hyperon/storage/expat_storage.h>

namespace
{
	class logger_element : public element
	{
	public:
		DECLARE_ELEMENT("logger")

	protected:
		BEGIN_ATTRIBUTE_MAP()
			MANDATORY_ATTRIBUTE_ENTRY(str_attribute, "level")
			MANDATORY_ATTRIBUTE_ENTRY(str_attribute, "path")
		END_ATTRIBUTE_MAP()
	};

	class stats_element : public element
	{
	public:
		DECLARE_ELEMENT("stats")

	protected:
		BEGIN_ATTRIBUTE_MAP()
			MANDATORY_ATTRIBUTE_ENTRY(u32_attribute, "dump_to_sec")
		END_ATTRIBUTE_MAP()
	};


	class dpdk_element : public element
	{
	public:
		DECLARE_ELEMENT("dpdk")

	protected:
		BEGIN_ATTRIBUTE_MAP()
			MANDATORY_ATTRIBUTE_ENTRY(str_attribute, "init_str")
		END_ATTRIBUTE_MAP()
	};

	class memif_element : public element
	{
	public:
		DECLARE_ELEMENT("memif")

	protected:
		BEGIN_ATTRIBUTE_MAP()
			MANDATORY_ATTRIBUTE_ENTRY(str_attribute, "conn_ids")
		END_ATTRIBUTE_MAP()
	};

	//////////////////////////////////////////////////
	// class properties_element
	//
	class properties_element : public element
	{
	public:
		DECLARE_ELEMENT("properties")

	protected:
		BEGIN_ELEMENT_MAP()
			MANDATORY_ELEMENT_ENTRY(logger_element)
			MANDATORY_ELEMENT_ENTRY(stats_element)
			MANDATORY_ELEMENT_ENTRY(dpdk_element)
			MANDATORY_ELEMENT_ENTRY(memif_element)
		END_ELEMENT_MAP()
	};

	//////////////////////////////////////////////////
	// class root_element
	//
	class root_element : public element
	{
	public:
		DECLARE_ELEMENT("root")

	protected:
		BEGIN_ELEMENT_MAP()
			MANDATORY_ELEMENT_ENTRY(properties_element)
		END_ELEMENT_MAP()
	};

	typedef properties_sngl<root_element, expat_storage> props;

} // namespace

#endif // _CONFIG_H_
