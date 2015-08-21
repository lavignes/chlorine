#ifndef CHLORINE2
#define CHLORINE2

#include <sys/time.h>

#include <pthread.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

#ifndef CHLORINE_NO_COLORS
  #define __CL_RESET "\x1B[0m"
  #define __CL_BOLD "\x1B[1m"
  #define __CL_UL "\x1B[4m"
  #define __CL_TAB "        "

  #define __CL_BLACK "0"
  #define __CL_RED "1"
  #define __CL_GREEN "2"
  #define __CL_YELLOW "3"
  #define __CL_BLUE "4"
  #define __CL_MAGENTA "5"
  #define __CL_CYAN "6"
  #define __CL_WHITE "7"
  #define __CL_FG(color) "\x1B[3"color"m"
  #define __CL_BG(color) "\x1B[4"color"m"
#else
  #define __CL_RESET ""
  #define __CL_BOLD ""
  #define __CL_UL ""
  #define __CL_TAB "        "

  #define __CL_BLACK ""
  #define __CL_RED ""
  #define __CL_GREEN ""
  #define __CL_YELLOW ""
  #define __CL_BLUE ""
  #define __CL_MAGENTA ""
  #define __CL_CYAN ""
  #define __CL_WHITE ""
  #define __CL_FG(color) ""
  #define __CL_BG(color) ""
#endif

pthread_key_t __CLEnvKey;

typedef struct __CLSpecEnv {
  void* userdata;
  bool failed;
  char* test_name;
  char* output_buffer;
  size_t num_failed_asserts;
  size_t num_passed_asserts;
} __CLSpecEnv;

typedef enum CLOptions {
  CL_OPTION_NONE          = 0 << 0,
  CL_OPTION_SKIP_SETUP    = 1 << 0,
  CL_OPTION_SKIP_TEARDOWN = 2 << 0,
} CLOptions;

typedef void* (*__CLSpecType)();
typedef void (*__CLSetupType)();

__CLSetupType __cl_setup;
__CLSetupType __cl_teardown;
__CLSetupType __cl_setup_once;
__CLSetupType __cl_teardown_once;

void __cl_env_init(__CLSpecEnv* env) {
  env->userdata = NULL;
  env->failed = false;
  env->test_name = NULL;
  env->output_buffer = malloc(16);
  env->output_buffer[0] = '\0';
  env->num_failed_asserts = 0;
  env->num_passed_asserts = 0;
}

__CLSpecEnv* __cl_env() {
  return pthread_getspecific(__CLEnvKey);
}

void __cl_vappend(const char* format, va_list args) {
  __CLSpecEnv *env = __cl_env();
  size_t cursor = 0;
  if (env->output_buffer) {
    cursor = strlen(env->output_buffer);
  }
  char* new_output = NULL;
  size_t new_output_size = vasprintf(&new_output, format, args) + 1;
  env->output_buffer = realloc(env->output_buffer, cursor + new_output_size);
  strncpy(env->output_buffer + cursor, new_output, new_output_size);
  free(new_output);
}

void __cl_append(const char* format, ...) {
  va_list args;
  va_start(args, format);
  __cl_vappend(format, args);
  va_end(args);
}

void __cl_print(const char* format, ...) {
  va_list args;
  va_start (args, format);
  vfprintf(stderr, format, args);
  va_end(args);
}

void* cl_get_userdata() {
  __CLSpecEnv* env = __cl_env();
  if (!env) {
    __cl_print(__CL_BOLD __CL_FG(__CL_YELLOW) "[ERROR] " __CL_RESET
               "Detecting call to cl_get_userdata() from outside the scope "
               "of a SPEC\n\n");
    return NULL;
  }
  return env->userdata;
}

void cl_set_userdata(void* userdata) {
  __CLSpecEnv* env = __cl_env();
  if (!env) {
    __cl_print(__CL_BOLD __CL_FG(__CL_YELLOW) "[ERROR] " __CL_RESET
               "Detecting call to cl_set_userdata() from outside the scope "
               "of a SPEC\n\n");
    return;
  }
  env->userdata = userdata;
}

