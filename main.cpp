#include <cstdio>
#include <ctime>
#include <deque>
#include <random>
#include <chrono>
#include <cmath>
#include <unistd.h>
#include <cassert>

//#define DEBUG

#ifdef DEBUG
#define _LOG(...) { FILE* LOG = fopen("debug.log", "a"); fprintf(LOG, __VA_ARGS__); fclose(LOG); }
void CleanLogFile()
{
    fclose(fopen("debug.log", "w"));
}
#define LOG_LEEKS if(1)

#else
#define _LOG(...) ;
void CleanLogFile()
{}
#define LOG_LEEKS if(0)
#endif

#define CLOCK CLOCK_REALTIME

#include "ECS/ECS.hpp"
#include <SFML/Main.hpp>
#include <SFML/Graphics.hpp>

const int WINDOW_X = 1600;
const int WINDOW_Y = 900;
const int LOW_WALL_Y  = WINDOW_Y - 200;
const int HIGH_WALL_Y = 200;
const int PLAYER_SPEED = 200; // pics/sec
const float SHOOTING_SPEED = 3; //per second
const int X_DECREASING = 0x30000000;
const int CHUNK_SIZE = 10*30;
const int MAX_CANNONBALLS_COLLISIONS = 2;
const int FPS = 30;
const float FRAMERATE = 1.f / FPS;
const float CANNONBALL_SPEED = PLAYER_SPEED * 1.1;
const float FLOAT_PRECISION = 1e-4;


int64_t GetTimeBetween_nsec (const struct timespec& t1, const struct timespec& t2)
{
    return (t2.tv_sec - t1.tv_sec) * 1e9 + (t2.tv_nsec - t1.tv_nsec);
}

struct Cannon: public Entity<Cannon>
{
    static const int MIN_ANGLE = 90 - 35;
    static const int MAX_ANGLE = 90 + 35;
};



class Flamethrower: public Entity<Flamethrower>
{};

class Player: public Entity<Player>
{};

struct Cannonball: public Entity<Cannonball>
{};







class PositionComponent: public Component<PositionComponent>
{ 
    sf::Vector2f _position;
    
public:

    sf::Vector2f& getPosition() { return this->_position; }
    void setPosition(float x, float y)
    {
        this->_position.x = x;
        this->_position.y = y;
    }

    PositionComponent(entity_id_t owner, float x, float y):
        _position(x, y) 
    {
        _owner = owner;
    }

    PositionComponent (entity_id_t owner, sf::Vector2f& xy):
        _position(xy)
    {
        _owner = owner;
    }

    ~PositionComponent() {}
};

class OrientationComponent: public Component<OrientationComponent>
{ public:
    int _angle;

    OrientationComponent (entity_id_t owner, int angle):
        _angle(angle)
    {
        _owner = owner;
    }

    ~OrientationComponent() {}
};

class DrawingComponent: public Component<DrawingComponent>
{ public:
    PositionComponent* _position;

    sf::Texture* _texture;
    sf::Sprite _sprite;
    
    OrientationComponent* _orientation;

    DrawingComponent (entity_id_t owner, const char* filename, PositionComponent* position, OrientationComponent* orientation):
    _sprite(),
    _position(position),
    _orientation(orientation)
    {
        _owner = owner;
        _texture = new sf::Texture();
        _texture->loadFromFile(filename);
        _sprite.setTexture(*_texture);
        _sprite.setPosition(_position->getPosition());
        _sprite.setRotation(_orientation->_angle);
    }

    DrawingComponent (entity_id_t owner, sf::Texture* texture, PositionComponent* position, OrientationComponent* orientation):
    _texture (nullptr),
    _sprite (),
    _position (position),
    _orientation (orientation)
    {
        _owner = owner;
        _sprite.setTexture(*texture);
        _sprite.setPosition(_position->getPosition());
        _sprite.setRotation(_orientation->_angle);
    }

    ~DrawingComponent()
    {
        if (_texture) delete _texture;
    }

};

class MovingComponent: public Component<MovingComponent>
{ public:
    sf::Vector2f _speed;
    PositionComponent* _positionComponent;

    MovingComponent (entity_id_t owner, float x, float y, PositionComponent* positionComponent):
        _speed(x, y),
        _positionComponent(positionComponent)
    {
        _owner = owner;
    }


};

struct BouncingComponent: public Component<BouncingComponent>
{
    int _collisionsCount;
    float _bouncing;        //коэффициент потери скорости
    int _maxCollisions;

    BouncingComponent (entity_id_t owner, float bouncing, int maxCollisions):
        _collisionsCount(0),
        _bouncing(bouncing),
        _maxCollisions(maxCollisions)
    {
        _owner = owner;
    }
};

struct CollideableComponent: public Component<CollideableComponent>
{
    sf::Vector2f _widthPos;
    sf::Vector2f _widthNeg;

    CollideableComponent (entity_id_t owner, float xpos, float xneg, float ypos, float yneg):
        _widthPos(xpos, ypos),
        _widthNeg(xneg, yneg)
    {
        _owner = owner;
    }

    bool DoesCollideWith (float y, float y0)
    {
        return y > y0 - this->_widthNeg.y && y < y0 + this->_widthPos.y; 
    }

    bool DoesCollideWith_30x30 (float x, float y, float x0, float y0)
    {
        return (x - x0 < 30 && x0 - x < 30) && (y - y0 < 30 && y0 - y < 30);
    }

    bool DoesCollideWith (CollideableComponent& rhs, float x, float y, float x0, float y0)
    {
        float left_up_x = x - rhs._widthNeg.x,
              left_dn_x = x + rhs._widthPos.x,
              left_up_y = y - rhs._widthNeg.y,
              left_dn_y = y + rhs._widthPos.y,
              left_up_x0 = x0 - this->_widthNeg.x,
              left_dn_x0 = x0 + this->_widthPos.x,
              left_up_y0 = y0 - this->_widthNeg.y,
              left_dn_y0 = y0 + this->_widthPos.y;
        
        return     (x0 + this->_widthPos.x > x - rhs._widthNeg.x || x + rhs._widthPos.x > x0 - this->_widthNeg.x)
                && (y0 + this->_widthPos.y > y - rhs._widthNeg.y || y + rhs._widthPos.y > y0 - this->_widthNeg.y);
    }
};

struct DeadlyComponent: public Component<DeadlyComponent>
{
    DeadlyComponent (entity_id_t owner)
    {
        _owner = owner;
    }
};

class ShootingComponent: public Component<ShootingComponent>
{ 
    float _orientationCos;
    float _orientationSin;
    
    public:

