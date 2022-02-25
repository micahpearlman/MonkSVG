/*
 *  mkSVG.cpp
 *  MonkSVG
 *
 *  Created by Micah Pearlman on 8/2/10.
 *  Copyright 2010 Zero Vision. All rights reserved.
 *
 */

#include "mkSVG.h"
#include "tinyxml/tinyxml.h"
#include <map>
#include <iterator>
#include <regex>
#include <sstream>
// #include <boost/tokenizer.hpp>
// #include <boost/regex.hpp>
// using namespace boost;

namespace MonkSVG {

class SVG_Parser_Implementation : public SVG_Parser {
  public:
    SVG_Parser_Implementation(ISVGHandler::SmartPtr handler)
        : _handler(handler) {}

    ISVGHandler::SmartPtr _handler;

    // holds svg <symbols>
    std::map<std::string, TiXmlElement *> _symbols;

    virtual bool parse(const char *data) {

        TiXmlDocument doc;
        doc.Parse(data);

        if (doc.Error()) {
            std::cerr << "ERROR: could not parse svg file." << std::endl;
            return false;
        }

        TiXmlElement *root = doc.FirstChild("svg")->ToElement();
        recursive_parse(root);

        // get bounds information from the svg file, ignoring non-pixel values

        std::string numberWithUnitString;
        std::regex  numberWithUnitPattern("^(-?\\d+)(px)?$");

        _handler->_minX = 0.0f;
        if (root->QueryStringAttribute("x", &numberWithUnitString) ==
            TIXML_SUCCESS) {
            std::match_results<std::string::const_iterator> matches;
            if (std::regex_search(numberWithUnitString, matches,
                                  numberWithUnitPattern)) {
                _handler->_minX = ::atof(matches[1].str().c_str());
            }
        }

        _handler->_minY = 0.0f;
        if (root->QueryStringAttribute("y", &numberWithUnitString) ==
            TIXML_SUCCESS) {
            std::match_results<std::string::const_iterator> matches;
            if (std::regex_search(numberWithUnitString, matches,
                                  numberWithUnitPattern)) {
                _handler->_minY = ::atof(matches[1].str().c_str());
            }
        }

        _handler->_width = 0.0f;
        if (root->QueryStringAttribute("width", &numberWithUnitString) ==
            TIXML_SUCCESS) {
            std::match_results<std::string::const_iterator> matches;
            if (std::regex_search(numberWithUnitString, matches,
                                  numberWithUnitPattern)) {
                _handler->_width = ::atof(matches[1].str().c_str());
            }
        }

        _handler->_height = 0.0f;
        if (root->QueryStringAttribute("height", &numberWithUnitString) ==
            TIXML_SUCCESS) {
            std::match_results<std::string::const_iterator> matches;
            if (std::regex_search(numberWithUnitString, matches,
                                  numberWithUnitPattern)) {
                _handler->_height = ::atof(matches[1].str().c_str());
            }
        }

        return true;
    }

    bool parse(const std::string &data) { return parse(data.c_str()); }

    void recursive_parse(TiXmlElement *element) {

        if (element) {
            for (TiXmlElement *sibbling = element; sibbling != 0;
                 sibbling = sibbling->NextSiblingElement()) {
                handle_xml_element(sibbling);
            }
        }

        if (element) {
            TiXmlElement *child = element->FirstChildElement();
            // if we don't handle the element recursively go into it
            if (handle_xml_element(child) == false) {
                recursive_parse(child);
            }
        }
    }

    bool handle_xml_element(TiXmlElement *element) {
        if (!element)
            return false;

        std::string type = element->Value();

        if (type == "g") {
            handle_group(element);
            return true;
        } else if (type == "path") {
            handle_path(element);
            return true;
        } else if (type == "rect") {
            handle_rect(element);
            return true;
        } else if (type == "polygon") {
            handle_polygon(element);
            return true;
        } else if (type == "symbol") {
            std::string id;
            if (element->QueryStringAttribute("id", &id) == TIXML_SUCCESS) {
                _symbols[id] = (TiXmlElement *)element->Clone();
            }
            return true;
        } else if (type == "use") {
            std::string href;
            if (element->QueryStringAttribute("xlink:href", &href) ==
                TIXML_SUCCESS) {
                std::string id = href.substr(1); // skip the #
                _handler->onUseBegin();
                // handle transform and other parameters
                handle_general_parameter(element);
                recursive_parse(_symbols[id]);
                _handler->onUseEnd();
            }

            return true;
        }
        return false;
    }

