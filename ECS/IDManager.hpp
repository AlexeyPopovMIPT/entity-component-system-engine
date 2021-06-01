#pragma once
#ifndef __ID_MANAGER__
#define __ID_MANAGER__


class IDManager
{
    int id;

public:

    IDManager():
        id(-1)
    {}

    int GetUniqueID ()
    {
        return ++id;
    }

    int GetLastID ()
    {
        return id;
    }

};

IDManager entityIdManager;
IDManager componentIdManager;
IDManager systemIdManager;
IDManager eventIdManager;

#endif // !__ID_MANAGER__

