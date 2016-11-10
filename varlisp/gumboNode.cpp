#include "gumboNode.hpp"

#include <gumbo_query/QueryUtil.h>

#include <sstream>
// #include "object.hpp"

namespace varlisp {

gumboNode::gumboNode() {}

gumboNode::gumboNode(const CNode& n, const std::shared_ptr<CDocument>& d)
    : mNode(n), mDocument(d)
{
}

gumboNode::gumboNode(const std::string& html)
{
    mDocument.reset(new CDocument);
    mDocument->parse(html);
    if (mDocument->isOK()) {
        CSelection s = mDocument->find("html");
        if (s.nodeNum()) {
            mNode = s.nodeAt(0);
        }
    }
}

void gumboNode::print(std::ostream& o) const
{
    if (mDocument && mNode.valid()) {
        CQueryUtil::writeOuterHtml(o, reinterpret_cast<GumboNode*>(mNode.get()));
    }
}

std::string gumboNode::attribute(const std::string& key) const
{
    if (this->valid()) {
        return mNode.attribute(key);
    }
    return "";
}

bool gumboNode::hasAttr(const std::string& key) const
{
    if (this->valid()) {
        for (size_t i = 0; i < mNode.attrNum(); ++i) {
            if (mNode.attrNameAt(i) == key) {
                return true;
            }
        }
    }
    return false;
}

std::string gumboNode::text() const
{
    if (this->valid()) {
        return mNode.text();
    }
    return "";
}

std::string gumboNode::textNeat() const
{
    if (this->valid()) {
        return mNode.textNeat();
    }
    return "";
}

std::string gumboNode::onwText() const
{
    if (this->valid()) {
        return mNode.ownText();
    }
    return "";
}

std::string gumboNode::tag() const
{
    if (this->valid()) {
        return mNode.tag();
    }
    return "";
}

bool        gumboNode::isText() const
{
    if (this->valid()) {
        return mNode.isGumboType(GUMBO_NODE_TEXT);
    }
    return false;
}

std::string gumboNode::innerHtml() const
{
    if (this->valid()) {
        std::ostringstream oss;
        CQueryUtil::writeInnerHtml(oss, reinterpret_cast<GumboNode*>(mNode.get()));
        return oss.str();
    }
    return "";
}

std::string gumboNode::outerHtml() const
{
    if (this->valid()) {
        std::ostringstream oss;
        CQueryUtil::writeOuterHtml(oss, reinterpret_cast<GumboNode*>(mNode.get()));
        return oss.str();
    }
    return "";
}

std::vector<gumboNode> gumboNode::find(const std::string& query) const
{
    std::vector<gumboNode> ret;
    if (this->valid()) {
        CSelection s = mNode.find(query);
        for (size_t i = 0; i < s.nodeNum(); ++i) {
            ret.emplace_back(s.nodeAt(i), mDocument);
        }
    }
    return ret;
}

} // namespace varlisp
