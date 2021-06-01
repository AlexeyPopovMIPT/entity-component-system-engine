#pragma once
#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include <cassert>

class ISystem
{
    friend class SystemManager;
    //int _updateRound;

public:
    int _priority;
    float _updateInterval;

    virtual ~ISystem()
    {}

    virtual float Update() = 0;

};

template <typename SystemName>
class System : public ISystem
{
    
public:

    virtual ~System()
    {}

    static id_t const SYSTEM_TYPE_ID;

    virtual float Update() override
    { return NAN; }
};

template <typename SystemName>
id_t const System<SystemName>::SYSTEM_TYPE_ID = systemIdManager.GetUniqueID();

inline int cmp (const ISystem* lhs, const ISystem* key, int oldPriority) // lhs < key
{   
    if (oldPriority > -1)
        return key == lhs ? lhs->_priority - oldPriority: key->_priority - lhs->_priority;
    int priorDifference = key->_priority - lhs->_priority;
    return priorDifference != 0 ? priorDifference : (int)(key - lhs);
}

void binsearch (const ISystem * const * arr, int len, const ISystem* key, bool& found, int& index, int oldPriority /*if not found, key should be inserted on index position*/)
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
        const ISystem* md = arr[mid]; 
        int cmp_res = cmp (md, key, oldPriority);
        if (cmp_res < 0)
            end = mid - 1;
        else if (cmp_res > 0)
            start = mid + 1;
        else
        {
            found = true;
            index = mid;
            return;
        }
    }

    found = false;
    index = start;
    return;

    
    /*if (mid > 0 && 
             cmp (arr[mid - 1], key) == 0) { found = true; index = mid - 1; }
    else if (cmp (arr[mid + 0], key) == 0) { found = true; index = mid + 0; }
    else if (mid < len - 1 &&
             cmp (arr[mid + 1], key) == 0) { found = true; index = mid + 1; }
    else
    {
        found = false;
        index = mid - 1 > 0 ? mid - 1 : 0;
        while (index < len && cmp(key, arr[index]) > 0) index++;
    }*/
}


