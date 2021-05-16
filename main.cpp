#include <cstdio>

FILE* LOG = fopen("debug.log", "w");

#include "ECS/ECS.hpp"
#include <SFML/Graphics.hpp>

class Player: public Entity<Player>
{
    char* name;

public:

    Player (void* args)
    {

    }

};

class HealthComponent: public Component<HealthComponent>
{

public:

    int hp;
    int maxhp;

    HealthComponent(void* args)
    {
        hp = ((int*)args)[0];
        maxhp = ((int*)args)[1];
    }
};

class HPDecreased: public Event<HPDecreased>
{

public:

    int decreasedAmount;
    HealthComponent* health;

    HPDecreased(void* args)
    {
        decreasedAmount = *(int*)args;
        health = *((HealthComponent**)args + 1);
    }
};

class HealthSystem: public System<HealthSystem>, public IEventListener
{

public:

    HealthSystem (void* args)
    {
        entityManager = ((EntityManager**)args)[0];
        eventManager = ((EventManager**)args)[1];
    }


    virtual ~HealthSystem()
    {}

    EntityManager* entityManager;
    EventManager* eventManager;
    
    virtual void Update () override
    {
        if (this->_raisedEvents.size() > 0)
        {
            id_t eventId = this->_raisedEvents[0];
            this->_raisedEvents.pop_back();

            HPDecreased* event = (HPDecreased*) eventManager->GetEvent(eventId);
            event->health->hp -= event->decreasedAmount;
            if (event->health->hp <= 0) //HC died
            {
                entityManager->DestroyEntityObject<Player>(event->health->_owner);
            }
            eventManager->EventHandled(eventId, *this);
        }
    }
};

void test()
{
    EntityManager entityManager;
    ComponentManager componentManager;
    SystemManager systemManager;
    EventManager eventManager;

    int plr = entityManager.CreateEntityObject<Player>(nullptr);

    struct { int hp; int maxhp; } hInit;
    hInit = {100, 100};
    componentManager.AddComponent<HealthComponent>(plr, &hInit);

    struct {EntityManager* _1; EventManager* _2; } hsInit;
    hsInit = {&entityManager, &eventManager};
    HealthSystem* hs = systemManager.AddSystem<HealthSystem>(&hsInit);

    eventManager.Subscribe<HPDecreased>(hs);

    struct {int amount; HealthComponent* hc; } decr;
    decr.amount = 120;
    decr.hc = (HealthComponent*) componentManager.GetComponentsVector(plr)[0];
    eventManager.SendEvent<HPDecreased>(plr, &decr);

    systemManager.Update();

}

void Runtime()
{
    
}

int main()
{
    test();  
    printf("Hello, World!");
    fclose(LOG);
}