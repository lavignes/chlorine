#Chlorine<sub>2</sub>

A more stable molecule.

##What the heck is this thing?

  * Chlorine is micro-framework for making C unit tests.

  * It is meant to be easily dropped into existing C projects to facilitate unit testing.

  * It aims to be [thread-safe](#threading) and incredibly simple to use.
  
##Features
  
  ☑ Test suites (Use chlorine bundles)

  ☑ Tests run on seperate threads **(New in Cl<sub>2</sub>)**
  
  ☐ Test pooling
  
  ☑ Non-fatal assertions **(New in Cl<sub>2</sub>)**
  
  ☑ Re-entrant assertions **(New in Cl<sub>2</sub>)**
  
  ☑ Re-entrant Logging and timing **(New in Cl<sub>2</sub>)**

  ☐ Test timeouts
    
  ☐ Test fixtures
  
##Quickstart

1. First grab a copy of [chlorine.h](https://raw.githubusercontent.com/pyrated/chlorine/master/chlorine.h) and add it somewhere easily accessible in your project.

2. Create a new file to house a test bundle. For example: "my_tests.c"
 
    ```c
    #include "chlorine.h"
    
    CL_SPEC (my_first_test) {
        cl_log("This is my first empty test");
    }
    
    CL_BUNDLE (
        my_first_test,
        /* I can add more specs here */
    );
    ```

3. Compile & Run

    ```sh
    $ cc my_tests.c -o my_tests && ./my_tests
    ```

You see output similar to:

```
[INFO] Running BUNDLE: my_tests.c

[INFO] Executing SPEC => my_first_test

        [LOG]   This is my first empty test
        [FAIL]  Failed SPEC in 0.0000 s -> 0 fail, 0 pass

======================================================

[FAILURE] 1 SPECS failed, 0 SPECS passed in 0.0001 s
```
  
The test spec for my_first_test fails because it doesn't actually test for anything. If we want the test to blindly pass, we'll need to add a test condition. We can add conditions with **cl_assert()**:

```c
#include <stdbool.h>
#include "chlorine.h"
    
CL_SPEC (my_first_test) {
    cl_assert(true, "This will always pass");
}

CL_BUNDLE (
    my_first_test,
);
```

When we compile and run again we see:
    
```
[INFO] Running BUNDLE: my_tests.c

[INFO] Executing SPEC => my_first_test

        [PASS]  Passed SPEC in 0.0000 s -> 0 fail, 1 pass

======================================================

[SUCCESS] 0 SPECS failed, 1 SPECS passed in 0.0002 s
```

You can test multiple conditions in a single test spec:

```c
#include <stdbool.h>
#include "chlorine.h"

CL_SPEC (my_first_test) {
    cl_assert(true, "Always true");
    cl_assert(false, "Always false");
    cl_assert(2 + 2 == 5, "... still false");
    cl_assert(true, "Finish on a high note!");
}

CL_BUNDLE (
    my_first_test,
);
```

output:

```
[INFO] Running BUNDLE: my_tests.c

[INFO] Executing SPEC => my_first_test

        [ERROR] Assertion Failed: false in my_tests.c:6
                Always false

        [ERROR] Assertion Failed: 2 + 2 == 5 in my_tests.c:7
                ... still false

        [FAIL]  Failed SPEC in 0.0000 s -> 2 fail, 2 pass

======================================================

[FAILURE] 1 SPECS failed, 0 SPECS passed in 0.0001 s
```

##Threading

By default, Chlorine runs all tests serially. This is actually faster for many short and quick specs. But imagine you have tests like this: *(Take note of the order of the specs in the bundle)*

```c
#include <unistd.h>
#include <stdbool.h>
#include "chlorine.h"

// We can call cl_assert outside of a spec
// because it is thread-safe and re-entrant.
void long_operation(int time) {
  sleep(time);
  cl_assert(true, "true");
}

CL_SPEC (fast_test) {
    long_operation(0);
}

CL_SPEC (one_sec_test) {
    long_operation(1);
}

CL_SPEC (two_sec_test) {
    long_operation(2);
}

CL_BUNDLE (
    two_sec_test,
    fast_test,
    one_sec_test,
);
```

These specs take 3 seconds to run in serial:

```
[INFO] Running BUNDLE: my_tests.c

[INFO] Executing SPEC => two_sec_test

        [PASS]  Passed SPEC in 2.0011 s -> 0 fail, 1 pass

======================================================

[INFO] Executing SPEC => fast_test

        [PASS]  Passed SPEC in 0.0000 s -> 0 fail, 1 pass

======================================================

[INFO] Executing SPEC => one_sec_test

        [PASS]  Passed SPEC in 1.0010 s -> 0 fail, 1 pass

======================================================

[SUCCESS] 0 SPECS failed, 3 SPECS passed in 3.0026 s
```

But there are no issues with these specs mutating shared state (And there **never** should be). We can run these tests in parallel. Just change **CL_BUNDLE** to **CL_BUNDLE_PARALLEL**. So easy!

```c
#include <unistd.h>
#include <stdbool.h>
#include "chlorine.h"

void long_operation(int time) {
  sleep(time);
  cl_assert(true, "true");
}

CL_SPEC (fast_test) {
    long_operation(0);
}

CL_SPEC (one_sec_test) {
    long_operation(1);
}

CL_SPEC (two_sec_test) {
    long_operation(2);
}

CL_BUNDLE_PARALLEL (
    two_sec_test,
    fast_test,
    one_sec_test,
);
```

outputs:

```
[INFO] Running PARALLEL BUNDLE: my_tests.c

[INFO] Executing SPEC => two_sec_test

        [PASS]  Passed SPEC in 2.0005 s -> 0 fail, 1 pass

======================================================

[INFO] Executing SPEC => fast_test

        [PASS]  Passed SPEC in 0.0000 s -> 0 fail, 1 pass

======================================================

[INFO] Executing SPEC => one_sec_test

        [PASS]  Passed SPEC in 1.0011 s -> 0 fail, 1 pass

======================================================

[SUCCESS] 0 SPECS failed, 3 SPECS passed in 2.0007 s
```

The tests now only take as long as the slowest spec. All tests are running in parallel but **the logging order is still retained**! Each spec logs to thread-local storage and the log order is resolved in real time as the tests complete!

##Other Stuff

Im still working on this as I expand it for use in my own projects. It is actually now pretty usable.

##Copyright & License

Copyright (c) 2014 Scott LaVigne. See [LICENSE.txt](https://github.com/pyrated/chlorine/blob/master/LICENSE.txt) for details. 
