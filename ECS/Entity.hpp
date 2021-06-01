#pragma once
#ifndef __ENTITY_H__
#define __ENTITY_H__

class IEntity
{
    entity_id_t _id;

public:

    IEntity()
    {}
    virtual ~IEntity()
    {}

    virtual const entity_t_id_t GetStaticEntityTypeID() const = 0;

    inline const entity_id_t GetEntityID() const
    { 
        return this->_id;
    }

};


template <typename EntityName>
struct Entity : public IEntity
{
    static entity_t_id_t const ENTITY_TYPE_ID;
    Entity ()
    {}

    virtual ~Entity()
    {}

    virtual entity_t_id_t const GetStaticEntityTypeID() const override 
    {
        return ENTITY_TYPE_ID; 
    }

};
template <typename EntityName> 
entity_t_id_t const Entity<EntityName>::ENTITY_TYPE_ID = entityIdManager.GetUniqueID();

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
        for (int prev = -1, iter = GetPhysInd(&entityObjectIndexes, 0);
                iter != prev;
                    prev = iter, iter = GetNext(&entityObjectIndexes, iter))
        {
            IEntity* ptr = (IEntity*)GetByPhInd(&entityObjectIndexes, iter);
            LOG_LEEKS _LOG( "Deleting [%p] from %s\n", ptr, __PRETTY_FUNCTION__);
            delete ptr;
        }
        ListDistruct(&entityObjectIndexes);
    }

    template <typename EntityName, typename... Args> //TODO: не передаются ли аргументы по значению, а не по ссылке
    entity_id_t CreateEntityObject (Args... args)
    {
        EntityName* entity = new EntityName(args...);
        LOG_LEEKS _LOG( "Allocating %lu bytes at [%p] from %s\n", sizeof(*entity), entity, __PRETTY_FUNCTION__);
        _LOG("LIST: adding to ");
        entity_id_t Id = AddToEnd(&entityObjectIndexes, entity);
        _LOG("%d\n", Id);
        return Id;
    }

    void* GetEntityObject(entity_id_t Id)
    {
        return GetByPhInd(&entityObjectIndexes, (size_t)Id);
    }


    int DestroyEntityObject (entity_id_t Id)
    {
        componentManager.RemoveComponentsOf(Id);
        IEntity* entity = nullptr;
        _LOG("LIST: removing from %d\n", Id);
        int ret = RemoveAtPos(&entityObjectIndexes, Id, (void**)&entity);
        if (ret >= 0) 
        {
            LOG_LEEKS _LOG( "Deleting [%p] from %s\n", entity, __PRETTY_FUNCTION__);
            delete entity;
        }
        return ret;
    }

};

EntityManager entityManager; //TODO: namespace

#endif // ! __ENTITY_H__

