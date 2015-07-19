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

#ifndef _CSS_CSSPARSER_H
#define _CSS_CSSPARSER_H

#include "Document.h"
#include "Style.h"

namespace StyleSheet {

class CssParser
{
public:
	CssParser(const CssDocument& doc) : doc_(doc) {}

	CssPropertySet getProperties(const CssStyle& style) const
	{
		CssPropertySet properties;
		// First add inline properties...
		properties.add(style.getInlineProperties());
		// ... then properties by id
		properties.add(doc_.getElement(style.getIdSelector()).getProperties());
		// ... then properties by type
		properties.add(doc_.getElement(style.getTypeSelector()).getProperties());
		// TODO: ... then properties by class
		CssSelectorList classSelectors = style.getClassSelectors();
		for (CssSelectorList::const_iterator it = classSelectors.begin(); it != classSelectors.end(); ++it)
			properties.add(doc_.getElement(*it).getProperties());
		return properties;
	}

private:
	CssDocument doc_;
};

} // namespace StyleSheet

#endif // _CSS_CSSPARSER_H
