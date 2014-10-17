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

  ☑ Test fixtures

  ☐ Test timeouts
  
##Quickstart

1. First grab a copy of [chlorine.h](https://github.com/pyrated/chlorine/blob/master/chlorine.h) and add it somewhere easily accessible in your project.

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

##Fixtures and Parallelism

Sometimes you need to have some common setup and teardown code for each test spec. Imagine this scenario:

```c
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "chlorine.h"

typedef struct TestData {
  int number;
  char* allocated_blob;
} TestData;

void setup_testdata(TestData* testdata) {
  testdata->number = 0;
  testdata->allocated_blob = strdup("malloc");
  cl_assert(testdata->allocated_blob, "malloc works!");
}

void cleanup_testdata(TestData* testdata) {
  free(testdata->allocated_blob);
}

CL_SPEC (thing) {
  TestData td;
  setup_testdata(&td);
  cl_assert(td.number == 0, "must be zero");
  td.number += 1;
  cleanup_testdata(&td);
}

CL_SPEC (thing2) {
  TestData td;
  setup_testdata(&td);
  cl_assert(td.number == 0, "must be zero");
  td.number += 2;
  cleanup_testdata(&td);
}

CL_SPEC (thing3) {
  TestData td;
  setup_testdata(&td);
  cl_assert(td.number == 0, "must be zero");
  td.number += 3; 
  cleanup_testdata(&td);
}

CL_BUNDLE (
  thing, thing2, thing3,
);
```

Each spec has tons of repeated setup code. We can simply this with fixtures. Here's an example using **CL_SETUP** and **CL_TEARDOWN**:

```c
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "chlorine.h"

typedef struct TestData {
  int number;
  char* allocated_blob;
} TestData;

TestData td;

CL_SETUP {
  td.number = 0;
  td.allocated_blob = strdup("malloc");
  cl_assert(td.allocated_blob, "malloc works!");
}

CL_TEARDOWN {
  free(td.allocated_blob);
}

CL_SPEC (thing) {
  td.number += 1;
}

CL_SPEC (thing2) {
  td.number += 2;
}

CL_SPEC (thing3) {
  td.number += 3; 
}

CL_BUNDLE (
  thing, thing2, thing3,
);
```

Here **CL_SETUP** and **CL_TEARDOWN** act like functions that are run at the beginning and end of each SPEC. They still continue to work like functions called from a spec, allowing you to call **cl_assert** for example.

But we can't safely run this in parallel... The global variable **td** that we factored out could be modified by each SPEC if we were running them all at once. We can solve this problem with **cl_set_userdata** and **cl_get_userdata**:

```c
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "chlorine.h"

typedef struct TestData {
  int number;
  char* allocated_blob;
} TestData;

CL_SETUP {
  TestData* td = malloc(sizeof(TestData));
  td->number = 0;
  td->allocated_blob = strdup("malloc");
  cl_assert(td->allocated_blob, "malloc works!");
  cl_set_userdata(td);
}

CL_TEARDOWN {
  TestData *td = cl_get_userdata();
  free(td->allocated_blob);
  free(td);
}

CL_SPEC (thing) {
  TestData *td = cl_get_userdata();
  td->number += 1;
}

CL_SPEC (thing2) {
  TestData *td = cl_get_userdata();
  td->number += 2;
}

CL_SPEC (thing3) {
  TestData *td = cl_get_userdata();
  td->number += 3; 
}

CL_BUNDLE_PARALLEL (
  thing, thing2, thing3,
);
```

Now we can run in parallel! **cl_set_userdata** and **cl_get_userdata** allow you to access a special pointer that is unique for each running thread. But if you want to set up some immutable shared data for all of your specs, then you can use the very awesome **CL_SETUP_ONCE** and **CL_TEARDOWN_ONCE**:

```c
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "chlorine.h"

typedef struct TestData {
  int number;
  char* allocated_blob;
} TestData;

TestData td;

CL_SETUP_ONCE {
  td.number = 42;
  td.allocated_blob = strdup("malloc");
}

CL_TEARDOWN_ONCE {
  free(td.allocated_blob);
}

CL_SETUP {
  cl_assert(td.number == 42, "shared test");
}

CL_SPEC (thing) {
  cl_log("%s %d", td.allocated_blob, td.number + 1);
}

CL_SPEC (thing2) {
  cl_log("%s %d", td.allocated_blob, td.number + 2);
}

CL_SPEC (thing3) {
  cl_log("%s %d", td.allocated_blob, td.number + 3);
}

CL_BUNDLE_PARALLEL (
  thing, thing2, thing3,
);
```

**CL_SETUP_ONCE** and **CL_TEARDOWN_ONCE** run exactly as you would expect, only once! It is important to note that **CL_SETUP_ONCE** and **CL_TEARDOWN_ONCE** are not part of a SPEC. You cannot call methods like **cl_assert**, **cl_log**, or **cl_get_userdata** because they are not ran as part of the test. They are just initial setup and teardown. If you do accidently call those functions, Chlorine will display a message with some helpful info and try to abort the test:

```c
#include <stdbool.h>

#include "chlorine.h"

CL_SETUP_ONCE {
  cl_log("Logging once...");
}

CL_SPEC (cant_fail) {
  cl_assert(true, "yes");
}

CL_BUNDLE_PARALLEL (
  cant_fail,
);
```

output:

```
[INFO] Running PARALLEL BUNDLE: bad_test.c

[ERROR] Detecting call to cl_log() from outside the scope of a SPEC at bad_test.c:6

[INFO] Executing SPEC => cant_fail

        [PASS]  Passed SPEC in 0.0000 s -> 0 fail, 1 pass

======================================================

[SUCCESS] 0 SPECS failed, 1 SPECS passed in 0.0001 s
```

##Spec configuration

You can also define specs with special options using **CL_SPEC_OPTIONS**. Two handy option flags are **CL_OPTION_SKIP_SETUP**, and **CL_OPTION_SKIP_TEARDOWN**. When a spec is created with these flags it will do as you expect, skip setup or teardown:

```c
#include <stdbool.h>
#include "chlorine.h"

CL_SETUP {
  cl_assert(false, "false");
}

CL_TEARDOWN {
  cl_assert(true, "true");
}

CL_SPEC (fails_inherits_setup) { }

CL_SPEC_OPTIONS (fails_no_tests,
  CL_OPTION_SKIP_SETUP | CL_OPTION_SKIP_TEARDOWN) {
  cl_log("no tests");
}

CL_SPEC_OPTIONS (fails_skips_teardown, CL_OPTION_SKIP_TEARDOWN) { }

CL_SPEC_OPTIONS (passes_skips_setup, CL_OPTION_SKIP_SETUP) { }

CL_BUNDLE (
  fails_inherits_setup,
  fails_no_tests,
  fails_skips_teardown,
  passes_skips_setup
);
```

outputs:

```
[INFO] Running BUNDLE: tests.c

[INFO] Executing SPEC => fails_inherits_setup

        [ERROR] Assertion Failed: false in tests.c:5
                false

        [FAIL]  Failed SPEC in 0.0000 s -> 1 fail, 1 pass

======================================================

[INFO] Executing SPEC => fails_no_tests

        [LOG]   no tests
        [FAIL]  Failed SPEC in 0.0000 s -> 0 fail, 0 pass

======================================================

[INFO] Executing SPEC => fails_skips_teardown

        [ERROR] Assertion Failed: false in libtofu/tests/tofu_runtime_tests.c:5
                false

        [FAIL]  Failed SPEC in 0.0000 s -> 1 fail, 0 pass

======================================================

[INFO] Executing SPEC => passes_skips_setup

        [PASS]  Passed SPEC in 0.0000 s -> 0 fail, 1 pass

======================================================

[FAILURE] 3 SPECS failed, 1 SPECS passed in 0.0003 s
```

##Other Stuff

Im still working on this as I expand it for use in my own projects. It is actually now pretty usable.

##Copyright & License

Copyright (c) 2014 Scott LaVigne. See [LICENSE.txt](https://github.com/pyrated/chlorine/blob/master/LICENSE.txt) for details. 
