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

#ifndef _CSS_CSSPROPERTY_H
#define _CSS_CSSPROPERTY_H

#include <boost/algorithm/string.hpp>
#include <string>
#include <stdexcept>

namespace StyleSheet {

class CssProperty
{
public:
  static const CssProperty& Empty() {
    static CssProperty emptyProperty("", ""); return emptyProperty;
  }

  CssProperty(const std::string& name, const std::string& value)
          : name_(name), value_(value)
  {
    // TODO: check name and value validity
  }

  // static ctor
  static CssProperty parse(const std::string& str)
  {
    std::size_t pos = str.find(':');
    if (pos == std::string::npos)
      throw std::invalid_argument("value not found");
    if ((pos + 1) == str.size())
      throw std::invalid_argument("value not found");
    std::string name = str.substr(0, pos);
    boost::algorithm::trim(name);

    std::size_t lastPos = str.find(';');
    std::string value = lastPos == std::string::npos ? str.substr(pos + 1) : str.substr(pos + 1, lastPos - pos - 1);
    boost::algorithm::trim(value);
    return CssProperty(name, value);
  }

  bool operator<(const CssProperty& p) const {
    return name_ < p.name_/* || (name_ == p.name_ && value_ < p.value_)*/;
  }

  const std::string& getName() const { return name_; }
  const std::string& getValue() const { return value_; }

  bool isValid() const { return !name_.empty(); }

  std::string toString() const { return name_ + ": " + value_ + ";"; }

private:
  std::string name_;
  std::string value_;
};

} // namespace StyleSheet

#endif // _CSS_CSSPROPERTY_H
