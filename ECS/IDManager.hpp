#pragma once
#ifndef __ID_MANAGER__
#define __ID_MANAGER__

struct
{
private:

    template <typename T>
    int* ID ()
    {
        static int id = -1;
        return &id;
    }

public:

    template <typename T>
    int GetUniqueID ()
    {
        return ++*(this->ID<T>());
    }

    template <typename T>
    int GetLastID ()
    {
        return *(this->ID<T>());
    }

} IDManager;

#endif // !__ID_MANAGER__

