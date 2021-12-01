#include <iostream>
#include <sstream>
#include "apex.hpp"
#include "apex_starpu.hpp"

#include "starpu_prof.h"

#warning "Compiling StarPU support"

extern "C" {
    void myfunction_cb( starpu_prof_info* prof_info, starpu_event_info* event_info, starpu_api_info* api_info ){
        //  DEBUG_PRINT("EVENT CALLED\n");
        std::cout << "Event called" << std::endl;
    }
    
/* Mandatory */

void starpu_register_library( starpu_prof_reg reg, starpu_prof_reg unreg ) {
    APEX_UNUSED(unreg);
    std::cout << "register library" << std::endl;

    dev_type[starpu_driver_cpu] = "CPU";
    dev_type[starpu_driver_gpu] = "GPU";

    starpu_register_t info;
    reg( starpu_event_init_begin, &myfunction_cb, info );
    reg( starpu_event_init_end, &myfunction_cb, info );
    reg( starpu_event_init, &myfunction_cb, info );
    reg( starpu_event_terminate, &myfunction_cb, info );
    reg( starpu_event_driver_init, &myfunction_cb, info );
    reg( starpu_event_driver_deinit, &myfunction_cb, info );
    reg( starpu_event_driver_init_start, &myfunction_cb, info );
    reg( starpu_event_driver_init_end, &myfunction_cb, info );
    reg( starpu_event_start_cpu_exec, &myfunction_cb, info );
    reg( starpu_event_end_cpu_exec, &myfunction_cb, info );
    reg( starpu_event_start_gpu_exec, &myfunction_cb, info );
    reg( starpu_event_end_gpu_exec, &myfunction_cb, info );
    reg( starpu_event_start_transfer, &myfunction_cb, info );
    reg( starpu_event_end_transfer, &myfunction_cb, info );


}


}
