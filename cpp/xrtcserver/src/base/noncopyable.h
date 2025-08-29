#ifndef __BASE_NONCOPYABLE_H__
#define __BASE_NONCOPYABLE_H__

class noncopyable{
protected:
    noncopyable() = default;
    ~noncopyable() = default;
private:
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;
};

#endif