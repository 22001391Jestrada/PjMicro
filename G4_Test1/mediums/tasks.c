#include "tasks.h"
#include "seg.h"
#include "iomap.h"
#include "pot2seg.h"
#include "lcdwatch.h"
#include "arcade.h"

// se llama en cada vuelta del while(1) del main; cada tarea revisa si le toca
void tasks_update(void){
    pot2seg_task();
    seg_task();
    iomap_task();
    lcdwatch_task();
    arcade_task();
}
