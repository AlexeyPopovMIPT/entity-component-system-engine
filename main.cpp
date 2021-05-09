#include <cstdio>
#include <cstring> //for memset()
#include <vector>
#include <SFML/Graphics.hpp>

// Include doubly linked list library
#define List_elem_t void*
#define NO_GetByPhInd
#include "../lib/The List.h"
List_elem_t GetByPhInd(List_t* thou, size_t ph_ind)
{
    if (ph_ind >= thou->capacity || thou->buffer[ph_ind].prev_item_ind == -1) return NULL;

    return (thou->buffer[ph_ind]).item;
}
#undef NO_GetByPhInd
#undef List_elem_t

typedef int32_t entity_id_t;
typedef int32_t entity_t_id_t;
typedef int32_t component_id_t;
typedef int32_t component_t_id_t;
//typedef int32_t id_t;

struct Vector2
{
    int x;
    int y;

    void Add (Vector2& rhs)
    {
        this->x += rhs.x;
        this->y += rhs.y;
    }
};
class System
{
    virtual void Update();
};







///----------------------------------------------------------------///
///---------------------     Entity part      ---------------------///
class IEntity
{
    entity_id_t _id;

public:

    IEntity();
    virtual ~IEntity();

    virtual const entity_t_id_t GetStaticEntityTypeID() const = 0;

    inline const entity_id_t GetEntityID() const
    { 
        return this->_id;
    }

};


class
{
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


template <typename EntityName>
struct Entity : public IEntity
{
    static entity_t_id_t const ENTITY_TYPE_ID;
    Entity ();
    Entity (void* args);
    virtual ~Entity();

    virtual entity_t_id_t const GetStaticEntityTypeID() const override 
    {
        return ENTITY_TYPE_ID; 
    }

};
template <typename EntityName> 
entity_t_id_t const Entity<EntityName>::ENTITY_TYPE_ID = IDManager.GetUniqueID<IEntity>();

class EntityManager
{
    List entityObjectIndexes;

    inline int GetTotalObjectsCount()
    {
        return GetCount (&this->entityObjectIndexes);
    }

public:

    EntityManager()
    {
        MakeList(&entityObjectIndexes, "EntityManager");
    }

    ~EntityManager()
    {
        ListDistruct(&entityObjectIndexes);
    }

    template <typename EntityName>
    entity_id_t CreateEntityObject (void* args)
    {
        EntityName entity = new EntityName(args);
        entity_id_t Id = AddToEnd(&entityObjectIndexes, &entity);
        return Id;
    }

    void* GetEntityObject(entity_id_t Id)
    {
        return GetByPhInd(&entityObjectIndexes, (size_t)Id);
    }

    template <typename EntityName>
    int DestroyEntityObject (entity_id_t Id)
    {
        EntityName* entity = nullptr;
        int ret = RemoveAtPos(&entityObjectIndexes, Id, &entity);
        if (entity) delete *entity;
        return ret;
    }

};


///-------------------------------------------------------------------------------
///-------------------------          Component          -------------------------
template <typename ComponentName> class Component;
class ComponentManager
{

    #define THIS_COMPONENT this->componentObjectPointersByComponent[Component<ComponentName>::COMPONENT_TYPE_ID][entityId]
    //List componentObjectsPointers; // Component ID to component pointer

    std::vector<std::vector<void*>>  componentsOfEntities; // Entity ID to component IDs
    std::vector<void*>* componentObjectPointersByComponent; //[componentTypeId][entityObjectId] = pointer to component object of given entity
    int componentTypesCount;

    template <typename T>
    void ExpandVector (std::vector<T>& thou, int new_size)
    {
        int n = new_size - thou.size();
        while (n > 0)
        {
            thou.push_back(T());
            n--;
        }
    }

public:

    ComponentManager()
    {
        //MakeList(&componentObjectsPointers, "ComponentManager");

        componentTypesCount = IDManager.GetLastID<IEntity>();

        componentObjectPointersByComponent = new std::vector<void*> [componentTypesCount];
    }

    ~ComponentManager()
    {
        //ListDistruct(&componentObjectsPointers);

        for (int i = 0; i < componentTypesCount; i++)
            for (auto component: componentObjectPointersByComponent[i])
                delete component;


        delete componentObjectPointersByComponent;
    }