    PositionComponent* _position;
    OrientationComponent* _orientation;
    struct timespec _timeOfLastShot;

    ShootingComponent (entity_id_t owner, PositionComponent* position, OrientationComponent* orientation):
        _position (position),
        _orientation (orientation),
        _timeOfLastShot({})
    {
        _owner = owner;
        _orientationCos = -cos((orientation->_angle + 90) * M_PI / 180);
        _orientationSin = -sin((orientation->_angle + 90) * M_PI / 180);
    }

    void Shoot (struct timespec shootTime, sf::Texture* cannonballTexture)
    {
        entity_id_t cannonball = entityManager.CreateEntityObject<Cannonball>();

        auto position =  _orientationSin < 0 ? 
            componentManager.AddComponent<PositionComponent>(cannonball, _position->getPosition().x - 15, _position->getPosition().y - 45)
          : componentManager.AddComponent<PositionComponent>(cannonball, _position->getPosition().x - 15, _position->getPosition().y + 15);

        auto orientation = componentManager.AddComponent<OrientationComponent>(cannonball, 0);
        componentManager.AddComponent<DrawingComponent>(cannonball, cannonballTexture, position, orientation);
        componentManager.AddComponent<MovingComponent>(cannonball, _orientationCos * CANNONBALL_SPEED, _orientationSin * CANNONBALL_SPEED, position);
        componentManager.AddComponent<BouncingComponent>(cannonball, 1.f, MAX_CANNONBALLS_COLLISIONS);
        componentManager.AddComponent<CollideableComponent>(cannonball, 30.f, 0.f, 30.f, 0.f);
        componentManager.AddComponent<DeadlyComponent>(cannonball);

        _timeOfLastShot = shootTime;
    }
};

struct HealthComponent:  public Component<HealthComponent>
{
    int _hp;
    //int _invincibilityTime;
    //int _lastTimeHurt;

    HealthComponent (entity_id_t owner, int hp)://, int invincibilityTime):
        _hp(hp)//,
        //_invincibilityTime(invincibilityTime)
    {
        _owner = owner;
    }
};

///**************************************************************************************************
//   Events   ***************************************************************************************

struct MovementKeyDown: public Event<MovementKeyDown>
{
    int _wasd;

    MovementKeyDown (int wasd):
        _wasd(wasd)
    {}

    ~MovementKeyDown() {}
};

struct MovementKeyUp: public Event<MovementKeyUp>
{
    int _wasd;

    MovementKeyUp (int wasd):
        _wasd (wasd)
    {}

    ~MovementKeyUp() {}
};

struct EnterPressed: public Event<EnterPressed> {};
struct PausedOrResumed: public Event<PausedOrResumed> {};
struct GameStarted: public Event<GameStarted> {};
struct PlayerPassedChunk: public Event<PlayerPassedChunk> {};
struct XReducing: public Event<XReducing> {};
struct XReduced: public Event<XReduced> {};
struct GameOver: public Event<GameOver> {};
struct ExitGame: public Event<ExitGame> {};

struct EntityHurt: public Event<EntityHurt>
{};

struct PlayerDied: public Event<PlayerDied> 
{
    entity_id_t _entityId;

    PlayerDied (entity_id_t entityId):
        _entityId(entityId)
    {}
};

struct PlayerSpawned: public Event<PlayerSpawned> 
{
    entity_id_t playerId;
    PlayerSpawned (entity_id_t _):
        playerId(_)
    {}
};

struct WallCollision: public Event<WallCollision>
{
    entity_id_t _entityId;

    WallCollision (int entityId):
        _entityId(entityId)
    {}
}; 

///**************************************************************************************************
//   Systems   **************************************************************************************

class DrivingSystem: public System <DrivingSystem>, public IEventListener 
{                                                          //MovementKeyDown, MovementKeyUp, PlayerSpawned, PlayerDied GameOver
    entity_id_t _driveableId;
    bool _wasdDown[4];
    const int _w = 0;
    const int _a = 1;
    const int _s = 2;
    const int _d = 3;

    inline bool DriveableEntityPresent()
    {
        return _driveableId != -1;
    }

public:

    DrivingSystem ():
        _driveableId (-1)
    {
        _updateInterval = FRAMERATE;
        _wasdDown[0] = false;
        _wasdDown[1] = false;
        _wasdDown[2] = false;
        _wasdDown[3] = false;
    }

    void UpdateSpeed()
    {
        int speed_x = -_wasdDown[_a] + _wasdDown[_d];
        int speed_y = -_wasdDown[_w] + _wasdDown[_s];
        MovingComponent* moving = componentManager.GetComponent<MovingComponent>(_driveableId);
        //TODO: MovingComponent-ы в DrivingSystem полем
        moving->_speed.x = speed_x * PLAYER_SPEED;
        moving->_speed.y = speed_y * PLAYER_SPEED;
    }

    virtual float Update() override
    {
        for (int i = 0; i < this->_raisedEvents.size(); i++)
        {
            id_t eventId = this->_raisedEvents[i];

            IEvent* event = eventManager.GetEvent(eventId);
            
            if (event->_eventTypeId == Event<MovementKeyDown>::EVENT_TYPE_ID)
            {
                if (DriveableEntityPresent())
                {
                    MovementKeyDown* mkDownEvent = dynamic_cast <MovementKeyDown*> (event);
                    int wasd = mkDownEvent->_wasd;
                    assert (0 <= wasd && wasd < 4);
                    _wasdDown[wasd] = true;
                }
            }

            else if (event->_eventTypeId == Event<MovementKeyUp>::EVENT_TYPE_ID)
            {
                if (DriveableEntityPresent())
                {
                    MovementKeyUp* mkDownEvent = dynamic_cast <MovementKeyUp*> (event);
                    int wasd = mkDownEvent->_wasd;
                    assert (0 <= wasd && wasd < 4);
                    _wasdDown[wasd] = false;
                }
            }

            else if (event->_eventTypeId == Event<PlayerSpawned>::EVENT_TYPE_ID)
            {
                _driveableId = (dynamic_cast <PlayerSpawned*>(event))->playerId;
                _LOG("DrivingSystem: player set: id %d\n", _driveableId);
            }

            else if (event->_eventTypeId == Event<GameOver>::EVENT_TYPE_ID)
            {
                _driveableId = -1;
                _wasdDown[0] = false; _wasdDown[1] = false; 
                _wasdDown[2] = false; _wasdDown[3] = false; 
                _LOG("DrivingSystem: player died catched\n");
            }

            else
                _LOG( "ERROR: incorrect event type in DrivingSystem: %d\n", event->_eventTypeId);

            eventManager.EventHandled(eventId, *this);
        }

        if (DriveableEntityPresent()) UpdateSpeed();

        this->_raisedEvents.clear();

        return 0;

    }
};

