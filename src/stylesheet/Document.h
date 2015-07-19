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

#ifndef _CSS_CSSDOCUMENT_H
#define _CSS_CSSDOCUMENT_H

#include "Element.h"
#include <boost/tokenizer.hpp>

namespace StyleSheet {

class CssDocument
{
public:
	CssDocument() {}
	// static ctor
	static CssDocument parse(const std::string& doc)
	{
		CssDocument document;
		boost::char_separator<char> sep("}");
		boost::tokenizer<boost::char_separator<char> > tok(doc, sep);
		for (boost::tokenizer<boost::char_separator<char> >::iterator it = tok.begin(); it != tok.end(); ++it)
		{
			document.addElement(CssElement::parse(*it));
		}
		return document;
	}

	std::size_t getElementCount() const { return elements_.size(); }

	CssElement getElement(const CssSelector& selector) const
  {
		CssElementSet::const_iterator it = std::find_if(elements_.begin(), elements_.end(), SelectorFinder(selector));
		if (it == elements_.end())
			return CssElement(selector, CssPropertySet());
		return *it;
	}

	bool hasSelector(const CssSelector& selector) const {
		return std::find_if(elements_.begin(), elements_.end(), SelectorFinder(selector)) != elements_.end();
	}

	void addElement(const CssElement& element)
	{
		if (element.getPropertyCount() == 0)
			return;
		elements_.insert(element);
	}

	std::string toString() const
	{
		std::string res = "";
		for (CssElementSet::const_iterator it = elements_.begin(); it != elements_.end(); ++it)
		{
			res += it->toString() + "\n";
		}
		return res;
	}

private:
	typedef std::set<CssElement> CssElementSet;
	CssElementSet elements_;

private:
	class SelectorFinder
	{
	public:
		SelectorFinder(const CssSelector& selector)
			: selector_(selector)
		{}
		SelectorFinder(const SelectorFinder& other)
			: selector_(other.selector_)
		{}
		SelectorFinder& operator=(const SelectorFinder&);	// not implemented

		bool operator()(const CssElement& element) const {
			return element.getSelector() == selector_;
		}

	private:
		const CssSelector& selector_;
	};
};

} // namespace StyleSheet

#endif // _CSS_CSSDOCUMENT_H