    //template <typename ComponentName>
    //std::vector<entity_id_t>* GetEntitiesVector  ()
    //{
        // Добавить типам компонентов идентификаторы и создать vector<vector>
    //    static std::vector<entity_id_t> entities;
    //    return &entities;
    //}

    template <typename ComponentName>
    void                      AddComponent       (entity_id_t entityId, void* args)
    {
        ComponentName* component = new ComponentName(args);
        //component_id_t componentId = AddToEnd(&this->componentObjectsPointers, component);

        ExpandVector<std::vector<void*>>(componentsOfEntities, entityId + 1);
        this->componentsOfEntities[entityId].push_back(component);

        //this->GetEntitiesVector<ComponentName>()->push_back(entityId);
        THIS_COMPONENT = component;

    }

    template <typename ComponentName>
    void                       RemoveComponent     (entity_id_t entityId)
    {
        ComponentName* component = THIS_COMPONENT;
        delete component;
        THIS_COMPONENT = nullptr;
        for (int i = 0; i < this->componentsOfEntities[entityId].size(); i++)
            if (this->componentsOfEntities[entityId][i] == component)
            {
                this->componentsOfEntities[entityId].erase
                    (this->componentsOfEntities[entityId].begin() + i);
                
                return;
            }

        printf("ALARM ALARM line %d\n", __LINE__);
    }

    template <typename ComponentName>
    std::vector<void*> const& GetEntitiesVector() const
    {
        return this->componentObjectPointersByComponent[Component<ComponentName>::COMPONENT_TYPE_ID];
    }

    std::vector<void*> const& GetComponentsVector (entity_id_t entityId) const
    {
        return this->componentsOfEntities[entityId];
    }



};

class IComponent
{
    entity_id_t entity;

};
template <typename ComponentName>
class Component
{

public:

    static const component_t_id_t COMPONENT_TYPE_ID;

};
template <typename ComponentName> 
component_t_id_t const Component<ComponentName>::COMPONENT_TYPE_ID = IDManager.GetUniqueID<IComponent>();


///----------------------------------------------------------------------------------
///-----------------------------         Event          -----------------------------

class IEvent
{
    friend class EventManager;

    int unhandlingsCount;

public:

    entity_id_t _entityId;

    IEvent(entity_id_t entityId) :

        _entityId(entityId)
        {}

    virtual ~IEvent();

};

template <typename EventName>
class Event : public IEvent
{

public:

    static const id_t EVENT_TYPE_ID;

    virtual ~Event();

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
        //eventObjectsByTypes = new std::vector<IEvent*> [IDManager.GetLastID<IEvent>() + 1];
        _eventListenersByTypes = new std::vector<IEventListener*> [IDManager.GetLastID<IEvent>() + 1];
    }

    ~EventManager()
    {
        for (int prev = -1, iter = GetPhysInd(&_eventPointers, 0);
                iter != prev;
                    prev = iter, iter = GetNext(&_eventPointers, iter))

                    delete (IEvent*)GetByPhInd(&_eventPointers, iter);

                    
        ListDistruct(&_eventPointers);
    }

    template <typename EventName>
    void Subscribe (const IEventListener* eventListener)
    {
        this->_eventListenersByTypes[Event<EventName>::EVENT_TYPE_ID].push_back(eventListener);
    }

    template <typename EventName>
    void Unsubscribe (const IEventListener* eventListener);

    template <typename EventName>
    id_t SendEvent (entity_id_t entityId, void* args)
    {
        Event<EventName>* event = new Event<EventName>(args);
        id_t eventId = AddToEnd(&_eventPointers, event);

        event->unhandlingsCount = this->_eventListenersByTypes[Event<EventName>::EVENT_TYPE_ID].size();
        for (IEventListener* listener : this->_eventListenersByTypes[Event<EventName>::EVENT_TYPE_ID])
            listener->_raisedEvents.push_back(eventId);

    }

    void EventHandled (id_t eventId, const IEventListener& eventListener)
    {
        IEvent* event = (IEvent*)GetByPhInd(&this->_eventPointers, eventId);
        event->unhandlingsCount--;
        if (event->unhandlingsCount == 0)
        {
            delete event;
            RemoveAtPos(&this->_eventPointers, eventId, nullptr);
        }
    }

};

int main()
{
    
}
