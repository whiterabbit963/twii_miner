#ifndef XML_LOADER_H
#define XML_LOADER_H

#include <string>
#include <rapidxml/rapidxml.hpp>

class XMLLoader
{
public:
    XMLLoader() = default;
    bool load(const std::string &path);
    rapidxml::xml_document<> &doc() { return m_doc; };

private:
    std::string m_buf;
    rapidxml::xml_document<> m_doc;
};

#endif // XML_LOADER_H
