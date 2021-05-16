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

    entity_id_t _entityId;

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
    {}

    virtual ~Event()
    {}

};
template <typename EventName>
id_t const Event<EventName>::EVENT_TYPE_ID = IDManager.GetUniqueID<IEvent>();


class IEventListener
{

public:

    std::vector<id_t> _raisedEvents;

};
//TODO: void* -> interface*, виртуальные деструкторы для корректности удаления объектов, виртуальные другие функции
class EventManager
{
    List _eventPointers;
    //std::vector<IEvent*>* eventObjectsByTypes;
    std::vector<IEventListener*>* _eventListenersByTypes;

public:

    EventManager()
    {
        MakeList(&_eventPointers, "EventManager");
        _eventListenersByTypes = new std::vector<IEventListener*> [IDManager.GetLastID<IEvent>() + 1];
        fprintf(LOG, "Allocating %lu bytes at [%p] from %s\n", sizeof(*_eventListenersByTypes) * (IDManager.GetLastID<IEvent>() + 1), _eventListenersByTypes, __PRETTY_FUNCTION__);
    }

    ~EventManager()
    {
        for (int prev = -1, iter = GetPhysInd(&_eventPointers, 0);
                iter != prev;
                    prev = iter, iter = GetNext(&_eventPointers, iter))
        {
                    IEvent* ptr = (IEvent*)GetByPhInd(&_eventPointers, iter);
                    fprintf(LOG, "Deleting [%p] from %s\n", ptr, __PRETTY_FUNCTION__);
                    delete ptr;
        }


        ListDistruct(&_eventPointers);
        fprintf(LOG, "Deleting [%p] from %s\n", _eventListenersByTypes, __PRETTY_FUNCTION__);
        delete[] _eventListenersByTypes;
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

    template <typename EventName>
    id_t SendEvent (entity_id_t entityId, void* args)
    {
        EventName* event = new EventName(args);
        fprintf(LOG, "Allocating %lu bytes at [%p] from %s\n", sizeof(*event) * 1, event, __PRETTY_FUNCTION__);
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
            fprintf(LOG, "Deleting [%p] from %s\n", event, __PRETTY_FUNCTION__);
            delete event;
            RemoveAtPos(&this->_eventPointers, eventId, nullptr);
        }
    }

};

#endif // ! __EVENT_H__

