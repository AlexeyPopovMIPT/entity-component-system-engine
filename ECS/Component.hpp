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
component_t_id_t const Component<ComponentName>::COMPONENT_TYPE_ID = IDManager.GetUniqueID<IComponent>();


class ComponentManager
{

    #define THIS_COMPONENT this->componentObjectPointersByComponent[Component<ComponentName>::COMPONENT_TYPE_ID][entityId]


    std::vector<std::vector<void*>>  componentsOfEntities; // Entity ID to component IDs
    std::vector<std::vector<void*>> componentObjectPointersByComponent; //[componentTypeId][entityObjectId] = pointer to component object of given entity
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

    ComponentManager()
    {
        componentTypesCount = IDManager.GetLastID<IComponent>() + 1;

        for (int i = 0; i < componentTypesCount; i++)
            componentObjectPointersByComponent.push_back(std::vector<void*>());
 
    }

    ~ComponentManager()
    {
        for (int i = 0; i < componentTypesCount; i++)
            for (auto component: componentObjectPointersByComponent[i])
            {
                fprintf(LOG, "Deleting [%p] from %s\n", component, __PRETTY_FUNCTION__);
                delete (IComponent*)component;
            }

    }

    template <typename ComponentName>
    void                      AddComponent       (entity_id_t entityId, void* args)
    {
        ComponentName* component = new ComponentName(args);
        fprintf(LOG, "Allocating %lu bytes at [%p] from %s\n", sizeof(*component), component, __PRETTY_FUNCTION__);

        ExpandVector<std::vector<void*>> (this->componentsOfEntities, entityId + 1);
        this->componentsOfEntities[entityId].push_back(component);

        int componentTypeId = Component<ComponentName>::COMPONENT_TYPE_ID;
        ExpandVector_initialised<void*> ((this->componentObjectPointersByComponent)[componentTypeId], entityId + 1, nullptr);
        this->componentObjectPointersByComponent[Component<ComponentName>::COMPONENT_TYPE_ID][entityId] = component;

    }

    template <typename ComponentName>
    void                       RemoveComponent     (entity_id_t entityId)
    {
        ComponentName* component = THIS_COMPONENT;
        delete component;
        fprintf(LOG, "Deleting [%p] from %s\n", component, __PRETTY_FUNCTION__);
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

#endif // ! __COMPONENT_H__

