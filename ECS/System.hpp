#pragma once
#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include <cassert>

class ISystem
{
    friend class SystemManager;
    int _updateRound;

public:
    int _priority;
    float _updateInterval;

    virtual ~ISystem()
    {}

    virtual void Update() = 0;

};

template <typename SystemName>
class System : public ISystem
{
    
public:

    virtual ~System()
    {}

    static id_t const SYSTEM_TYPE_ID;

    virtual void Update() override
    {}
};

template <typename SystemName>
id_t const System<SystemName>::SYSTEM_TYPE_ID = IDManager.GetUniqueID<ISystem>();

inline int cmp (const ISystem* lhs, const ISystem* rhs)
{   
    return rhs->_priority - lhs->_priority;
}

void binsearch (const ISystem * const * arr, int len, const ISystem* key, bool& found, int& index /*if not found, key should be inserted on index position*/)
{
    if (len == 0)
    {
        found = false;
        index = 0;
        return;
    }

    int start = 0, end = len - 1, mid = 0;
    while (start <= end)
    {
        mid = (start + end) / 2;
        if (cmp (arr[mid], key) > 0)
            end = mid - 1;
        else
            start = mid + 1;
    }
    
    if (mid > 0 && 
             cmp (arr[mid - 1], key) == 0) { found = true; index = mid - 1; }
    else if (cmp (arr[mid + 0], key) == 0) { found = true; index = mid + 0; }
    else if (mid < len - 1 &&
             cmp (arr[mid + 1], key) == 0) { found = true; index = mid + 1; }
    else
    {
        found = false;
        index = mid - 1 > 0 ? mid - 1 : 0;
        while (index < len && cmp(key, arr[index]) > 0) index++;
    }
}


class SystemManager
{
    ISystem** _systemPointers;
    int _systemCount;

    class SystemOrderManager
    {
        int _maxSystemsCount;
        int _registeredSystems;
        ISystem** _systemOrder;

        bool ASSERT_OK()
        {
            int cnt = 0;
            for (int i = 0; i < _maxSystemsCount; i++)
            {
                cnt += bool(_systemOrder[i] != nullptr);
            }
            if (cnt != _registeredSystems) return false;
            return true;
        }

    public:

        SystemOrderManager (int maxSystemsCount)
        {
            _maxSystemsCount = maxSystemsCount;
            _registeredSystems = 0;
            _systemOrder = new ISystem*[maxSystemsCount];
            fprintf(LOG, "Allocating %lu bytes at [%p] from %s\n", sizeof(*_systemOrder) * maxSystemsCount, _systemOrder, __PRETTY_FUNCTION__);
            memset(_systemOrder, 0, maxSystemsCount * sizeof(*_systemOrder));

        }

        ~SystemOrderManager()
        {
            delete[] _systemOrder;
            fprintf(LOG, "Deleting [%p] from %s\n", _systemOrder, __PRETTY_FUNCTION__);
        }


        ISystem ** getSystemOrder()
        {
            return this->_systemOrder;
        }

        int GetUpdateRound (const ISystem* system)
        {
            assert(ASSERT_OK());

            bool systemRegistered = false;
            int  systemIndex = 0;

            binsearch(_systemOrder, _registeredSystems, system, systemRegistered, systemIndex);

            assert (systemRegistered);
            assert(ASSERT_OK());

            return systemIndex;
        }

        void UpdateOrder (ISystem* changedSystem, int oldUpdateRound)
        {
            assert(ASSERT_OK());
            bool systemRegistered = false;
            int  systemIndex = 0;
            binsearch(_systemOrder, _registeredSystems, changedSystem, systemRegistered, systemIndex);

            if (!systemRegistered)
            {
                assert(oldUpdateRound == -1);
                memmove (this->_systemOrder + systemIndex,
                        this->_systemOrder + systemIndex + 1,
                        this->_registeredSystems - systemIndex);

                this->_registeredSystems++;

            } 

            else
            {
                assert(oldUpdateRound != -1);
                if (systemIndex == oldUpdateRound) // no changes in order
                    return;

                if (systemIndex >  oldUpdateRound)
                {
                    memmove (this->_systemOrder + oldUpdateRound + 1,
                            this->_systemOrder + oldUpdateRound,
                            systemIndex - oldUpdateRound);
                }

                else 
                {
                    memmove (this->_systemOrder + systemIndex,
                            this->_systemOrder + systemIndex + 1,
                            oldUpdateRound - systemIndex);
                }

            }

            this->_systemOrder[systemIndex] = changedSystem;
            assert(ASSERT_OK());
            
        }