class MovingSystem: public System<MovingSystem>, public IEventListener //PlayerSpawned, PlayerDied GameOver
{
    struct timespec _timeOfLastUpdate;
    int _playerId;

    void CheckEvents()
    {
        int size = this->_raisedEvents.size();
        for (int i = 0; i < size; i++)
        {
            id_t eventId = this->_raisedEvents[i];
            IEvent* event = eventManager.GetEvent(eventId);
            if (event->_eventTypeId == Event<PlayerSpawned>::EVENT_TYPE_ID)
            {
                _playerId = (dynamic_cast<PlayerSpawned*>(event))->playerId;
                _LOG("MovingSystem: player set as %d\n", _playerId);
            }
            else if (event->_eventTypeId == Event<GameOver>::EVENT_TYPE_ID)
            {
                _playerId = -1;
            }
            else
            {
                _LOG("Unknown event type in MovingSystem: %d\n", event->_eventTypeId);
            }

            eventManager.EventHandled(eventId, *this);

        }
        this->_raisedEvents.clear();
    }

    float GetTimeSinceLastUpdate (struct timespec& currentTime)
    {
        float ret = 1e-9 * (currentTime.tv_nsec - _timeOfLastUpdate.tv_nsec);
        if (ret < 0) return ret + 1;
        return ret;
    }

public:

    MovingSystem():
        _timeOfLastUpdate({}),
        _playerId(-1)
    {
        _updateInterval = FRAMERATE;
        clock_gettime(CLOCK, &_timeOfLastUpdate);
    }

    virtual ~MovingSystem()
    {}

    virtual float Update() override
    {
        CheckEvents();

        struct timespec currentTime = {};
        clock_gettime(CLOCK, &currentTime);

        float timeSinceLastUpdate = GetTimeSinceLastUpdate(currentTime);
                                                                   //TODO: rename function
        std::vector<IComponent*> movingComponents = componentManager.GetEntitiesVector<MovingComponent>(),
                            bouncingComponents    = componentManager.GetEntitiesVector<BouncingComponent>(),
                            deadlyComponents      = componentManager.GetEntitiesVector<DeadlyComponent>(),
                            collideableComponents = componentManager.GetEntitiesVector<CollideableComponent>(),
                            healthComponents      = componentManager.GetEntitiesVector<HealthComponent>();      //only 1, player's


        float playerOldX = _playerId == -1 ? 0.f : componentManager.GetComponent<PositionComponent>(_playerId)->getPosition().x;

        for (int i = 0; i < movingComponents.size(); i++)
        {
            MovingComponent* movingComponent = dynamic_cast<MovingComponent*>(movingComponents[i]);
            if (movingComponents[i] == nullptr) continue;

            // Update position
            PositionComponent* positionComponent = movingComponent->_positionComponent;
            float old_x = positionComponent->getPosition().x,
                  old_y = positionComponent->getPosition().y,
                  new_x = old_x + timeSinceLastUpdate * (movingComponent)->_speed.x,
                  new_y = old_y + timeSinceLastUpdate * (movingComponent)->_speed.y;

            //positionComponent->_position.x += new_x - old_x;
            //positionComponent->_position.y += new_y - old_y;
            positionComponent->setPosition (positionComponent->getPosition().x + new_x - old_x, positionComponent->getPosition().y + new_y - old_y);


            // обработка возможного столкновения со стеной
            BouncingComponent* bouncingComponent = dynamic_cast<BouncingComponent*>(bouncingComponents[i]);
            CollideableComponent* collideableComponent = dynamic_cast<CollideableComponent*>(collideableComponents[i]);

            if (bouncingComponent)
            {
                if (collideableComponent->DoesCollideWith(LOW_WALL_Y,  new_y) || LOW_WALL_Y  < new_y
                ||  collideableComponent->DoesCollideWith(HIGH_WALL_Y, new_y) || HIGH_WALL_Y > new_y)
                {
                    _LOG("EVENT: Wall collision: entity %d (%f, %f)<-(%f,%f)\n", i, new_x, new_y, old_x, old_y);
                    eventManager.SendEvent<WallCollision>(i);
                }

            }


            //обработка возможного столкновения с игроком

            DeadlyComponent* deadlyComponent = i < deadlyComponents.size() && deadlyComponents[i] ? dynamic_cast<DeadlyComponent*>(deadlyComponents[i]) : nullptr;
            if (deadlyComponent)
            {
                entity_id_t thisEntity = _playerId;

                CollideableComponent* thisCollideableComponent = dynamic_cast<CollideableComponent*>(collideableComponents[thisEntity]);
                PositionComponent* thisPositionComponent = (dynamic_cast<MovingComponent*>(movingComponents[thisEntity]))->_positionComponent;

                if (thisCollideableComponent->DoesCollideWith_30x30 (positionComponent->getPosition().x, //TODO: DoesCollide принимает ID сущностей
                    positionComponent->getPosition().y, thisPositionComponent->getPosition().x, thisPositionComponent->getPosition().y))
                {
                    eventManager.SendEvent<EntityHurt>();
                    _LOG("EVENT: Cannonball %d (%f,%f) collided w/ player (%f,%f)\n", i,  positionComponent->getPosition().x, positionComponent->getPosition().y,
                    thisPositionComponent->getPosition().x, thisPositionComponent->getPosition().y);
                    _LOG("Destroying called from line %d\n", __LINE__);
                    entityManager.DestroyEntityObject(i);
                }

            }

            //if have bouncing and bumped on wall, distruct

        }

        this->_timeOfLastUpdate = currentTime;

        if (_playerId != -1)
        {
            float playerNewX = componentManager.GetComponent<PositionComponent>(_playerId)->getPosition().x;
            if ((int)(playerOldX / CHUNK_SIZE) < (int)(playerNewX / CHUNK_SIZE))
            {
                eventManager.SendEvent<PlayerPassedChunk>();

                if (playerNewX > 2 * X_DECREASING)
                    eventManager.SendEvent<XReducing>();

            }
        }

        return 0;
    }




};

class WallCollisionSystem: public System<WallCollisionSystem>, public IEventListener //WallCollision
{ 

    bool IncremCollisions_DestroyIfNeeded (BouncingComponent& bouncing)
    {
        if ((bouncing._collisionsCount)++ >= bouncing._maxCollisions && bouncing._maxCollisions > 0)
        {
            _LOG("Cannonball %d reached %d collisions, destroy\n", bouncing._owner, bouncing._collisionsCount);
            _LOG("Destroying called from line %d\n", __LINE__);
            entityManager.DestroyEntityObject(bouncing._owner);
            return true;
        }

        return false;
    } 

    
    
public:

