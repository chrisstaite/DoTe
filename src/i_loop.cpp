#include "i_loop.h"

namespace dote {

ILoop::Registration::Registration(ILoop* loop, int handle, Type type) :
    m_loop(loop),
    m_handle(handle),
    m_type(type)
{ }

ILoop::Registration::Registration() :
    m_loop(nullptr),
    m_handle(-1),
    m_type(Type::Moved)
{ }

ILoop::Registration::Registration(Registration&& other) :
    m_loop(other.m_loop),
    m_handle(other.m_handle),
    m_type(other.m_type)
{
    other.m_type = Type::Moved;
}

ILoop::Registration& ILoop::Registration::operator=(Registration&& other)
{
    std::swap(m_loop, other.m_loop);
    std::swap(m_handle, other.m_handle);
    std::swap(m_type, other.m_type);
    return *this;
}

ILoop::Registration::~Registration()
{
    reset();
}

void ILoop::Registration::reset() {
    switch (m_type) {
        case Moved:
            break;
        case Read:
            m_loop->removeRead(m_handle);
            break;
        case Write:
            m_loop->removeWrite(m_handle);
            break;
        case Exception:
            m_loop->removeException(m_handle);
            break;
    }
    m_type = Type::Moved;
}

bool ILoop::Registration::valid() const {
    return m_type != Type::Moved;
}

}  // namespace dote