    void handle_group(TiXmlElement *pathElement) {

        _handler->onGroupBegin();

        // handle transform and other parameters
        handle_general_parameter(pathElement);

        // go through all the children
        TiXmlElement *children = pathElement->FirstChildElement();
        for (TiXmlElement *child = children; child != 0;
             child = child->NextSiblingElement()) {
            std::string type = child->Value();
            handle_xml_element(child);
        }

        _handler->onGroupEnd();
    }

    void handle_path(TiXmlElement *pathElement) {

        _handler->onPathBegin();
        std::string d;
        if (pathElement->QueryStringAttribute("d", &d) == TIXML_SUCCESS) {
            parse_path_d(d);
        }

        handle_general_parameter(pathElement);

        _handler->onPathEnd();
    }

    void handle_rect(TiXmlElement *pathElement) {
        _handler->onPathBegin();

        float pos[2] = {0, 0};
        if (pathElement->QueryFloatAttribute("x", &pos[0]) == TIXML_SUCCESS) {
            // parse_path_d( d );
        }
        if (pathElement->QueryFloatAttribute("y", &pos[1]) == TIXML_SUCCESS) {
            // parse_path_d( d );
        }
        float sz[2] = {0, 0};
        if (pathElement->QueryFloatAttribute("width", &sz[0]) ==
            TIXML_SUCCESS) {
            // parse_path_d( d );
        }
        if (pathElement->QueryFloatAttribute("height", &sz[1]) ==
            TIXML_SUCCESS) {
            // parse_path_d( d );
        }
        _handler->onPathRect(pos[0], pos[1], sz[0], sz[1]);

        handle_general_parameter(pathElement);

        _handler->onPathEnd();
    }

    void handle_polygon(TiXmlElement *pathElement) {
        _handler->onPathBegin();
        std::string points;
        if (pathElement->QueryStringAttribute("points", &points) ==
            TIXML_SUCCESS) {
            parse_points(points);
        }

        handle_general_parameter(pathElement);

        _handler->onPathEnd();
    }

    void handle_general_parameter(TiXmlElement *pathElement) {
        std::string fill;
        if (pathElement->QueryStringAttribute("fill", &fill) == TIXML_SUCCESS) {
            _handler->onPathFillColor(string_hex_color_to_uint(fill));
        }

        std::string stroke;
        if (pathElement->QueryStringAttribute("stroke", &stroke) ==
            TIXML_SUCCESS) {
            _handler->onPathStrokeColor(string_hex_color_to_uint(stroke));
        }

        std::string stroke_width;
        if (pathElement->QueryStringAttribute("stroke-width", &stroke_width) ==
            TIXML_SUCCESS) {
            float width = atof(stroke_width.c_str());
            _handler->onPathStrokeWidth(width);
        }

        std::string style;
        if (pathElement->QueryStringAttribute("style", &style) ==
            TIXML_SUCCESS) {
            parse_path_style(style);
        }

        std::string transform;
        if (pathElement->QueryStringAttribute("transform", &transform) ==
            TIXML_SUCCESS) {
            parse_path_transform(transform);
        }

        std::string id_;
        if (pathElement->QueryStringAttribute("id", &id_) == TIXML_SUCCESS) {
            _handler->onId(id_);
        }

        std::string opacity;
        if (pathElement->QueryStringAttribute("opacity", &opacity) ==
            TIXML_SUCCESS) {
            float o = atof(opacity.c_str());
            _handler->onPathFillOpacity(o);
            // TODO: ??? stroke opacity???
        }
        if (pathElement->QueryStringAttribute("fill-opacity", &opacity) ==
            TIXML_SUCCESS) {
            float o = atof(opacity.c_str());
            _handler->onPathFillOpacity(o);
        }

        std::string fillrule;
        if (pathElement->QueryStringAttribute("fill-rule", &fillrule) ==
            TIXML_SUCCESS) {
            _handler->onPathFillRule(fillrule);
        }
    }

    float d_string_to_float(char *c, char **str) {
        while (isspace(*c)) {
            c++;
            (*str)++;
        }
        while (*c == ',') {
            c++;
            (*str)++;
        }

        return strtof(c, str);
    }

    int d_string_to_int(char *c, char **str) {
        while (isspace(*c)) {
            c++;
            (*str)++;
        }
        while (*c == ',') {
            c++;
            (*str)++;
        }

        return (int)strtol(c, str, 10);
    }

    uint32_t string_hex_color_to_uint(std::string &hexstring) {
        uint32_t color = (uint32_t)strtol(hexstring.c_str() + 1, 0, 16);
        if (hexstring.length() ==
            7) { // fix up to rgba if the color is only rgb
            color = color << 8;
            color |= 0x000000ff;
        }

        return color;
    }