        void RemoveSystem(const ISystem* system)
        {
            assert(ASSERT_OK());
            int systemOrderIdx = GetUpdateRound(system);
            memmove (this->_systemOrder + systemOrderIdx, 
                     this->_systemOrder + systemOrderIdx + 1, 
                     this->_registeredSystems-- - systemOrderIdx);
            assert(ASSERT_OK());
        }




    } systemOrderManager;


public:

    template <typename SystemName>
    inline bool IsRegistered()
    {
        return this->_systemPointers[System<SystemName>::SYSTEM_TYPE_ID];
    }

    SystemManager():
        _systemCount(IDManager.GetLastID<ISystem>() + 1),
        systemOrderManager(_systemCount)
    {

        _systemPointers = new ISystem* [_systemCount];
        fprintf(LOG, "Allocating %lu bytes at [%p] from %s\n", sizeof(*_systemPointers) * _systemCount, _systemPointers, __PRETTY_FUNCTION__);
        memset(_systemPointers, 0, _systemCount * sizeof(*_systemPointers));

    }

    ~SystemManager()
    {
        for (int i = 0; i < _systemCount; i++)
        {
            fprintf(LOG, "Deleting [%p] from %s\n", _systemPointers[i], __PRETTY_FUNCTION__);
            delete _systemPointers[i];
        }

        fprintf(LOG, "Deleting [%p] from %s\n", _systemPointers, __PRETTY_FUNCTION__);
        delete[] _systemPointers;
    }

    void Update()
    {
        ISystem ** order = systemOrderManager.getSystemOrder();
        for (int i = 0; i < this->_systemCount; i++)
            order[i]->Update();
    }

    template <typename SystemName>
    SystemName* AddSystem (void* args)
    {
        id_t systemTypeID = System<SystemName>::SYSTEM_TYPE_ID;

        if (IsRegistered<SystemName>())
            // system already registered
            return nullptr;

        _systemPointers[systemTypeID] = new SystemName(args);
        fprintf(LOG, "Allocating %lu bytes at [%p] from %s\n", sizeof(*(_systemPointers[systemTypeID])) * 1, _systemPointers[systemTypeID], __PRETTY_FUNCTION__);

        systemOrderManager.UpdateOrder (this->_systemPointers[systemTypeID], -1);

        return (SystemName*) _systemPointers[systemTypeID];

    }
    
    template <typename SystemName>
    void RemoveSystem (void* args)
    {
        id_t systemTypeID = System<SystemName>::SYSTEM_TYPE_ID;
        SystemName* system = _systemPointers[systemTypeID];

        systemOrderManager.RemoveSystem(system);

        fprintf(LOG, "Deleting [%p] from %s\n", system, __PRETTY_FUNCTION__);
        delete system;
        _systemPointers[systemTypeID] = nullptr;

    }

    template <typename SystemName>
    void SetPriority (int priority)
    {
        if (!IsRegistered<SystemName>())
            return;

        SystemName* system = this->_systemPointers[System<SystemName>::SYSTEM_TYPE_ID];

        int oldUpdateRound = this->systemOrderManager.GetUpdateRound(system);
        system->_priority = priority;

        this->systemOrderManager.UpdateOrder (this->_systemPointers[System<SystemName>::SYSTEM_TYPE_ID], oldUpdateRound);

    }


};

#endif // ! __SYSTEM_H__