    WallCollisionSystem()
    {
        _updateInterval = FRAMERATE;
    }

    virtual ~WallCollisionSystem() {}

    virtual float Update() override
    {
        int size = this->_raisedEvents.size();
        id_t eventId = -1;
        for (; size > 0; eventManager.EventHandled(eventId, *this))
        {
            eventId = this->_raisedEvents[--size];
            this->_raisedEvents.pop_back();

            WallCollision* event = dynamic_cast<WallCollision*> (eventManager.GetEvent(eventId)); //TODO: почему не давать в стек сразу указатель?

            entity_id_t entityId = event->_entityId;


            PositionComponent* position       = componentManager.GetComponent<PositionComponent>(entityId);
            CollideableComponent* collideable = componentManager.GetComponent<CollideableComponent>(entityId);
            BouncingComponent* bouncing       = componentManager.GetComponent<BouncingComponent>(entityId);

            if(!(position && collideable && bouncing))
                continue;                           // because this entity was already destroyed

            
            while (true)
            {
                float distanceLowWall = LOW_WALL_Y - (position->getPosition().y + collideable->_widthPos.y);

                if (distanceLowWall < - FLOAT_PRECISION)
                {
                    if (IncremCollisions_DestroyIfNeeded(*bouncing)) 
                        break;
                    else
                    {
                        position->setPosition (position->getPosition().x, position->getPosition().y + distanceLowWall * (1 + bouncing->_bouncing));
                        MovingComponent* moving = componentManager.GetComponent<MovingComponent>(entityId);
                        //moving->_speed.x *= bouncing->_bouncing; TODO: add horizontal bouncing
                        moving->_speed.y *= -bouncing->_bouncing;
                    }
                }

                else
                {
                    float distanceHighWall = HIGH_WALL_Y - (position->getPosition().y - collideable->_widthNeg.y);

                    if (distanceHighWall > FLOAT_PRECISION)
                    {
                        if (IncremCollisions_DestroyIfNeeded(*bouncing)) 
                            break;
                        else
                        {
                            position->setPosition (position->getPosition().x, position->getPosition().y + distanceHighWall * (1 + bouncing->_bouncing));
                            MovingComponent* moving = componentManager.GetComponent<MovingComponent>(entityId);
                            //moving->_speed.x *= bouncing->_bouncing;
                            moving->_speed.y *= -bouncing->_bouncing;
                        }
                    }

                    else
                        break;
                }

                
            }

        }

        return 0;

    }
};

class HealthSystem: public System<HealthSystem>, public IEventListener //EntityHurt, GameStarted, GameOver
{
    entity_id_t playerId;

    void HandleEntityHurt (EntityHurt* event)
    {

        HealthComponent* health = componentManager.GetComponent<HealthComponent>(playerId);

        health->_hp -= 1;
        _LOG("Player hurt: hp is %d\n", health->_hp);

        //assert (health->_hp >= 0);

        if (health->_hp == 0)
        {
            _LOG("EVENT: PlayerDied, %d\n", playerId);
            eventManager.SendEvent<PlayerDied>(playerId); //TODO: ShootingSystem срабатывает сразу после этого и удаляет Cannonballs
            //здесь было то, что сейас в хандлгеймовер
        }
    }

    void HandleGameOver()
    {
        _LOG("Destroying player...\n");
        _LOG("Destroying called from line %d\n", __LINE__);
        entityManager.DestroyEntityObject(playerId);
        _LOG("Done!\n");
        playerId = -1;
    }

    void HandleGameStarted (GameStarted* event)
    {
        _LOG("Creating player... \n");
        playerId = entityManager.CreateEntityObject<Player>();
        _LOG("Adding Health... \n");
        componentManager.AddComponent<HealthComponent>(playerId, 5);
        _LOG("Adding Position...\n");
        auto position = componentManager.AddComponent<PositionComponent>(playerId, 0.f, (LOW_WALL_Y + HIGH_WALL_Y) / 2);
        _LOG("Adding Orientation... \n");
        auto orientation = componentManager.AddComponent<OrientationComponent>(playerId, 0);
        _LOG("Adding Moving... \n");
        componentManager.AddComponent<MovingComponent>(playerId, 0.f, 0.f, position);
        _LOG("Adding Collideable... \n");
        componentManager.AddComponent<CollideableComponent>(playerId, 30.f, 0.f, 30.f, 0.f);
        _LOG("Adding Bouncing... \n");
        componentManager.AddComponent<BouncingComponent>(playerId, 0.f, 0);
        _LOG("Adding Drawing...");
        componentManager.AddComponent<DrawingComponent>(playerId, "media/player.png", position, orientation);
        _LOG("Done! id of player=%d\n", playerId);
        eventManager.SendEvent<PlayerSpawned>(playerId);

    }
    
    
public:

    HealthSystem()
    {
        _updateInterval = FRAMERATE / 2;
    }
    
    virtual ~HealthSystem() {}

    virtual float Update() override
    {
        int size = this->_raisedEvents.size();
        while (size > 0)
        {
            id_t eventId = this->_raisedEvents[--size];
            this->_raisedEvents.pop_back();

            //EntityHurt* event = dynamic_cast<EntityHurt*> (eventManager.GetEvent(eventId));
            IEvent* event = eventManager.GetEvent(eventId);
            if (event->_eventTypeId == Event<EntityHurt>::EVENT_TYPE_ID)
                HandleEntityHurt(dynamic_cast<EntityHurt*> (event));
            else if (event->_eventTypeId == Event<GameStarted>::EVENT_TYPE_ID)
                HandleGameStarted(dynamic_cast<GameStarted*> (event));
            else if (event->_eventTypeId == Event<GameOver>::EVENT_TYPE_ID)
                HandleGameOver();
            else
                _LOG("ERROR in HealthSystem: unknown event type %d\n", event->_eventTypeId);
            

            eventManager.EventHandled(eventId, *this);

        }
        
        return 0;
    }

};

class GameStateSystem: public System<GameStateSystem>, public IEventListener //PlayerDied, PlayerSpawned, EnterPressed, PausedOrResumed
{
    int _playersAlive;
    bool _onPause;
    bool _onGame;

    inline void HandleGameOver()
    {
        _LOG("EVENT: GameOver\n")
        eventManager.SendEvent<GameOver>();
        _onPause = false;
        _onGame = false;
    }
    
    
public:

