#include "gumboNode.hpp"

#include <sstream>

#include <gq/QueryUtil.h>

#include "detail/html.hpp"

namespace varlisp {

gumboNode::gumboNode() {}

gumboNode::gumboNode(const CNode& n,
                     const std::shared_ptr<CDocument>& d,
                     const std::shared_ptr<std::string>& r)
    : mNode(n), mDocument(d), mRefer(r)
{
}

gumboNode::gumboNode(std::string&& html)
{
    this->reset(std::make_shared<std::string>(std::move(html)));
}

gumboNode::gumboNode(std::shared_ptr<std::string> html)
{
    this->reset(html);
}

void gumboNode::reset(std::shared_ptr<std::string> html)
{
    if (!html) {
        return;
    }
    auto document = std::make_shared<CDocument>();
    document->parse(html->c_str());

    if (document->isOK()) {
        CSelection s = document->find("html");
        if (s.nodeNum()) {
            mNode = s.nodeAt(0);
            mRefer = html;
            mDocument = document;
        }
    }
}

void gumboNode::print(std::ostream& o) const
{
    if (mDocument && mNode.valid()) {
        CQueryUtil::writeOuterHtml(
            o, reinterpret_cast<GumboNode*>(mNode.get()),
            varlisp::detail::html::get_gqnode_indent().c_str());
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
        CQueryUtil::writeInnerHtml(
            oss, reinterpret_cast<GumboNode*>(mNode.get()),
            varlisp::detail::html::get_gqnode_indent().c_str());
        return oss.str();
    }
    return "";
}

std::string gumboNode::outerHtml() const
{
    if (this->valid()) {
        std::ostringstream oss;
        CQueryUtil::writeOuterHtml(
            oss, reinterpret_cast<GumboNode*>(mNode.get()),
            varlisp::detail::html::get_gqnode_indent().c_str());
        return oss.str();
    }
    return "";
}

std::vector<gumboNode> gumboNode::find(const std::string& query) const
{
    try {
        std::vector<gumboNode> ret;
        if (this->valid()) {
            CSelection s = mNode.find(query);
            for (size_t i = 0; i < s.nodeNum(); ++i) {
                ret.emplace_back(s.nodeAt(i), mDocument, mRefer);
            }
        }
        return ret;
    }
    catch(const std::string& msg) {
        COLOG_ERROR(msg);
        return {};
    }
}

std::vector<gumboNode> gumboNode::children() const
{
    std::vector<gumboNode> ret;
    if (this->valid()) {
        for (size_t i = 0; i != mNode.childNum(); ++i) {
            ret.emplace_back(mNode.childAt(i), mDocument, mRefer);
        }
    }
    return ret;
}

} // namespace varlisp
