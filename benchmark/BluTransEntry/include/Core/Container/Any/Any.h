#ifndef ANY_HPP
#define ANY_HPP
namespace BluTrans::Container{
class Any {
    Any() = delete;
    template <typename _Ty>
    Any(const _Ty& type);
    Any(const Any& other);
    Any& operator=(const Any& other);
    Any(Any&& other);
    Any& operator=(Any&& other);


};
}

#endif // ANY_HPP
