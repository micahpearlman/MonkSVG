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

#ifndef _CSS_CSSSTYLE_H
#define _CSS_CSSSTYLE_H

#include "PropertySet.h"
#include "Selector.h"
#include <boost/tokenizer.hpp>
#include <string>
#include <set>
#include <list>

namespace StyleSheet {

typedef std::list<CssSelector> CssSelectorList;

class CssStyle
{
public:
	CssStyle(const std::string& cssId, const std::string& cssType = "")
		: idSelector_(CssSelector::CssIdSelector(cssId))
		, typeSelector_(CssSelector::CssTypeSelector(cssType))
	{}

	const CssSelector& getIdSelector() const { return idSelector_; }
	const CssSelector& getTypeSelector() const { return typeSelector_; }

	void setType(const std::string& cssType) { typeSelector_ = CssSelector::CssTypeSelector(cssType); }

	const CssSelectorList& getClassSelectors() const { return classSelectors_; }

	void addClasses(const std::string& classTagValue)
	{
		boost::char_separator<char> sep(" ");
		boost::tokenizer<boost::char_separator<char> > tok(classTagValue, sep);
		for (boost::tokenizer<boost::char_separator<char> >::iterator it = tok.begin(); it != tok.end(); ++it)
		{
			addClass(*it);
		}
	}
	
	void addClass(const std::string& c)
	{
		if (c.empty())
			return;
		if (hasClass(c))
			return;
		classSelectors_.push_back(CssSelector::CssClassSelector(c));
	}
	void removeClass(const std::string& c)
	{
		CssSelectorList::iterator it = std::find(classSelectors_.begin(), classSelectors_.end(), CssSelector::CssClassSelector(c));
		if (it != classSelectors_.end())
			classSelectors_.erase(it);
	}
	bool hasClass(const std::string& c)
	{
		return std::find(classSelectors_.begin(), classSelectors_.end(), CssSelector::CssClassSelector(c)) != classSelectors_.end();
	}

	void setInlineProperties(const CssPropertySet& inlineProperties)
	{
		inlineProperties_ = inlineProperties;
	}

	const CssPropertySet& getInlineProperties() const { return inlineProperties_; }

private:
	CssSelector idSelector_, typeSelector_;
	CssSelectorList classSelectors_;
	CssPropertySet inlineProperties_;
};

} // namespace StyleSheet

#endif // _CSS_CSSSTYLE_H
