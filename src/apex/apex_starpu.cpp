#include <iostream>
#include <sstream>
#include "apex.hpp"
#include "apex_starpu.hpp"

#include "starpu_prof.h"
#include <starpu.h>

#warning "-----Compiling StarPU support"

#include "address_resolution.hpp"

extern "C" {
    
    const enum starpu_perf_counter_scope g_scope = starpu_perf_counter_scope_global;
    const enum starpu_perf_counter_scope w_scope = starpu_perf_counter_scope_per_worker;
    const enum starpu_perf_counter_scope c_scope = starpu_perf_counter_scope_per_codelet;
    
    thread_local struct starpu_perf_counter_listener* g_listener;
    thread_local struct starpu_perf_counter_listener* w_listener;
    thread_local struct starpu_perf_counter_set* g_set;
    thread_local struct starpu_perf_counter_set* w_set;
    
    void init_sched_listeners( struct starpu_conf* );
    void terminate_sched_listeners( void );
    
    void myfunction_cb( starpu_prof_info* prof_info, starpu_event_info* event_info, starpu_api_info* api_info ){
        //  DEBUG_PRINT("EVENT CALLED\n");
        //     std::cout << "Event called" << std::endl;

        std::stringstream ss, info;
        bool withFileInfo = true;        
        int start = -1;
        
        int tag = 0;

        const std::string* name = apex::lookup_address( (uintptr_t)(prof_info->fun_ptr),  withFileInfo);
        
        switch(  prof_info->event_type ) {
        case starpu_event_init:
            start = 1;
            ss << "StarPU";
            break;
        case starpu_event_terminate:
            terminate_sched_listeners();
            start = -1;
            break;
        case starpu_event_init_begin:
            start = 1;
            ss << "StarPU init";
            break;
        case starpu_event_init_end:
            if( nullptr != prof_info->conf ){
                init_sched_listeners( prof_info->conf );
            }
            start = 0;
            break;
        case starpu_event_driver_init:
            start = 1;
            ss << "StarPU driver";
            info << dev_type[prof_info->driver_type] << ":" << prof_info->device_number;
            break;       
        case starpu_event_driver_deinit:
            start = 0;
            //    info << " " << dev_type[prof_info->driver_type] << ":" << prof_info->device_number  << "}]";
            break;
        case starpu_event_driver_init_start:
            start = 1;
            ss << "StarPU driver init";
            info << dev_type[prof_info->driver_type] << ":" << prof_info->device_number;
           break;
        case starpu_event_driver_init_end:
            start = 0;
           break;
        case starpu_event_start_cpu_exec:
        case starpu_event_start_gpu_exec:
            ss << "StarPU exec ";
            info << dev_type[prof_info->driver_type] << ":" << prof_info->device_number;
            start = 1;
            break;
        case starpu_event_end_cpu_exec:
        case starpu_event_end_gpu_exec:
            start = 0;
            break;
        case starpu_event_start_transfer:
            ss << "StarPU_transfer";
            info << "memnode " << prof_info->memnode;
            start = 1;
            break;
        case starpu_event_end_transfer:
            start = 0;
            break;
        default:
            start = -1;
            ss << "UNKNOWN STARPU OTHER EVENT";
            std::cerr <<  "Unknown callback " <<  prof_info->event_type << std::endl;
            break;
        }

        if (start == 0) {
            apex::stop(apex::thread_instance::instance().get_current_profiler());
            return;
        }

        ss << " [{" << info.str() << ":";
        if( name != nullptr ) {
            ss << *name;
        }
        ss << "}]";
                
        std::string tmp{ss.str()};
        if (start == 1) {
            void* func_info = (void*)apex::start(tmp);
            //   other_event->tool_info = func_info;
        }
        else {
            apex::sample_value(tmp, 1);
        }
    }
    
/* Mandatory */

void starpu_register_library( starpu_prof_reg reg, starpu_prof_reg unreg ) {
    APEX_UNUSED(unreg);

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
    
    /* Register listeners for the scheduler's internal counters */
    
    /* global counters */
    thread_local int id_g_total_submitted;
    thread_local int id_g_peak_submitted;
    thread_local int id_g_peak_ready;
    
    /* per worker counters */
    thread_local int id_w_total_executed;
    thread_local int id_w_cumul_execution_time;
    

    void g_listener_cb(struct starpu_perf_counter_listener *listener, struct starpu_perf_counter_sample *sample, void *context){
        (void) listener;
        (void) context;
        int64_t g_total_submitted = starpu_perf_counter_sample_get_int64_value(sample, id_g_total_submitted);
        int64_t g_peak_submitted = starpu_perf_counter_sample_get_int64_value(sample, id_g_peak_submitted);
        int64_t g_peak_ready = starpu_perf_counter_sample_get_int64_value(sample, id_g_peak_ready);
        std::stringstream ss;
        ss << "global: g_total_submitted = ";
        ss <<  g_total_submitted;
        ss << ", g_peak_submitted = ";
        ss <<  g_peak_submitted;
        ss << ", g_peak_ready = ";
        ss << g_peak_ready;
        ss << std::endl;

        std::cout << ss.str();
    }
    
    void w_listener_cb(struct starpu_perf_counter_listener *listener, struct starpu_perf_counter_sample *sample, void *context) {
        (void) listener;
        (void) context;
        int workerid = starpu_worker_get_id();
        int64_t w_total_executed = starpu_perf_counter_sample_get_int64_value(sample, id_w_total_executed);
        double w_cumul_execution_time = starpu_perf_counter_sample_get_double_value(sample, id_w_cumul_execution_time);
        
        std::stringstream ss;
        ss << "worker[";
        ss << workerid;
        ss << "]: w_total_executed = ";
        ss << w_total_executed;
        ss << ", w_cumul_execution_time = ";
        ss << w_cumul_execution_time;
        ss << std::endl;

        std::cout << ss.str();        
    }
    
    void init_sched_listeners(struct starpu_conf* conf ){
        std::cout << "init sched listeners" << std::endl;
        conf->start_perf_counter_collection = 1;
        
        g_set = starpu_perf_counter_set_alloc(g_scope);
        STARPU_ASSERT(g_set != NULL);
        w_set = starpu_perf_counter_set_alloc(w_scope);
        STARPU_ASSERT(w_set != NULL);
        
        id_g_total_submitted = starpu_perf_counter_name_to_id(g_scope, "starpu.task.g_total_submitted");
        STARPU_ASSERT(id_g_total_submitted != -1);
        id_g_peak_submitted = starpu_perf_counter_name_to_id(g_scope, "starpu.task.g_peak_submitted");
        STARPU_ASSERT(id_g_peak_submitted != -1);
        id_g_peak_ready = starpu_perf_counter_name_to_id(g_scope, "starpu.task.g_peak_ready");
        STARPU_ASSERT(id_g_peak_ready != -1);
        
        
        id_w_total_executed = starpu_perf_counter_name_to_id(w_scope, "starpu.task.w_total_executed");
        STARPU_ASSERT(id_w_total_executed != -1);
        id_w_cumul_execution_time = starpu_perf_counter_name_to_id(w_scope, "starpu.task.w_cumul_execution_time");
        STARPU_ASSERT(id_w_cumul_execution_time != -1);

        starpu_perf_counter_set_enable_id(g_set, id_g_total_submitted);
        starpu_perf_counter_set_enable_id(g_set, id_g_peak_submitted);
        starpu_perf_counter_set_enable_id(g_set, id_g_peak_ready);
        
        starpu_perf_counter_set_enable_id(w_set, id_w_total_executed);
        starpu_perf_counter_set_enable_id(w_set, id_w_cumul_execution_time);
        
        g_listener = starpu_perf_counter_listener_init(g_set, g_listener_cb, (void *)(uintptr_t)42);
        w_listener = starpu_perf_counter_listener_init(w_set, w_listener_cb, (void *)(uintptr_t)17);

        starpu_perf_counter_set_global_listener(g_listener);
        starpu_perf_counter_set_all_per_worker_listeners(w_listener);
    }
    
    void terminate_sched_listeners( ){
        std::cout << "terminate sched listeners" << std::endl;
        
        starpu_perf_counter_unset_all_per_worker_listeners();
        starpu_perf_counter_unset_global_listener();
        
        starpu_perf_counter_listener_exit(w_listener);
        starpu_perf_counter_listener_exit(g_listener);
        
        starpu_perf_counter_set_disable_id(w_set, id_w_cumul_execution_time);
        starpu_perf_counter_set_disable_id(w_set, id_w_total_executed);
        
        starpu_perf_counter_set_disable_id(g_set, id_g_peak_ready);
        starpu_perf_counter_set_disable_id(g_set, id_g_peak_submitted);
        starpu_perf_counter_set_disable_id(g_set, id_g_total_submitted);
        
        starpu_perf_counter_set_free(w_set);
        w_set = NULL;
        
        starpu_perf_counter_set_free(g_set);
        g_set = NULL;
    }
    
}