    void nextState(char **c, char *state) {
        if (**c == '\0') {
            *state = 'e';
            return;
        }

        while (isspace(**c)) {
            if (**c == '\0') {
                *state = 'e';
                return;
            }
            (*c)++;
        }
        if (isalpha(**c)) {
            *state = **c;
            (*c)++;

            if (islower(*state)) { // if lower case then relative coords (see
                                   // SVG spec)
                _handler->setRelative(true);
            } else {
                _handler->setRelative(false);
            }
        }

        // cout << "state: " << *state << endl;
    }

    void parse_path_transform(std::string &tr) {
        size_t p = tr.find("translate");
        if (p != std::string::npos) {
            size_t      left = tr.find("(");
            size_t      right = tr.find(")");
            std::string values = tr.substr(left + 1, right - 1);
            char       *c = const_cast<char *>(values.c_str());
            float       x = d_string_to_float(c, &c);
            float       y = d_string_to_float(c, &c);
            _handler->onTransformTranslate(x, y);
        } else if (tr.find("rotate") != std::string::npos) {
            size_t      left = tr.find("(");
            size_t      right = tr.find(")");
            std::string values = tr.substr(left + 1, right - 1);
            char       *c = const_cast<char *>(values.c_str());
            float       a = d_string_to_float(c, &c);
            _handler->onTransformRotate(a); // ??? radians or degrees ??
        } else if (tr.find("matrix") != std::string::npos) {
            size_t      left = tr.find("(");
            size_t      right = tr.find(")");
            std::string values = tr.substr(left + 1, right - 1);
            char       *cc = const_cast<char *>(values.c_str());
            float       a = d_string_to_float(cc, &cc);
            float       b = d_string_to_float(cc, &cc);
            float       c = d_string_to_float(cc, &cc);
            float       d = d_string_to_float(cc, &cc);
            float       e = d_string_to_float(cc, &cc);
            float       f = d_string_to_float(cc, &cc);
            _handler->onTransformMatrix(a, b, c, d, e, f);
        }
    }

    void parse_path_d(std::string &d) {
        char *c = const_cast<char *>(d.c_str());
        char  state = *c;
        nextState(&c, &state);
        while (state != 'e') {

            switch (state) {
            case 'm':
            case 'M': {
                // c++;
                float x = d_string_to_float(c, &c);
                float y = d_string_to_float(c, &c);
                _handler->onPathMoveTo(x, y);
                nextState(&c, &state);
            } break;

            case 'l':
            case 'L': {
                float x = d_string_to_float(c, &c);
                float y = d_string_to_float(c, &c);
                _handler->onPathLineTo(x, y);
                nextState(&c, &state);

            } break;

            case 'h':
            case 'H': {
                float x = d_string_to_float(c, &c);
                _handler->onPathHorizontalLine(x);
                nextState(&c, &state);

            } break;

            case 'v':
            case 'V': {
                float y = d_string_to_float(c, &c);
                _handler->onPathVerticalLine(y);
                nextState(&c, &state);

            } break;

            case 'c':
            case 'C': {
                float x1 = d_string_to_float(c, &c);
                float y1 = d_string_to_float(c, &c);
                float x2 = d_string_to_float(c, &c);
                float y2 = d_string_to_float(c, &c);
                float x3 = d_string_to_float(c, &c);
                float y3 = d_string_to_float(c, &c);
                _handler->onPathCubic(x1, y1, x2, y2, x3, y3);
                nextState(&c, &state);

            } break;

            case 's':
            case 'S': {
                float x2 = d_string_to_float(c, &c);
                float y2 = d_string_to_float(c, &c);
                float x3 = d_string_to_float(c, &c);
                float y3 = d_string_to_float(c, &c);
                _handler->onPathSCubic(x2, y2, x3, y3);
                nextState(&c, &state);

            } break;

            case 'a':
            case 'A': {
                float rx = d_string_to_float(c, &c);
                float ry = d_string_to_float(c, &c);
                float x_axis_rotation = d_string_to_float(c, &c);
                int   large_arc_flag = d_string_to_int(c, &c);
                int   sweep_flag = d_string_to_int(c, &c);
                float x = d_string_to_float(c, &c);
                ;
                float y = d_string_to_float(c, &c);
                _handler->onPathArc(rx, ry, x_axis_rotation, large_arc_flag,
                                    sweep_flag, x, y);
                nextState(&c, &state);

            } break;

            case 'z':
            case 'Z': {
                _handler->onPathClose();
                nextState(&c, &state);

            } break;

            case 'q':
            case 'Q': {
                float x1 = d_string_to_float(c, &c);
                float y1 = d_string_to_float(c, &c);
                float x2 = d_string_to_float(c, &c);
                float y2 = d_string_to_float(c, &c);
                _handler->onPathQuad(x1, y1, x2, y2);
                nextState(&c, &state);
            } break;

            default:
                // BUGBUG: can get stuck here!
                // TODO: figure out the next state if we don't handle a
                // particular state or just dummy handle a state!
                c++;
                break;
            }
        }
    }

