#include "xml_loader.h"

#include <fstream>

using namespace std;

bool XMLLoader::load(const std::string &path)
{
    m_doc.clear();

    ifstream f;
    f.open(path, ios::in | ios::binary | ios::ate);
    if(!f.is_open())
    {
        return false;
    }
    auto fsize = f.tellg();
    f.seekg(0, ios::beg);
    m_buf.resize(fsize);
    if(!f.read(m_buf.data(), m_buf.size()).good())
    {
        return false;
    }

    m_doc.parse<0>(m_buf.data());
    return true;
}
