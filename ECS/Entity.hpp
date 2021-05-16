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
    Entity (void* args);
    virtual ~Entity()
    {}

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
        for (int prev = -1, iter = GetPhysInd(&entityObjectIndexes, 0);
                iter != prev;
                    prev = iter, iter = GetNext(&entityObjectIndexes, iter))
        {
            IEntity* ptr = (IEntity*)GetByPhInd(&entityObjectIndexes, iter);
            fprintf(LOG, "Deleting [%p] from %s\n", ptr, __PRETTY_FUNCTION__);
            delete ptr;
        }
        ListDistruct(&entityObjectIndexes);
    }

    template <typename EntityName>
    entity_id_t CreateEntityObject (void* args)
    {
        EntityName* entity = new EntityName(args);
        fprintf(LOG, "Allocating %lu bytes at [%p] from %s\n", sizeof(*entity), entity, __PRETTY_FUNCTION__);
        entity_id_t Id = AddToEnd(&entityObjectIndexes, entity);
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
        int ret = RemoveAtPos(&entityObjectIndexes, Id, (void**)&entity);
        if (ret >= 0) 
        {
            fprintf(LOG, "Deleting [%p] from %s\n", entity, __PRETTY_FUNCTION__);
            delete entity;
        }
        return ret;
    }

};

#endif // ! __ENTITY_H__

