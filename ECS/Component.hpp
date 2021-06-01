#pragma once
#ifndef __COMPONENT_H__
#define __COMPONENT_H__
#include <vector>

//TODO: delete components of deleted entity

class IComponent
{

public:

    entity_id_t _owner;

    IComponent()
    {}

    virtual ~IComponent()
    {}

};
template <typename ComponentName>
class Component : public IComponent
{

public:

    static const component_t_id_t COMPONENT_TYPE_ID;

    Component()
    {}

    virtual ~Component()
    {}

};
template <typename ComponentName> 
component_t_id_t const Component<ComponentName>::COMPONENT_TYPE_ID = componentIdManager.GetUniqueID();


class ComponentManager
{

    #define THIS_COMPONENT (this->componentObjectPointersByComponent[Component<ComponentName>::COMPONENT_TYPE_ID][entityId])


    std::vector<std::vector<IComponent*>> componentsOfEntities; // Entity ID to component IDs
    std::vector<std::vector<IComponent*>> componentObjectPointersByComponent; //[componentTypeId][entityObjectId] = pointer to component object of given entity
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

    template <typename T>
    void ExpandVector_initialised (std::vector<T>& thou, int new_size, T init)
    {
        int n = new_size - thou.size();
        while (n > 0)
        {
            thou.push_back(init);
            n--;
        }
    }



public:

    void Setup()
    {
        componentTypesCount = componentIdManager.GetLastID() + 1;

        for (int i = 0; i < componentTypesCount; i++)
            componentObjectPointersByComponent.push_back(std::vector<IComponent*>());
 
    }

    ~ComponentManager()
    {
        for (int i = 0; i < componentTypesCount; i++)
            for (auto component: componentObjectPointersByComponent[i])
            {
                LOG_LEEKS _LOG( "Deleting [%p] from %s\n", component, __PRETTY_FUNCTION__);
                delete component;
            }

    }

    template <typename ComponentName, typename... Args>
    ComponentName*             AddComponent       (entity_id_t entityId, Args... args)
    {
        ComponentName* component = new ComponentName(entityId, args...);
        LOG_LEEKS _LOG( "Allocating %lu bytes at [%p] from %s\n", sizeof(*component), component, __PRETTY_FUNCTION__);

        ExpandVector<std::vector<IComponent*>> (this->componentsOfEntities, entityId + 1);
        this->componentsOfEntities[entityId].push_back(dynamic_cast<IComponent*>(component));

        int componentTypeId = Component<ComponentName>::COMPONENT_TYPE_ID;
        ExpandVector_initialised<IComponent*> ((this->componentObjectPointersByComponent)[componentTypeId], entityId + 1, nullptr);
        this->componentObjectPointersByComponent[Component<ComponentName>::COMPONENT_TYPE_ID][entityId] = dynamic_cast<IComponent*>(component);

        return component;

    }

    template <typename ComponentName>
    void                       RemoveComponent     (entity_id_t entityId)
    {
        ComponentName* component = THIS_COMPONENT;
        delete component;
        LOG_LEEKS _LOG( "Deleting [%p] from %s\n", component, __PRETTY_FUNCTION__);
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
    ComponentName*             GetComponent        (entity_id_t entityId)
    {
        if (entityId == -1) return nullptr;
        return dynamic_cast <ComponentName*> (this->componentObjectPointersByComponent[Component<ComponentName>::COMPONENT_TYPE_ID][entityId]);
    }

    template <typename ComponentName>
    std::vector<IComponent*> const& GetEntitiesVector() const
    {
        return this->componentObjectPointersByComponent[Component<ComponentName>::COMPONENT_TYPE_ID];
    }

    std::vector<IComponent*> const& GetComponentsVector (entity_id_t entityId) const
    {
        return this->componentsOfEntities[entityId];
    }

    void RemoveComponentsOf (entity_id_t entityId)
    {
        this->componentsOfEntities[entityId].clear();

        for (int i = 0; i < this->componentObjectPointersByComponent.size(); i++)
        {
            if (entityId < this->componentObjectPointersByComponent[i].size() && this->componentObjectPointersByComponent[i][entityId]) 
            {
                delete (IComponent*)(this->componentObjectPointersByComponent[i][entityId]);
                this->componentObjectPointersByComponent[i][entityId] = nullptr; //TODO: сокращать вектор
                putchar(0);
            }
        }
    }



};

ComponentManager componentManager;

#endif // ! __COMPONENT_H__

