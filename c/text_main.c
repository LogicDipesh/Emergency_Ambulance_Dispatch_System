
#include <stdio.h>
#include "engine.h"

int main(void)
{
    printf("=== Emergency Ambulance Dispatch System (Dehradun) ===\n\n");

    engine_init();

    engine_add_ambulance(3);   /* Dehradun Railway Station */
    engine_add_ambulance(12);  /* Clement Town */
    printf("Fleet size: %d ambulances\n\n", engine_ambulance_count());

    engine_add_emergency(9, 1);   /* Tapkeshwar — critical */
    engine_add_emergency(10, 3);  /* Sahastradhara — minor */
    engine_add_emergency(4, 2);   /* ISBT — serious */
    engine_add_emergency(14, 1);  /* Raipur — critical */
    engine_add_emergency(8, 2);   /* Robber's Cave — serious */

    for (int t = 1; t <= 120; t++) {
        engine_tick(1.0);   
        if (t % 20 == 0)
            printf("--- t = %.0f min ---\n%s\n\n", engine_sim_time(),
                   engine_get_state());
    }

    printf("=== final state ===\n%s\n", engine_get_state());
    return 0;
}