    // semicolon-separated property declarations of the form "name : value"
    // within the ‘style’ attribute
    void parse_path_style(std::string &ps) {

        std::map<std::string, std::string> style_key_values;

        // split out the key-value pairs separated by ";"
        std::regex               values_seperator("\\;");
        std::vector<std::string> key_values(
            std::sregex_token_iterator(ps.begin(), ps.end(), values_seperator,
                                       -1),
            std::sregex_token_iterator());

        for (auto &kv : key_values) {
            std::regex               key_value_seperator("\\:");
            std::vector<std::string> key_values_split(
                std::sregex_token_iterator(kv.begin(), kv.end(),
                                           key_value_seperator, -1),
                std::sregex_token_iterator());
            // BUGBUG: assuming key and value can always be indexed by 0,1
            std::string key = key_values_split[0];
            std::string value = key_values_split[1];

            // remove white space
            key.erase(
                std::remove_if(key.begin(), key.end(),
                               [](unsigned char x) { return std::isspace(x); }),
                key.end());
            value.erase(
                std::remove_if(value.begin(), value.end(),
                               [](unsigned char x) { return std::isspace(x); }),
                value.end());

            // finally store it in our map
            style_key_values[key] = value;
        }

        // char_separator<char>            values_seperator(";");
        // char_separator<char>            key_value_seperator(":");
        // tokenizer<char_separator<char>> values_tokens(ps, values_seperator);
        // for (string values : values_tokens) {
        //     tokenizer<char_separator<char>> key_value_tokens(values,
        //                                                                key_value_seperator);
        //     tokenizer<char_separator<char>>::iterator k =
        //     key_value_tokens.begin();
        //     tokenizer<char_separator<char>>::iterator v = k; v++;
        //     // cout << *k << ":" << *v << endl;
        //     style_key_values[*k] = *v;
        // }

        std::map<std::string, std::string>::iterator kv =
            style_key_values.find(std::string("fill"));
        if (kv != style_key_values.end()) {
            if (kv->second != "none")
                _handler->onPathFillColor(string_hex_color_to_uint(kv->second));
        }

        kv = style_key_values.find("stroke");
        if (kv != style_key_values.end()) {
            if (kv->second != "none")
                _handler->onPathStrokeColor(
                    string_hex_color_to_uint(kv->second));
        }

        kv = style_key_values.find("stroke-width");
        if (kv != style_key_values.end()) {
            float width = atof(kv->second.c_str());
            _handler->onPathStrokeWidth(width);
        }

        kv = style_key_values.find("fill-rule");
        if (kv != style_key_values.end()) {
            _handler->onPathFillRule(kv->second);
        }

        kv = style_key_values.find("fill-opacity");
        if (kv != style_key_values.end()) {
            float o = atof(kv->second.c_str());
            _handler->onPathFillOpacity(o);
        }

        kv = style_key_values.find("opacity");
        if (kv != style_key_values.end()) {
            float o = atof(kv->second.c_str());
            _handler->onPathFillOpacity(o);
            // ?? TODO: stroke Opacity???
        }

        kv = style_key_values.find("stroke-opacity");
        if (kv != style_key_values.end()) {
            float o = atof(kv->second.c_str());
            _handler->onPathStrokeOpacity(o);
        }
    }

    void parse_points(std::string &points) {
        const std::regex ws_re("\\s*[,\\s*]\\s*"); // whitespace or comma
        std::vector<std::string> tokens(
            std::sregex_token_iterator(points.begin(), points.end(), ws_re, -1),
            std::sregex_token_iterator());
        // char_separator<char>            sep(", \t");
        // tokenizer<char_separator<char>> tokens(points, sep);
        float xy[2];
        int   xy_offset = 0; // 0:x, 1:y
        bool  first = true;
        _handler->setRelative(false);
        for (std::string p : tokens) {
            // remove white space
            p.erase(
                std::remove_if(p.begin(), p.end(),
                               [](unsigned char x) { return std::isspace(x); }),
                p.end());

            xy[xy_offset++] = (float)atof(p.c_str());

            if (xy_offset == 2) {
                xy_offset = 0;
                if (first) {
                    _handler->onPathMoveTo(xy[0], xy[1]);
                    first = false;
                } else
                    _handler->onPathLineTo(xy[0], xy[1]);
            }
        }
        _handler->onPathClose();
    }
};

SVG_Parser *SVG_Parser::create(ISVGHandler::SmartPtr handler) {
    return new SVG_Parser_Implementation(handler);
}

void SVG_Parser::destroy(SVG_Parser *svg_parser) { delete svg_parser; }
}; // namespace MonkSVG