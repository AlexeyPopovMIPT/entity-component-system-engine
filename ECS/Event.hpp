#pragma once
#ifndef __EVENT_H__
#define __EVENT_H__

#include <vector>
#include "../../lib/The List.h"

class IEvent
{
    friend class EventManager;

    int unhandlingsCount;

public:

    id_t _eventTypeId;

    IEvent ()
    {}

    virtual ~IEvent()
    {}

};

template <typename EventName>
class Event : public IEvent
{

public:

    static const id_t EVENT_TYPE_ID;

    Event()
    {
        _eventTypeId = EVENT_TYPE_ID;
    }

    virtual ~Event()
    {}

};
template <typename EventName>
id_t const Event<EventName>::EVENT_TYPE_ID = eventIdManager.GetUniqueID();


class IEventListener
{

public:

    std::vector<id_t> _raisedEvents;

    ~IEventListener()
    {
        _LOG("Deleting IEventListener [%p]\n", this);
        if (_raisedEvents.size() > 0)
            _LOG("ACHTUNG: %d event(s) haven't been handled\n", _raisedEvents.size());
    }

};
//TODO: void* -> interface*, виртуальные деструкторы для корректности удаления объектов, виртуальные другие функции
class EventManager
{
    List _eventPointers;
    //std::vector<IEvent*>* eventObjectsByTypes;
    std::vector<IEventListener*>* _eventListenersByTypes;

public:

    EventManager():
        _eventListenersByTypes (nullptr)
    {
        MakeList(&_eventPointers, "EventManager");
    }

    void Setup()
    {
        _eventListenersByTypes = new std::vector<IEventListener*> [eventIdManager.GetLastID() + 1];
        LOG_LEEKS _LOG("Allocating %lu bytes at [%p] from %s\n", sizeof(*_eventListenersByTypes) * (eventIdManager.GetLastID() + 1), _eventListenersByTypes, __PRETTY_FUNCTION__);
    }

    ~EventManager()
    {
        for (int prev = -1, iter = GetPhysInd(&_eventPointers, 0);
                iter != prev;
                    prev = iter, iter = GetNext(&_eventPointers, iter))
        {
            IEvent* ptr = (IEvent*)GetByPhInd(&_eventPointers, iter);
            LOG_LEEKS _LOG("Deleting [%p] from %s\n", ptr, __PRETTY_FUNCTION__);
            delete ptr;
        }


        ListDistruct(&_eventPointers);
        if (_eventListenersByTypes)
        {
            LOG_LEEKS _LOG("Deleting [%p] from %s\n", _eventListenersByTypes, __PRETTY_FUNCTION__);
            delete[] _eventListenersByTypes;
        }
    }

    IEvent* GetEvent(id_t id)
    {
        return (IEvent*) GetByPhInd(&this->_eventPointers, id);
    }

    template <typename EventName>
    void Subscribe (IEventListener* eventListener)
    {
        this->_eventListenersByTypes[Event<EventName>::EVENT_TYPE_ID].push_back(eventListener);
    }

    template <typename EventName>
    void Unsubscribe (const IEventListener* eventListener);

    template <typename EventName, typename... Args>
    id_t SendEvent (/*entity_id_t entityId, */Args... args)
    {
        EventName* event = new EventName(args...);
        LOG_LEEKS _LOG( "Allocating %lu bytes at [%p] from %s\n", sizeof(*event) * 1, event, __PRETTY_FUNCTION__);
        id_t eventId = AddToEnd(&_eventPointers, event);

        event->unhandlingsCount = this->_eventListenersByTypes[Event<EventName>::EVENT_TYPE_ID].size();
        for (IEventListener* listener : this->_eventListenersByTypes[Event<EventName>::EVENT_TYPE_ID])
            listener->_raisedEvents.push_back(eventId);

        return eventId;

    }

    void EventHandled (id_t eventId, const IEventListener& eventListener)
    {
        IEvent* event = (IEvent*)GetByPhInd(&this->_eventPointers, eventId);
        event->unhandlingsCount--;
        if (event->unhandlingsCount == 0)
        {
            LOG_LEEKS _LOG( "Deleting [%p] from %s\n", event, __PRETTY_FUNCTION__);
            delete event;
            RemoveAtPos(&this->_eventPointers, eventId, nullptr);
        }
    }

};

EventManager eventManager;

#endif // ! __EVENT_H__

