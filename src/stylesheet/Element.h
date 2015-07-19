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

#ifndef _CSS_CSSELEMENT_H
#define _CSS_CSSELEMENT_H

#include "Selector.h"
#include "PropertySet.h"
#include <set>

namespace StyleSheet {

class CssElement
{
public:
	CssElement(const CssSelector& selector, const CssPropertySet& properties)
		: selector_(selector), properties_(properties)
	{}
	// static ctor
	static CssElement parse(const std::string& str)
	{
		std::size_t posPropStart = str.find('{');
		std::size_t posPropEnd = str.find('}');

		std::string properties = str.substr(posPropStart + 1, posPropEnd - posPropStart - 1);
		boost::algorithm::trim(properties);

		return CssElement(CssSelector::parse(str.substr(0, posPropStart)), CssPropertySet::parse(properties));
	}

	bool operator<(const CssElement& e) const { return selector_ < e.selector_; }

	const CssSelector& getSelector() const { return selector_; }
	std::size_t getPropertyCount() const { return properties_.size(); }
	const CssPropertySet& getProperties() const { return properties_; }

	void addProperty(const CssProperty& prop) { properties_.add(prop); }

	std::string toString() const {
		return selector_.toString() + " { " + properties_.toString() + "}";
	}

private:
	CssSelector selector_;
	CssPropertySet properties_;
};

} // namespace StyleSheet

#endif // _CSS_CSSELEMENT_H
