#ifndef ENGINE_H
#define ENGINE_H

#ifdef _WIN32
#define ENGINE_API __declspec(dllexport)
#else
#define ENGINE_API
#endif

ENGINE_API void        engine_init(void);
ENGINE_API void        engine_reset(void);
ENGINE_API int         engine_add_ambulance(int node);            
ENGINE_API int         engine_remove_ambulance(int amb_id);       
ENGINE_API int         engine_set_ambulance_node(int amb_id, int node); 
ENGINE_API int         engine_add_emergency(int node, int severity);    
ENGINE_API void        engine_tick(double sim_minutes);
ENGINE_API const char *engine_get_state(void);   
ENGINE_API int         engine_ambulance_count(void);
ENGINE_API double      engine_sim_time(void);

#endif