    GameStateSystem():
        _playersAlive(0),
        _onPause(false),
        _onGame(false)
    {
        _updateInterval = FRAMERATE / 2;
    }

    virtual ~GameStateSystem() {}

    virtual float Update() override
    {
        int size = this->_raisedEvents.size();
        for (int i = 0; size > i; i++)
        {
            id_t eventId = this->_raisedEvents[i];

            IEvent* event = eventManager.GetEvent(eventId);
            if (event->_eventTypeId == Event<PlayerSpawned>::EVENT_TYPE_ID)
            {
                ++(this->_playersAlive);
            }
            
            else if (event->_eventTypeId == Event<PlayerDied>::EVENT_TYPE_ID)
            {
                --(this->_playersAlive); //TODO: уничтожением игрока занимается HealthSystem
                if (this->_playersAlive == 0)
                {
                    HandleGameOver();
                }
            }

            else if (event->_eventTypeId == Event<PausedOrResumed>::EVENT_TYPE_ID)
            {
                if (this->_onGame) this->_onPause ^= 1;
            }

            else if (event->_eventTypeId == Event<EnterPressed>::EVENT_TYPE_ID)
            {
                if (this->_onPause) 
                {
                    HandleGameOver();
                }

                else if (!this->_onGame)
                {
                    eventManager.SendEvent<GameStarted>();
                    this->_onPause = false;
                    this->_onGame = true;
                }
            }

            else 
                _LOG("ERROR in GameOverSystem: unknown event %d\n", event->_eventTypeId);

            eventManager.EventHandled(eventId, *this);


        }
        this->_raisedEvents.clear();

        assert (this->_playersAlive >= 0);

        return 0;

    }

};

class UserInputSystem: public System<UserInputSystem>
{
    sf::RenderWindow* _window;

    int GetWASDMovement(sf::Keyboard::Key code)
    {
        switch (code)
        {
            case sf::Keyboard::W: return 0;
            case sf::Keyboard::A: return 1;
            case sf::Keyboard::S: return 2;
            case sf::Keyboard::D: return 3;
            default: return -1;
        }
    }
    
public:

    UserInputSystem (sf::RenderWindow* window):
        _window(window)
    {
        _updateInterval = FRAMERATE;
    }

    virtual ~UserInputSystem() {}

    virtual float Update() override
    {
        static sf::Event event;

        while (_window->pollEvent(event))
            switch (event.type)
            {
                case sf::Event::Closed:
                    {
                        _LOG("EVENT: ExitGame\n"); 
                        eventManager.SendEvent<ExitGame>();
                        break;
                    }

                case sf::Event::KeyPressed:
                    {
                        int wasd = GetWASDMovement(event.key.code);
                        if (wasd != -1)
                        {
                            eventManager.SendEvent<MovementKeyDown>(wasd);
                            break;
                        }
                        if (event.key.code == sf::Keyboard::Escape)
                        {
                            eventManager.SendEvent<PausedOrResumed>();
                            break;
                        }
                        if (event.key.code == sf::Keyboard::Enter)
                        {
                            eventManager.SendEvent<EnterPressed>();
                            break;
                        }
                        break;
                    }
                
                case sf::Event::KeyReleased:
                    {
                        int wasd = GetWASDMovement(event.key.code);
                        if (wasd != -1)
                        {
                            eventManager.SendEvent<MovementKeyUp>(wasd);
                            break;
                        }
                        break;
                    }

                //TODO: Resized, Fullscreen

                default:
                    //Действие, на которое не обращаем внимания
                    break;


            }


        return 0;
    }
};

//TODO: интервал обновления. Передаётся в качестве параметра systemManager у и в нём же хранится

class RenderSystem: public System<RenderSystem>, public IEventListener //GameStarted, GameOver, PausedOrResumed, PlayerSpawned, PlayerDied
{
    bool _onGame;
    bool _onPause;
    entity_id_t _playerId;
    sf::Sprite _brickSprite;
    sf::Texture _brickTexture;
    sf::Sprite _wallSprite;
    sf::Texture _wallTexture;
    sf::Texture _pauseTexture;
    sf::Sprite _pauseSprite;
    sf::Texture _mainmenuTexture;
    sf::Sprite _mainmenuSprite;
    sf::Texture _levelTexture;
    sf::Sprite _levelSprite;
    sf::RenderWindow *_window;
    sf::Text _score_hp;
    sf::Font _font;
    char _str_score_hp [24];
    int _score;

    const char* FONT_FILE = "media/arial.ttf";

    void PrepareString (int hp)
    {
        int digits = 0;
        sprintf(_str_score_hp + 7, "%d%n", _score, &digits);
        _str_score_hp[7 + digits] = ' ';
        sprintf(_str_score_hp + 21, "%d", hp);
    }


    float getDelta (float cameraPos)
    {
        return -((int)cameraPos % 30);
    }

    void CheckEvents()
    {
        int size = this->_raisedEvents.size();
        for (int i = 0; i < size; i++)
        {
            IEvent* event = eventManager.GetEvent(this->_raisedEvents[i]);
            if (event->_eventTypeId == Event<GameStarted>::EVENT_TYPE_ID)
            {
                _onGame = true;
                _score = 0;
                sprintf(_str_score_hp + 7, "\t\t\t\t\t\t\t\t\t");
                _str_score_hp[16] = '\n';
            }
            else if (event->_eventTypeId == Event<GameOver>::EVENT_TYPE_ID)
            {
                _onGame = false;
                _onPause = false;
                _playerId = -1;
            }
            else if (event->_eventTypeId == Event<PausedOrResumed>::EVENT_TYPE_ID)
            {
                if (_onGame)
                    _onPause = !_onPause;
            }
            else if (event->_eventTypeId == Event<PlayerSpawned>::EVENT_TYPE_ID)
            {
                _playerId = (dynamic_cast<PlayerSpawned*>(event))->playerId;
                _LOG("RenderSystem: player set as %d\n", _playerId);
            }
            else if (event->_eventTypeId == Event<PlayerDied>::EVENT_TYPE_ID)
            {
                _playerId = -1;
            }
            else
            {
                _LOG( "ERROR in RenderSystem: unknown event %d\n", event->_eventTypeId);
            }

            eventManager.EventHandled(this->_raisedEvents[i], *this);
        }
        this->_raisedEvents.clear();
    }

public:

    inline sf::RenderWindow* GetWindow() { return (this->_window); }

    RenderSystem ():
        _onGame(false),
        _onPause(false),
        _playerId(-1),
        _brickTexture(),
        _brickSprite(),
        _wallTexture(),
        _wallSprite(),
        _pauseTexture(),
        _pauseSprite(),
        _levelTexture(),
        _levelSprite()
        
