#ifndef SmartPointer42_HPP
# define SmartPointer42_HPP
# include <iostream>
# include <string>

template<typename T> class SmartPointer42
{
    T *pointer;
    
    public:
    SmartPointer42() : pointer(NULL) { }
    //constructors and destructors
    explicit SmartPointer42(T * pointer) : pointer(pointer)
    {
        std::cout << "instantiated smart pointer for add " << pointer << std::endl;
    }
    //Do NOT implement the copy constructor as we don't want two SmartPointers that will eventually delete the same instance pointer twice.
    ~SmartPointer42()
    {
        if (pointer != NULL)
        {
            std::cout << "deleting pointer of adress " << pointer << std::endl;
            delete pointer;
        }
    }
    //operator overloads
    /*
        DO NO IMPLEMENT = operator overload!
        We don't want to have two SmartPointer42 instances.
        Because when the second destructor gets called, it will (re)delete the pointer that has already been deleted.
    */
    T operator=(T *new_pointer)
    {
        if (pointer != NULL)
            delete pointer;
        new_pointer = pointer;
    }
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