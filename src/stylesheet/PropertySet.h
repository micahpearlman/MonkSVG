/*
 *   Copyright 2010, Marco Ambu.
 *
 *   This file is part of StyleSheet library.
 *
 *   StyleSheet is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   StyleSheet is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with StyleSheet.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _CSS_PROPERTYSET_H
#define _CSS_PROPERTYSET_H

#include "Property.h"
#include <set>

namespace StyleSheet {

class CssPropertySet
{
public:
	static CssPropertySet parse(const std::string& str)
	{
		CssPropertySet properties;
		std::size_t posPropStart = 0;
		std::size_t lastPos = str.find(';', posPropStart);
		while (lastPos != std::string::npos)
		{
			std::string prop = str.substr(posPropStart, lastPos - posPropStart);
			properties.add(CssProperty::parse(prop));
			if (lastPos == std::string::npos || (lastPos + 1) >= str.size())
				break;
			posPropStart = lastPos + 1;
			lastPos = str.find(';', posPropStart);
		}
		return properties;
	}

	void add(const CssProperty& prop)
	{
		if (prop.getValue().empty())
			return;
		properties_.insert(prop);
	}
	void remove(const std::string& /*propName*/) {}

	void add(const CssPropertySet& propSet)
	{
		for (PropertySet::const_iterator it = propSet.properties_.begin(); it != propSet.properties_.end(); ++it)
			add(*it);
	}

	std::size_t size() const { return properties_.size(); }
	bool empty() const { return properties_.empty(); }

	const CssProperty& getProperty(const std::string& name) const
	{
		PropertySet::const_iterator it = std::find_if(properties_.begin(), properties_.end(), PropertyFinder(name));
		if (it == properties_.end())
			return CssProperty::Empty();
		return *it;
	}

	std::string toString() const
  {
		std::string res;
		for (PropertySet::const_iterator it = properties_.begin(); it != properties_.end(); ++it)
			res += it->toString() + " ";
		return res;
	}

private:
	typedef std::set<CssProperty> PropertySet;
	PropertySet properties_;

private:
	class PropertyFinder
	{
	public:
		PropertyFinder(const std::string& name)
			: name_(name)
		{}
		PropertyFinder(const PropertyFinder& other)
			: name_(other.name_)
		{}
		PropertyFinder& operator=(const PropertyFinder&);	// not implemented

		bool operator()(const CssProperty& prop) const {
			return prop.getName() == name_;
		}

	private:
		const std::string& name_;
	};
};

} // namespace StyleSheet

#endif // _CSS_PROPERTYSET_H
