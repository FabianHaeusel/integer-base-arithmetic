#include "test.h"

#include <stdarg.h>
#include <stdio.h>

#include "implementations/impl_tests.h"

static TestResult tr_static = {0, 0, ""};

void test_run_with_env(void *env, void *testcase, bool executor(void *, void *), TestResult *tr,
                       char *testcase_fmt, char *actual_fmt, ...) {
    tr->n_tests++;

    va_list args;
    va_start(args, actual_fmt);

#ifdef ANNOUNCE_TESTS
    vprintf(testcase_fmt, args);
    fflush(stdout);
#endif
    if (executor(env, testcase)) {
        tr->n_passed++;
#ifdef ANNOUNCE_TESTS
        printf(": passed\n");
#endif
    } else {
#ifndef ANNOUNCE_TESTS
        vprintf(testcase_fmt, args);
#endif
        printf(": failed (");
        vprintf(actual_fmt, args);
        printf(")\n");
    }

    va_end(args);
}

void test_run(void *testcase, bool executor(void *), TestResult *tr, char *testcase_fmt,
              char *actual_fmt, ...) {
    tr->n_tests++;

    va_list args;
    va_start(args, actual_fmt);

#ifdef ANNOUNCE_TESTS
    vprintf(testcase_fmt, args);
    fflush(stdout);
#endif
    if (executor(testcase)) {
        tr->n_passed++;
#ifdef ANNOUNCE_TESTS
        printf(": passed\n");
#endif
    } else {
#ifndef ANNOUNCE_TESTS
        vprintf(testcase_fmt, args);
#endif
        printf(": failed (");
        vprintf(actual_fmt, args);
        printf(")\n");
    }

    va_end(args);
}

static void print_test_start_internal(const char *module, char *fmt, va_list args) {
    printf("Testing [%s]: ", module);
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

TestResult test_init(const char *module, char *description_fmt, ...) {
    va_list args;
    va_start(args, description_fmt);
    print_test_start_internal(module, description_fmt, args);
    va_end(args);
    TestResult tr = {0, 0, module};
    return tr;
}

TestResult test_init_impl(Implementation impl, char *description_fmt, ...) {
    va_list args;
    va_start(args, description_fmt);
    print_test_start_internal(impl.name, description_fmt, args);
    va_end(args);
    TestResult tr = {0, 0, impl.name};
    return tr;
}

void print_result(TestResult tr) {
    if (tr.n_tests != 0) {
        printf("[%s] ", tr.title);
        if (tr.n_passed == tr.n_tests) {
            printf("All tests passed (%i).\n", tr.n_tests);
        } else {
            printf("%i/%i tests passed.\n", tr.n_passed, tr.n_tests);
        }
    }
}

void test_finalize(TestResult tr) {
    if (tr.n_passed != tr.n_tests) {
        print_result(tr);
    }

    tr_static.n_tests += tr.n_tests;
    tr_static.n_passed += tr.n_passed;
}

void test_impl(Implementation impl) {
    tr_static.n_tests = 0;
    tr_static.n_passed = 0;
    tr_static.title = impl.name;

    if (impl.test != NULL) {
        impl.test(impl);
    }
    impl_tests_test(impl);

    print_result(tr_static);
}

void test_all_impls() {
    int n_tests = 0;
    int n_passed = 0;

    for (size_t i = 0; i < IMPLEMENTATIONS_COUNT; i++) {
        test_impl(implementations[i]);
        n_tests += tr_static.n_tests;
        n_passed += tr_static.n_passed;
    }

    tr_static.n_tests = 0;
    tr_static.n_passed = 0;
    tr_static.title = "all";

    impl_tests_test_all();

    print_result(tr_static);

    tr_static.n_tests += n_tests;
    tr_static.n_passed += n_passed;
    tr_static.title = "Total";

    print_result(tr_static);
}