    {
        _window = new sf::RenderWindow (sf::VideoMode(WINDOW_X, WINDOW_Y), "XEPOB ETG", sf::Style::Default);
        _wallTexture.loadFromFile("media/wall.png");
        _wallSprite.setTexture(_wallTexture);
        _wallSprite.setScale(3, 3);

        _brickTexture.loadFromFile("media/brick.png");
        _brickSprite.setTexture(_brickTexture);
        _brickSprite.setScale(3, 3);

        _levelTexture.loadFromFile("media/level.png");
        _levelSprite.setTexture(_levelTexture);
        _levelSprite.setPosition(0, LOW_WALL_Y);

        _pauseTexture.loadFromFile("media/pause.png");
        _pauseSprite.setTexture(_pauseTexture);
        _pauseSprite.setPosition((WINDOW_X - 1000) / 2, (WINDOW_Y - 600) / 2);

        _mainmenuTexture.loadFromFile("media/mainmenu.png");
        _mainmenuSprite.setTexture(_mainmenuTexture);
        _mainmenuSprite.setPosition((WINDOW_X - 1000) / 2, (WINDOW_Y - 600) / 2);

        _font.loadFromFile(FONT_FILE);
        _score_hp.setCharacterSize(30);
        _score_hp.setStyle(sf::Text::Bold);
        _score_hp.setFillColor(sf::Color::Cyan);
        _score_hp.setFont(_font);
        _score_hp.setPosition(0.f, 0.f);

        strcpy(_str_score_hp, "Score: \t\t\t\t\t\t\t\t\t\nHP: \t");

        _window->setKeyRepeatEnabled(false);

        _updateInterval = FRAMERATE;

    }

    virtual ~RenderSystem() 
    {
        delete _window;
    }

    virtual float Update() override
    {
        sf::RenderWindow& thisWindow = *(this->_window);
        CheckEvents();
        sf::Vector2f cameraPosition = 
            _playerId == -1 ? 
            sf::Vector2f(0.f, 0.f) : 
            componentManager.GetComponent<PositionComponent>(this->_playerId)->getPosition() + sf::Vector2f (-WINDOW_X / 2, -WINDOW_Y / 2);

        thisWindow.clear();
        if (_onGame)
        {
            //this->_levelSprite.setPosition(getDelta(cameraPosition.x) - 30, HIGH_WALL_Y);
            //this->_window.draw(this->_levelSprite);
            // Нарисовать стены
            float y0, x0, y0_end = HIGH_WALL_Y - cameraPosition.y;

            for (y0 = HIGH_WALL_Y - cameraPosition.y - 4 * 30; y0 < y0_end; y0 += 30)
                for (x0 = getDelta(cameraPosition.x) - 30; x0 < WINDOW_X; x0 += 30)
                {
                    this->_brickSprite.setPosition(x0, y0);
                    thisWindow.draw (this->_brickSprite);
                }
            y0_end = LOW_WALL_Y - cameraPosition.y;
            for (; y0 < y0_end; y0 += 30)
                for (x0 = getDelta(cameraPosition.x) - 30; x0 < WINDOW_X; x0 += 30)
                {
                    this->_wallSprite.setPosition(x0, y0);
                    thisWindow.draw (this->_wallSprite);
                }
            y0_end = LOW_WALL_Y + 4 * 30 - cameraPosition.y;
            for (; y0 < y0_end; y0 += 30)
                for (x0 = getDelta(cameraPosition.x) - 30; x0 < WINDOW_X; x0 += 30)
                {
                    this->_brickSprite.setPosition(x0, y0);
                    thisWindow.draw (this->_brickSprite);
                }



            if (cameraPosition.x > _score)
                _score = cameraPosition.x;

            PrepareString(_playerId == -1 ? 0 : componentManager.GetComponent<HealthComponent>(_playerId)->_hp);

            _score_hp.setString(sf::String(_str_score_hp));
            thisWindow.draw(_score_hp);

        }

        else
        {
            thisWindow.draw(_score_hp);   
            thisWindow.draw (this->_mainmenuSprite);
        }



        // Нарисовать entities

        std::vector<IComponent*> drawingComponents = componentManager.GetEntitiesVector<DrawingComponent>();

        for (IComponent* drawingCo : drawingComponents)
        {
            if (drawingCo == nullptr) continue;
            register auto drrCo = dynamic_cast<DrawingComponent*>(drawingCo);
            drrCo->_sprite.setPosition (drrCo->_position->getPosition() - cameraPosition);

            sf::Sprite& sprite = drrCo->_sprite;
            if (sprite.getPosition().y < WINDOW_Y)
                thisWindow.draw (sprite);
        }



        if (_onPause)
        {
            assert(_onGame);
            this->_window->draw (this->_pauseSprite);
        }




        _window->display(); 


        return 0;

    }
};
                                                                                                     // 10 * 30 pix
class LevelGenSystem: public System<LevelGenSystem>, public IEventListener //GameStarted, PlayerPassedChunk, XReduced, GameOver
{
    int _left_x;
    int _right_x;
    entity_id_t _playerId;
    std::deque <entity_id_t> _turrets;
    sf::Texture _turretTexture;

    const int WIDE = 500 * 30; //pix
    const int TURRETS_INTERVAL = 10 * 30; //pix
    const int NO_TURRETS_ON_START = 25 * 30;

    entity_id_t GenerateCannon (float x, float y, int minAngle, int maxAngle)
    {
        entity_id_t cannon = entityManager.CreateEntityObject<Cannon>();
        PositionComponent* position = componentManager.AddComponent<PositionComponent>(cannon, x, y);
        OrientationComponent* orientation = componentManager.AddComponent<OrientationComponent>(cannon, rand()%(maxAngle - minAngle + 1) + minAngle - 90);
        auto drawing = componentManager.AddComponent<DrawingComponent>(cannon, &_turretTexture, position, orientation);
        drawing->_sprite.setOrigin(15.f, 45.f);
        componentManager.AddComponent<ShootingComponent>(cannon, position, orientation);
        this->_turrets.push_back(cannon);
        return cannon;
    }

    void GenerateBetween(float x1, float x2)
    {
        for (float x = x1; x < x2; x += TURRETS_INTERVAL)
        {
            GenerateCannon(x, LOW_WALL_Y, Cannon::MIN_ANGLE, Cannon::MAX_ANGLE);
            GenerateCannon(x, HIGH_WALL_Y, 180 + Cannon::MIN_ANGLE, 180 + Cannon::MAX_ANGLE);
        }
    }

