#ifndef __GUMBONODE_HPP_1478762530__
#define __GUMBONODE_HPP_1478762530__

// gumbo-node wrapper class
// 支持query语义；
// 首先是创建
// (gumbo "<html>") -> '(gumboNode)
// (gumbo "<html>" "query-string") -> '(gumboNode)
// 注意，返回一个单一的gumboNode；并且，这个node，默认引用到<html>标签。
// (gumbo-query '(gumboNode) "query-string") -> '(gumboNode)
// 这个，返回的是节点的链表；
//
// 注意，可以重复query;
//
// 支持的其他內建函数：
// (innerHtml '(gumboNode))
// (outerHtml '(gumboNode))
// (text '(gumboNode))
// (gp:attib CNode

#include <memory>
#include <iostream>

#include <vector>

#include <gq/Document.h>
#include <gq/Node.h>

#include "String.hpp"

namespace varlisp {

struct Environment;
struct List;

class gumboNode
{
public:
    gumboNode();
    gumboNode(const CNode& n,
              const std::shared_ptr<CDocument>& d,
              const std::shared_ptr<std::string>& r);
    // explicit gumboNode(const std::string& html);
    explicit gumboNode(std::string&& html);
    explicit gumboNode(std::shared_ptr<std::string> html);
    ~gumboNode() = default;

public:
    gumboNode(gumboNode&& ) = default;
    gumboNode& operator = (gumboNode&& ) = default;

public:
    gumboNode(const gumboNode& ) = default;
    gumboNode& operator = (const gumboNode& ) = default;

public:
    void reset(std::shared_ptr<std::string> html);
    void print(std::ostream& ) const;

    std::string attribute(const std::string& key) const;
    bool hasAttr(const std::string& key) const;
    std::string text() const;
    std::string textNeat() const;
    std::string onwText() const;

    std::string tag() const;
    bool        isText() const;

    std::string innerHtml() const;
    std::string outerHtml() const;

    bool operator == (const gumboNode& ref) const
    {
        return mNode.get() == ref.mNode.get();
    }

    bool operator<(const gumboNode& ) const
    {
        return false;
    }
    bool operator>(const gumboNode& ) const
    {
        return false;
    }

    bool valid() const {
        return mNode.valid();
    }

    CNode getCNode() const {
        return mNode;
    }

    std::vector<gumboNode> find(const std::string& query) const;
    std::vector<gumboNode> children() const;

private:
    CNode mNode;
    std::shared_ptr<CDocument> mDocument;
    std::shared_ptr<std::string> mRefer;
};

inline std::ostream& operator<<(std::ostream& o, const gumboNode& g)
{
    g.print(o);
    return o;
}

} // namespace varlisp

// 范例：
// void queryText(std::ostream& o, const std::string& utf8html,
//                const std::string& css_path)
// {
//     CDocument doc;
//     doc.parse(utf8html);
//     if (!doc.isOK()) {
//         return;
//     }
//
//     CSelection s = doc.find(css_path);
//
//     for (size_t i = 0; i != s.nodeNum(); ++i) {
//         CNode n = s.nodeAt(i);
//         o << n.textNeat();
//     }
// }
// 说明：
//   CDocument
//      gumbo_document 的实际管理类。子节点CNode都需要引用
//      这个类，才能访问具体数据。
//      mpOutput->GumboOutput*;
//
//   CNode:
//      子节点；
//      mpNode->GumboNode*;
//
//   CSelection: public CObject
//      mNodes->std::vector<GumboNode*>;
//
//   然后，这三种类型，都有find(const std::string query)->Selection方法。
//   并且，Selection可以方便地分出N个CNode。
//
//   然后需要注意的是CDocument是不支持拷贝的！

#endif /* __GUMBONODE_HPP_1478762530__ */
