/**
  Thats right... Chlorine's tests are written with chlorine!
*/

#include <chlorine.h>

CL_SETUP_ONCE
{
    cl_log("You won't see this message, rather an [ERROR] in the output.");
}

CL_SETUP
{
    cl_assert_msg(true, "The serial test better not fail!");
    cl_log("SETUP: This log is probably in parallel.");
}

CL_TEARDOWN
{
    if (!strcmp(cl_get_name(), "serial")) {
        cl_assert_msg(true, "RUNTIME INTROSPECTION!?");
        cl_log("This will only get called in the serial SPEC. My mind is blown.");
    }
}

CL_SPEC_OPTIONS (serial, CL_OPTION_SERIAL | CL_OPTION_SKIP_SETUP)
{
    cl_assert(cl_num_passed() == 0);
    cl_assert(cl_num_passed() == 1);
    cl_assert(cl_num_passed() == 2);
    cl_assert(cl_num_passed() == 3);
    cl_assert(cl_num_failed() == 0);
    cl_assert(!cl_has_failed());
    cl_assert(!strcmp(cl_get_name(), "serial"));
    cl_assert(!cl_is_parallel());
}

CL_FIXTURE (parallel_setup)
{
    cl_set_userdata((void *)cl_get_name());
}

void parallel_work()
{
    cl_assert(cl_is_parallel());
    const char *name = cl_get_userdata();
    cl_log("I'm a test named: %s", name);
    cl_assert(!strncmp(name, "parallel", 8));
}

#define PARALLEL(N) \
CL_SPEC_FIXTURE (parallel##N, parallel_setup, CL_FIXTURE_NONE) { parallel_work(); }

PARALLEL(1) PARALLEL(2) PARALLEL(3)

CL_BUNDLE_PARALLEL
(
    serial,
    parallel1, parallel2, parallel3
)