    void EraseTill (float x0)
    {
        entity_id_t entity = this->_turrets.front();
        PositionComponent* pos = componentManager.GetComponent<PositionComponent>(entity);
        while (true)
        {
            entity = this->_turrets.front();
            pos = componentManager.GetComponent<PositionComponent>(entity);
            if (pos->getPosition().x > x0)
                break;
            _LOG("Destroying called from line %d\n", __LINE__);
            entityManager.DestroyEntityObject(entity);
            this->_turrets.pop_front();
        }
        
    }

    void HandleGameStarted()
    {
        this->_left_x = - WIDE / 4;
        this->_right_x = 3 * WIDE / 4;

        GenerateBetween (NO_TURRETS_ON_START, this->_right_x);

    }

    void HandlePlayerPassedChunk()
    {
        auto playerPos = componentManager.GetComponent<PositionComponent>(this->_playerId);
        if (playerPos == nullptr)
        {
            _LOG("ERROR in WorldGenSystem: can\'t find player\n");
            return;
        }

        float playerX = playerPos->getPosition().x;
        if (playerX * 2 >= this->_left_x + this->_right_x)
        {
            _LOG("Level updated at position %f\n", playerPos->getPosition().x)
            GenerateBetween(this->_right_x, playerX + 3 * WIDE / 4);
            EraseTill(playerX - WIDE / 4);
            this->_left_x  = playerX - 1 * WIDE / 4;
            this->_right_x = playerX + 3 * WIDE / 4;
        }
    }

    void HandleXReduced()
    {
        this->_right_x -= X_DECREASING;
        this->_left_x  -= X_DECREASING;
    }

    void HandleGameOver()
    {
        for (entity_id_t entity : this->_turrets)
        {
            _LOG("Destroying called from line %d\n", __LINE__);
            entityManager.DestroyEntityObject(entity);
        }
        this->_turrets.clear();
    }

    void HandlePlayerSpawned (PlayerSpawned* event)
    {
        this->_playerId = event->playerId;
        _LOG("LevelGenSystem: player set as %d\n", _playerId);
    }

    void HandlePlayerDied ()
    {
        this->_playerId = -1;
    }

public:

    LevelGenSystem ():
        _left_x(0),
        _right_x(0),
        _playerId(-1),
        _turretTexture()
    {
        _updateInterval = CHUNK_SIZE / PLAYER_SPEED / 2;
        _turretTexture.loadFromFile("media/gun.png");
    }

    virtual ~LevelGenSystem() {}

    virtual float Update() override
    {
        int size = this->_raisedEvents.size();
        for (int i = 0; i < size; i++)
        {
            IEvent* event = eventManager.GetEvent(this->_raisedEvents[i]);
            if (event->_eventTypeId == Event<GameStarted>::EVENT_TYPE_ID)   
                HandleGameStarted();
            else if (event->_eventTypeId == Event<PlayerPassedChunk>::EVENT_TYPE_ID)   
                HandlePlayerPassedChunk();
            else if (event->_eventTypeId == Event<XReduced>::EVENT_TYPE_ID)   
                HandleXReduced();
            else if (event->_eventTypeId == Event<GameOver>::EVENT_TYPE_ID)   
                HandleGameOver();
            else if (event->_eventTypeId == Event<PlayerSpawned>::EVENT_TYPE_ID) 
                HandlePlayerSpawned(dynamic_cast<PlayerSpawned*>(event));
            else if (event->_eventTypeId == Event<PlayerDied>::EVENT_TYPE_ID) 
                HandlePlayerDied();
            else
                _LOG("ERROR in LevelGenSystem: unknown event %d\n", event->_eventTypeId);


            eventManager.EventHandled(this->_raisedEvents[i], *this);
        }

        this->_raisedEvents.clear();

        return 0;
             
    }
};


class ExitGameSystem: public System<ExitGameSystem>, public IEventListener //ExitGame
{ public:

    ExitGameSystem()
    {
        _updateInterval = FRAMERATE;
    }

    virtual ~ExitGameSystem() {}

    virtual float Update() override
    {
        if (this->_raisedEvents.size() == 0)
            return 0;

        if (this->_raisedEvents.size() >  1)
            _LOG("ERROR: Unexpected size of _raisedEvents in ExitGameSystem: %d\n", (int)(this->_raisedEvents.size()));

        #ifndef DEBUG
        systemManager.Break();
        #else
        id_t eventId = this->_raisedEvents[0];
        if (eventManager.GetEvent(eventId)->_eventTypeId != Event<ExitGame>::EVENT_TYPE_ID)
        {
            _LOG("ERROR: unknown eventId in ExitGameSystem: %d\n", eventManager.GetEvent(eventId)->_eventTypeId);
        }
        else
        {
            systemManager.Break();
        }
        #endif

        eventManager.EventHandled(this->_raisedEvents[0], *this);
        this->_raisedEvents.pop_back();

        return 0;
    }
};

class XReducingSystem: public System<XReducingSystem>, public IEventListener //XReducing
{ public:

    XReducingSystem()
    {
        _updateInterval = 15 * 60;
    }

    virtual ~XReducingSystem() {}

    virtual float Update() override
    {
        if (this->_raisedEvents.size() >  1)
            _LOG("ERROR: Unexpected size of _raisedEvents in XReducingSystem: %d\n", (int)(this->_raisedEvents.size()));

        if (this->_raisedEvents.size() == 0)
            return 0;

        #ifdef DEBUG
        id_t eventId = this->_raisedEvents[0];
        if (eventManager.GetEvent(eventId)->_eventTypeId != Event<XReducing>::EVENT_TYPE_ID)
        {
            _LOG("ERROR: unknown eventId in XReducingSystem: %d\n", eventManager.GetEvent(eventId)->_eventTypeId);
        }
        #endif

        auto positions = componentManager.GetEntitiesVector<PositionComponent>();

        for (auto position: positions)
        {
            #ifdef DEBUG
            if (((PositionComponent*)position)->getPosition().x < X_DECREASING)
            {
                _LOG("WARNING: position of entity no %d is less than X_DECREASING, but decreased\n", ((PositionComponent*)position)->_owner);
            }
            #endif

            ((PositionComponent*)position)->getPosition().x -= X_DECREASING;

        }

        eventManager.SendEvent<XReduced>();

        return 0;
        
    }
};

class ShootingSystem: public System<ShootingSystem>, public IEventListener //GameOver
{
    struct timespec _timeOfLastUpdate;
    struct timespec _timeOfNextShot;
    sf::Texture _cannonballTexture;

