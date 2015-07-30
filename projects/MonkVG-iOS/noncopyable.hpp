#ifndef MBGL_UTIL_NONCOPYABLE
#define MBGL_UTIL_NONCOPYABLE

namespace MonkVG { namespace util {

namespace non_copyable_
{

class noncopyable
{
protected:
    constexpr noncopyable() = default;
    ~noncopyable() = default;
    noncopyable( noncopyable const& ) = default;
    noncopyable& operator=(noncopyable const& ) = default;
};
}

typedef non_copyable_::noncopyable noncopyable;

}}

#endif