void __cl_info(const char* format, ...) {
  __cl_append(__CL_BOLD __CL_FG(__CL_CYAN) "[INFO]" __CL_RESET " ");
  va_list args;
  va_start (args, format);
  __cl_vappend(format, args);
  va_end(args);
}

void __cl_error(const char* format, ...) {
  __cl_append(__CL_BOLD __CL_FG(__CL_YELLOW) __CL_TAB "[ERROR]" __CL_RESET " ");
  va_list args;
  va_start (args, format);
  __cl_vappend(format, args);
  va_end(args);
}

void __cl_pass(const char* format, ...) {
  __cl_append(__CL_BOLD __CL_FG(__CL_GREEN) __CL_TAB "[PASS] " __CL_RESET " ");
  va_list args;
  va_start (args, format);
  __cl_vappend(format, args);
  va_end(args);
}

void __cl_fail(const char* format, ...) {
  __cl_append(__CL_BOLD __CL_FG(__CL_RED) __CL_TAB "[FAIL] " __CL_RESET " ");
  va_list args;
  va_start (args, format);
  __cl_vappend(format, args);
  va_end(args);
}

void __cl_log(const char* format, ...) {
  __cl_append(__CL_BOLD __CL_FG(__CL_RED) __CL_TAB "[LOG]  " __CL_RESET " ");
  va_list args;
  va_start (args, format);
  __cl_vappend(format, args);
  va_end(args);
  __cl_append("\n");
}

double __cl_time() {
  struct timeval t; 
  gettimeofday(&t, NULL);
  return (double) t.tv_sec + t.tv_usec / 1000000.0;
}

