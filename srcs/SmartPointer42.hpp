#ifndef SmartPointer42_HPP
# define SmartPointer42_HPP
# include <iostream>
# include <string>

template<typename T> class SmartPointer42
{
    T *pointer;
    
    public:
    //constructors and destructors
    SmartPointer42(T * pointer) : pointer(pointer)
    {
        
    }
    SmartPointer42(const SmartPointer42& other)
    {
        *this = other;
    }
    ~SmartPointer42()
    {
        std::cout << "deleting pointer" << std::endl;
        delete pointer;
    }
    //operator overloads
    /*
        DO NO IMPLEMENT = operator overload!
        We don't want to have two SmartPointer42 instances.
        Because when the second destructor gets called, it will (re)delete the pointer that has already been deleted.
    */
    T operator*(void) const
    {
        return *pointer;
    }
    T* operator->() const
    {
        return pointer;
    }
};
#endif