    bool IfGameIsOver()
    {
        int size = this->_raisedEvents.size();
        for (int i = 0; i < size; i++)
        {
            IEvent* ievent = eventManager.GetEvent(this->_raisedEvents[i]);
            GameOver* event = dynamic_cast<GameOver*>(ievent);
            assert(event);

            std::vector<IComponent*> deadlyComponents = componentManager.GetEntitiesVector<DeadlyComponent>();
            for (IComponent* comp : deadlyComponents)
            {
                if (comp == nullptr) continue;

                entityManager.DestroyEntityObject (comp->_owner);
            }


            eventManager.EventHandled(this->_raisedEvents[i], *this);
        }
        this->_raisedEvents.clear();
        return size > 0;
    }

public:

    ShootingSystem():
        _timeOfLastUpdate({}),
        _timeOfNextShot({-1, -1}),
        _cannonballTexture()
    {
        clock_gettime(CLOCK, &_timeOfLastUpdate);
        _updateInterval = FRAMERATE;
        _cannonballTexture.loadFromFile("media/ball.png");
    }

    virtual ~ShootingSystem() {}

    virtual float Update() override
    {
        if (IfGameIsOver())
            return FRAMERATE;

        struct timespec currentTime = {};
        clock_gettime(CLOCK, &currentTime);

        int64_t timebetw = GetTimeBetween_nsec (currentTime, _timeOfNextShot);
        if (timebetw > 0)
        {
            _timeOfLastUpdate = currentTime;
            return 0;
        }

        std::vector<IComponent*> shootings = componentManager.GetEntitiesVector<ShootingComponent>();

        int size = shootings.size();
        int64_t timeToNextShoot_nsec = SHOOTING_SPEED * 1000000000LL;

        for (entity_id_t entityId = 0; entityId < size; entityId++)
        {
            ShootingComponent* shooting = dynamic_cast<ShootingComponent*>(shootings[entityId]);

            if (shooting == nullptr)
                continue;

            int64_t thou_timeToNextShoot_nsec = SHOOTING_SPEED * 1000000000LL - GetTimeBetween_nsec (shooting->_timeOfLastShot, currentTime);
            if (thou_timeToNextShoot_nsec < 0)
            {
                shooting->Shoot (currentTime, &(this->_cannonballTexture));
            }
            else if (thou_timeToNextShoot_nsec < timeToNextShoot_nsec)
                timeToNextShoot_nsec = thou_timeToNextShoot_nsec;
        }

        _timeOfNextShot = currentTime;
        _timeOfNextShot.tv_nsec += timeToNextShoot_nsec % 1000000000LL;
        _timeOfNextShot.tv_sec  += timeToNextShoot_nsec / 1000000000LL;
        _timeOfLastUpdate = currentTime;

        return 0;
    }

};


void Runtime()
{

    auto 
    system = dynamic_cast<IEventListener*> (systemManager.AddSystem<MovingSystem>());
    eventManager.Subscribe<PlayerSpawned>    (system);
    eventManager.Subscribe<GameOver>         (system);

    system = dynamic_cast<IEventListener*> (systemManager.AddSystem<ShootingSystem>());
    eventManager.Subscribe<GameOver>         (system);
    
    system = dynamic_cast<IEventListener*> (systemManager.AddSystem<HealthSystem>());
    eventManager.Subscribe<EntityHurt>       (system);
    eventManager.Subscribe<GameStarted>      (system);
    eventManager.Subscribe<GameOver>         (system);

    system = dynamic_cast<IEventListener*> (systemManager.AddSystem<DrivingSystem>());
    eventManager.Subscribe<MovementKeyDown>  (system);
    eventManager.Subscribe<MovementKeyUp>    (system);
    eventManager.Subscribe<PlayerSpawned>    (system);
    eventManager.Subscribe<GameOver>         (system);

    auto render = systemManager.AddSystem<RenderSystem>();

    system = dynamic_cast<IEventListener*> (render);
    eventManager.Subscribe<GameStarted>      (system);
    eventManager.Subscribe<GameOver>         (system);
    eventManager.Subscribe<PausedOrResumed>  (system);
    eventManager.Subscribe<PlayerSpawned>    (system);
    eventManager.Subscribe<PlayerDied>       (system);

    sf::RenderWindow* window = render->GetWindow();
    systemManager.AddSystem<UserInputSystem>(window);

    system = dynamic_cast<IEventListener*> (systemManager.AddSystem<LevelGenSystem>());
    eventManager.Subscribe<GameStarted>      (system);
    eventManager.Subscribe<GameOver>         (system);
    eventManager.Subscribe<XReduced>         (system);
    eventManager.Subscribe<PlayerPassedChunk>(system);
    eventManager.Subscribe<PlayerSpawned>    (system);
    eventManager.Subscribe<PlayerDied>       (system);

    system = dynamic_cast<IEventListener*> (systemManager.AddSystem<ExitGameSystem>());
    eventManager.Subscribe<ExitGame>         (system);

    system = dynamic_cast<IEventListener*> (systemManager.AddSystem<WallCollisionSystem>());
    eventManager.Subscribe<WallCollision>    (system);

    system = dynamic_cast<IEventListener*> (systemManager.AddSystem<GameStateSystem>());
    eventManager.Subscribe<EnterPressed>     (system);
    eventManager.Subscribe<PausedOrResumed>  (system);
    eventManager.Subscribe<PlayerSpawned>    (system);
    eventManager.Subscribe<PlayerDied>       (system);



    systemManager.SetPriority<GameStateSystem>(10);
    systemManager.SetPriority<WallCollisionSystem>(9);
    systemManager.SetPriority<ExitGameSystem>(8);
    systemManager.SetPriority<LevelGenSystem>(7);
    systemManager.SetPriority<UserInputSystem>(6);
    systemManager.SetPriority<RenderSystem>(5);
    systemManager.SetPriority<DrivingSystem>(4);
    systemManager.SetPriority<HealthSystem>(3);
    systemManager.SetPriority<MovingSystem>(2);
    systemManager.SetPriority<ShootingSystem>(1);
    

    float toSleep = 0.f;
    while (!std::isnan(toSleep))
    {
        if (toSleep > 10000.f) _LOG("WARNING: toSleep is too big: %f (%d, %X)\n", toSleep, (int)(toSleep), (int)(toSleep));
        usleep((unsigned)(toSleep * 1000));
        toSleep = systemManager.Update();
        //printf("%f\n", toSleep);
    }
}

int main()
{ 
    fclose(fopen("debug.log", "w"));
    _LOG("%d", (int)(std::isnan(NAN)));
    SetupManagers();
    auto begin = std::chrono::high_resolution_clock::now();
    Runtime();

}
























/*class Player: public Entity<Player>
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

}*/
