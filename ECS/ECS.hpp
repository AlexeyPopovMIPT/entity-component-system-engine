#include <cstdio>
#include <cstring> //for memset(), memmove()
#include <vector>

// Include doubly linked list library
#define List_elem_t void*
#define NO_GetByPhInd
#include "../../lib/The List.h"
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


#include "IDManager.hpp"


///---------------------------------------------------------------------------------
///-------------------------           Component          --------------------------
#include "Component.hpp"


///---------------------------------------------------------------------------------
///-------------------------             Entity           --------------------------
#include "Entity.hpp"



///---------------------------------------------------------------------------------
///-----------------------------         Event          ----------------------------

#include "Event.hpp"


///---------------------------------------------------------------------------------
///-----------------------------        System         -----------------------------

#include "System.hpp"


void SetupManagers()
{
    componentManager.Setup();
    systemManager.Setup();
    eventManager.Setup();
}