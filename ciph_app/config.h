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

	class general_element : public element
	{
	public:
		DECLARE_ELEMENT("general")

	protected:
		BEGIN_ATTRIBUTE_MAP()
			// TODO combine name and type
			MANDATORY_ATTRIBUTE_ENTRY(str_attribute, "feed_name")
			MANDATORY_ATTRIBUTE_ENTRY(str_attribute, "archive_path")
			INITIALIZED_ATTRIBUTE_ENTRY(u32_attribute, "chunk_duration_sec", 300)
			MANDATORY_ATTRIBUTE_ENTRY(str_attribute, "source_type")
			MANDATORY_ATTRIBUTE_ENTRY(str_attribute, "source_name")
			//format digit+[m, h, d]
			INITIALIZED_ATTRIBUTE_ENTRY(str_attribute, "archive_duration", "15m")
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
			MANDATORY_ELEMENT_ENTRY(general_element)
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