#define cl_log(format, ...)                                                  \
do {                                                                         \
  if (!__cl_env()) {                                                         \
        __cl_print(__CL_BOLD __CL_FG(__CL_YELLOW) "[ERROR] " __CL_RESET      \
               "Detecting call to cl_log() from outside the scope "          \
               "of a SPEC at %s:%d\n\n", __FILE__, __LINE__);                \
    return;                                                                  \
  }                                                                          \
  __cl_log(format, ##__VA_ARGS__);                                           \
} while (0)                                                                  \

#define cl_abort()                                                           \
do {                                                                         \
  __CLSpecEnv* env = __cl_env();                                             \
  if (!env) {                                                                \
    __cl_print(__CL_BOLD __CL_FG(__CL_YELLOW) "[ERROR] " __CL_RESET          \
               "Detecting call to cl_abort() from outside the scope "        \
               "of a SPEC at %s:%d\n\n", __FILE__, __LINE__);                \
  }                                                                          \
  env->failed = true;                                                        \
  __cl_fail("Aborted\n\n");                                                  \
  __cl_append("======================================================\n\n"); \
  pthread_exit(NULL);                                                        \
} while(0)                                                                   \

#define cl_assert(test, format, ...)                                         \
do {                                                                         \
  __CLSpecEnv* env = __cl_env();                                             \
  if (!env) {                                                                \
    __cl_print(__CL_BOLD __CL_FG(__CL_YELLOW) "[ERROR] " __CL_RESET          \
               "Detecting call to cl_assert() from outside the scope "       \
               "of a SPEC at %s:%d\n\n", __FILE__, __LINE__);                \
  } else if (!(test)) {                                                      \
    env->num_failed_asserts++;                                               \
    /* Log the assert failure */                                             \
    __cl_error("Assertion Failed: " __CL_FG(__CL_RED) __CL_BOLD              \
               "%s" __CL_RESET " in %s:%d\n" __CL_TAB __CL_TAB, #test,       \
               __FILE__, __LINE__);                                          \
    __cl_append(__CL_FG(__CL_BLUE));                                         \
    __cl_append(format, ##__VA_ARGS__);                                      \
    __cl_append(__CL_RESET "\n\n");                                          \
  } else {                                                                   \
    env->num_passed_asserts++;                                               \
  }                                                                          \
} while(0)                                                                   \

#define cl_assert0(test) cl_assert(test, "")

#define CL_SETUP                                                             \
void __cl_setup_impl();                                                      \
__CLSetupType __cl_setup = __cl_setup_impl;                                  \
void __cl_setup_impl()                                                       \

#define CL_TEARDOWN                                                          \
void __cl_teardown_impl();                                                   \
__CLSetupType __cl_teardown = __cl_teardown_impl;                            \
void __cl_teardown_impl()                                                    \

#define CL_SETUP_ONCE                                                        \
void __cl_setup_once_impl();                                                 \
__CLSetupType __cl_setup_once = __cl_setup_once_impl;                        \
void __cl_setup_once_impl()                                                  \

#define CL_TEARDOWN_ONCE                                                     \
void __cl_teardown_once_impl();                                              \
__CLSetupType __cl_teardown_once = __cl_teardown_once_impl;                  \
void __cl_teardown_once_impl()                                               \

void* __cl_spec(
  const char* name,
  __CLSetupType spec,
  __CLSpecEnv* env,
  CLOptions options)
{
  env->test_name = (char *) name;
  pthread_setspecific(__CLEnvKey, env);
  __cl_info("Executing SPEC => " __CL_FG(__CL_CYAN) __CL_BOLD
            "%s\n\n" __CL_RESET, name);
  double time_start = __cl_time();
  if (!(options & CL_OPTION_SKIP_SETUP) && __cl_setup) {
    __cl_setup();
  }
  spec();
  if (!(options & CL_OPTION_SKIP_TEARDOWN) && __cl_teardown) {
    __cl_teardown();
  }
  double elapsed = __cl_time() - time_start;
  size_t passed = env->num_passed_asserts;
  size_t failed = env->num_failed_asserts;
  if (passed > 0 && failed == 0) {
    __cl_pass("Passed SPEC in %.4f s -> " __CL_FG(__CL_RED) "%zu fail"
      __CL_RESET ", " __CL_FG(__CL_GREEN) "%zu pass" __CL_RESET "\n\n",
      elapsed, failed, passed);
  } else {
    env->failed = true;
    __cl_fail("Failed SPEC in %.4f s -> " __CL_FG(__CL_RED) "%zu fail"
      __CL_RESET ", " __CL_FG(__CL_GREEN) "%zu pass" __CL_RESET "\n\n",
      elapsed, failed, passed);
  }
  __cl_append("======================================================\n\n");
  return NULL;
}

#define CL_SPEC(name)                                                        \
void __cl_spec_##name ();                                                    \
void* name (__CLSpecEnv* env) {                                              \
  return __cl_spec(#name, __cl_spec_##name, env, CL_OPTION_NONE);            \
}                                                                            \
void __cl_spec_##name ()                                                     \

#define CL_SPEC_OPTIONS(name, options)                                       \
void __cl_spec_##name ();                                                    \
void* name (__CLSpecEnv* env) {                                              \
  return __cl_spec(#name, __cl_spec_##name, env, options);                   \
}                                                                            \
void __cl_spec_##name ()                                                     \


int __cl_main_parallel(
  int argc,
  char** argv,
  __CLSpecType* specs,
  size_t num_specs,
  const char* filename)
{
  pthread_key_create(&__CLEnvKey, NULL);
  size_t i;
  size_t failed = 0;
  double time_start = __cl_time();
  __cl_print(__CL_BOLD __CL_FG(__CL_CYAN) "[INFO] " __CL_RESET
             "Running PARALLEL BUNDLE: %s\n\n", filename);
  if (__cl_setup_once) {
    __cl_setup_once();
  }
  /* TODO: Use a simple thread pool */
  pthread_t* threads = malloc(sizeof(pthread_t) * num_specs);
  __CLSpecEnv* envs = malloc(sizeof(__CLSpecEnv) * num_specs);
  for (i = 0; i < num_specs; i++) {
    __cl_env_init(&envs[i]);
    pthread_create(&threads[i], NULL, specs[i], &envs[i]);
  }
  for (i = 0; i < num_specs; i++) {
    pthread_join(threads[i], NULL);
    __CLSpecEnv *env = &envs[i];
    if (env->output_buffer) {
      fprintf(stderr, "%s", env->output_buffer);
      free(env->output_buffer);
    }
    if (env->failed) {
      failed++;
    }
  }
  free(threads);
  free(envs);
  if (__cl_teardown_once) {
    __cl_teardown_once();
  }
  size_t passed = num_specs - failed;
  double elapsed = __cl_time() - time_start;
  if (failed > 0) {
    __cl_print(__CL_BOLD __CL_FG(__CL_RED) "[FAILURE] " __CL_RESET);
  } else {
    __cl_print(__CL_BOLD __CL_FG(__CL_GREEN) "[SUCCESS] " __CL_RESET);
  }
  __cl_print(__CL_FG(__CL_RED) "%zu SPECS failed" __CL_RESET ", "
             __CL_FG(__CL_GREEN) "%zu SPECS passed" __CL_RESET
            " in %.4f s\n\n", failed, passed, elapsed);
  return failed;
}

int __cl_main(
  int argc,
  char** argv,
  __CLSpecType* specs,
  size_t num_specs,
  const char* filename)
{
  pthread_key_create(&__CLEnvKey, NULL);
  size_t i;
  size_t failed = 0;
  double time_start = __cl_time();
  __cl_print(__CL_BOLD __CL_FG(__CL_CYAN) "[INFO] " __CL_RESET
             "Running BUNDLE: %s\n\n", filename);
  if (__cl_setup_once) {
    __cl_setup_once();
  }
  pthread_t thread;
  __CLSpecEnv env;
  for (i = 0; i < num_specs; i++) {
    __cl_env_init(&env);
    pthread_create(&thread, NULL, specs[i], &env);
    pthread_join(thread, NULL);
    if (env.output_buffer) {
      fprintf(stderr, "%s", env.output_buffer);
      free(env.output_buffer);
      if (env.failed) {
        failed++;
      }
    }
  }
  if (__cl_teardown_once) {
    __cl_teardown_once();
  }
  size_t passed = num_specs - failed;
  double elapsed = __cl_time() - time_start;
  if (failed > 0) {
    __cl_print(__CL_BOLD __CL_FG(__CL_RED) "[FAILURE] " __CL_RESET);
  } else {
    __cl_print(__CL_BOLD __CL_FG(__CL_GREEN) "[SUCCESS] " __CL_RESET);
  }
  __cl_print(__CL_FG(__CL_RED) "%zu SPECS failed" __CL_RESET ", "
             __CL_FG(__CL_GREEN) "%zu SPECS passed" __CL_RESET
             " in %.4f s\n\n", failed, passed, elapsed);
  return failed;
}

#define CL_BUNDLE_PARALLEL(...)                                              \
int main(int argc, char** argv) {                                            \
  __CLSpecType specs[] = {__VA_ARGS__};                                      \
  size_t num_specs = sizeof(specs) / sizeof(__CLSpecType);                   \
  return __cl_main_parallel(argc, argv, specs, num_specs, __FILE__);         \
}                                                                            \

#define CL_BUNDLE(...)                                                       \
int main(int argc, char** argv) {                                            \
  __CLSpecType specs[] = {__VA_ARGS__};                                      \
  size_t num_specs = sizeof(specs) / sizeof(__CLSpecType);                   \
  return __cl_main(argc, argv, specs, num_specs, __FILE__);                  \
}                                                                            \

#endif /* CHLORINE2 */