class SystemManager
{
    ISystem** _systemPointers;
    int _systemCount;
    bool _isRunning;
public:
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
                cnt += (bool)(_systemOrder[i] != nullptr);
            }
            if (cnt != _registeredSystems) 
            {
                _LOG("ERROR: cnt = %d != %d = registeredSystems\n", cnt, _registeredSystems);
                return false;
            }
            return true;
        }

    public:

        SystemOrderManager ():
            _maxSystemsCount (0),
            _registeredSystems (0),
            _systemOrder (nullptr)
        {
            
        }

        void Setup (int maxSystemsCount)
        {
            _maxSystemsCount = maxSystemsCount;
            _registeredSystems = 0;
            _systemOrder = new ISystem*[maxSystemsCount];
            LOG_LEEKS _LOG("Allocating %lu bytes at [%p] from %s\n", sizeof(*_systemOrder) * maxSystemsCount, _systemOrder, __PRETTY_FUNCTION__);
            memset(_systemOrder, 0, maxSystemsCount * sizeof(*_systemOrder));
        }

        ~SystemOrderManager()
        {
            if (_systemOrder)
            {
                delete[] _systemOrder;
                LOG_LEEKS _LOG("Deleting [%p] from %s\n", _systemOrder, __PRETTY_FUNCTION__);
            }
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

            binsearch(_systemOrder, _registeredSystems, system, systemRegistered, systemIndex, -1);

            assert (systemRegistered);
            assert(ASSERT_OK());

            return systemIndex;
        }

        void UpdateOrder (ISystem* changedSystem, int oldUpdateRound, int oldPriority)
        {
            assert(ASSERT_OK());
            bool systemRegistered = false;
            int  systemIndex = 0;
            binsearch(_systemOrder, _registeredSystems, changedSystem, systemRegistered, systemIndex, oldPriority);

            if (oldUpdateRound == -1)
            {
                assert(oldUpdateRound == -1);
                memmove (this->_systemOrder + systemIndex + 1,
                         this->_systemOrder + systemIndex + 0,
                        (this->_registeredSystems - systemIndex) * sizeof(*_systemOrder));

                this->_registeredSystems++;

            } 

            else
            {
                systemIndex--; //!
                assert(oldUpdateRound != -1);
                if (systemIndex == oldUpdateRound) // no changes in order
                    return;

                if (systemIndex >  oldUpdateRound)
                {
                    memmove (this->_systemOrder + oldUpdateRound + 0,
                             this->_systemOrder + oldUpdateRound + 1,
                            (systemIndex - oldUpdateRound) * sizeof(*_systemOrder));
                }

                else 
                {
                    memmove (this->_systemOrder + systemIndex + 1,
                             this->_systemOrder + systemIndex + 0,
                            (oldUpdateRound - systemIndex) * sizeof(*_systemOrder));
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
        _systemCount(0),
        systemOrderManager(),
        _isRunning(true),
        _systemPointers (nullptr)
    {}

    void Setup()
    {
        _systemCount = systemIdManager.GetLastID() + 1;
        _systemPointers = new ISystem* [_systemCount];
        LOG_LEEKS _LOG( "Allocating %lu bytes at [%p] from %s\n", sizeof(*_systemPointers) * _systemCount, _systemPointers, __PRETTY_FUNCTION__);
        memset(_systemPointers, 0, _systemCount * sizeof(*_systemPointers));
        systemOrderManager.Setup(_systemCount);

    }

    ~SystemManager()
    {
        for (int i = 0; i < _systemCount; i++)
        {
            LOG_LEEKS _LOG("Deleting [%p] from %s\n", _systemPointers[i], __PRETTY_FUNCTION__);
            delete _systemPointers[i];
        }

        if (_systemPointers)
        {
            LOG_LEEKS _LOG("Deleting [%p] from %s\n", _systemPointers, __PRETTY_FUNCTION__);
            delete[] _systemPointers;
        }
        
    }

    float Update()
    {
        ISystem ** order = systemOrderManager.getSystemOrder();
        float timeToNext_ms = INFINITY;
        for (int i = 0; i < this->_systemCount; i++)
        {
            float timeToNextUpdate_ms = order[i]->Update();
            if (timeToNextUpdate_ms < 1000 * order[i]->_updateInterval)
                timeToNextUpdate_ms = 1000 * order[i]->_updateInterval;

            if (timeToNext_ms > timeToNextUpdate_ms)
                timeToNext_ms = timeToNextUpdate_ms;


        }

        return this->_isRunning ? timeToNext_ms : NAN;
    }

    template <typename SystemName, typename... Args>
    SystemName* AddSystem (Args... args)
    {
        id_t systemTypeID = System<SystemName>::SYSTEM_TYPE_ID;

        if (IsRegistered<SystemName>())
            // system already registered
            return nullptr;

        _systemPointers[systemTypeID] = new SystemName(args...);
        _systemPointers[systemTypeID]->_priority = 0;
        LOG_LEEKS _LOG( "Allocating %lu bytes at [%p] from %s\n", sizeof(*(_systemPointers[systemTypeID])) * 1, _systemPointers[systemTypeID], __PRETTY_FUNCTION__);

        systemOrderManager.UpdateOrder (this->_systemPointers[systemTypeID], -1, -1);

        return (SystemName*) _systemPointers[systemTypeID];

    }
    
    template <typename SystemName>
    void RemoveSystem (void* args)
    {
        id_t systemTypeID = System<SystemName>::SYSTEM_TYPE_ID;
        SystemName* system = _systemPointers[systemTypeID];

        systemOrderManager.RemoveSystem(system);

        LOG_LEEKS _LOG( "Deleting [%p] from %s\n", system, __PRETTY_FUNCTION__);
        delete system;
        _systemPointers[systemTypeID] = nullptr;

    }

    template <typename SystemName>
    void SetPriority (int priority)
    {
        if (!IsRegistered<SystemName>())
            return;

        SystemName* system = dynamic_cast<SystemName*>(this->_systemPointers[System<SystemName>::SYSTEM_TYPE_ID]);

        int oldUpdateRound = this->systemOrderManager.GetUpdateRound(system);

        int oldPriority = system->_priority;
        system->_priority = priority;

        this->systemOrderManager.UpdateOrder (this->_systemPointers[System<SystemName>::SYSTEM_TYPE_ID], oldUpdateRound, oldPriority);

    }

    void Break() { this->_isRunning = false; }


};

SystemManager systemManager;

#endif // ! __SYSTEM_H__

