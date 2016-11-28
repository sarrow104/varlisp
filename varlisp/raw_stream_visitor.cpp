#include "raw_stream_visitor.hpp"

#include <stdexcept>

#include <sss/util/PostionThrow.hpp>

#include "environment.hpp"
#include "object.hpp"

namespace varlisp {
void raw_stream_visitor::operator()(const varlisp::symbol& s) const
{
    Object* it = m_env.find(s.m_data);
    if (!it) {
        SSS_POSITION_THROW(std::runtime_error, "symbol ", s.m_data,
                          " not exsist");
    }
    boost::apply_visitor(raw_stream_visitor(m_o, m_env), *it);
}
}  // namespace varlisp
