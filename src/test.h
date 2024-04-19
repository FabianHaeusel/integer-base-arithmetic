#ifndef TEST_H
#define TEST_H

#include <stdbool.h>

#include "implementations.h"

struct TestResult {
    int n_passed;
    int n_tests;
    const char *title;
};

typedef struct TestResult TestResult;

/**
 * @brief Runs all tests with the specified implementation
 *
 * @param impl  Implementation to be tested
 */
void test_impl(Implementation impl);

/**
 * @short Runs all tests with all implementations
 */
void test_all_impls();

/**
 * @brief A function that executes a test given a testcase
 *
 * @return True if the test passed, otherwise false
 */
typedef bool test_executor(void *testcase);

/**
 * @brief A function that executes a test given an environment and a testcase
 *
 * @return True if the test passed, otherwise false
 */
typedef bool test_executor_env(void *env, void *testcase);

/**
 * @brief Runs a test given an environment and a testcase
 *
 * @param env           Pointer to an object representing the test environment
 * @param testcase      Pointer to an object representing the testcase
 * @param executor      Pointer to a function which executes the actual test given the parameters
 *                      env and testcase
 * @param n_passed      Pointer to the current tests TestResult
 * @param testcase_fmt  Message describing the testcase
 * @param actual_fmt    Message describing the actual result
 * @param ...           Arguments for the two format strings
 */
void test_run_with_env(void *env, void *testcase, test_executor_env executor, TestResult *tr,
                       char *testcase_fmt, char *actual_fmt, ...);

/**
 * @brief Runs a test given a testcase
 *
 * @param testcase      Pointer to an object representing the testcase
 * @param executor      Pointer to a function which executes the actual test given the parameter
 *                      testcase
 * @param n_passed      Pointer to the current tests TestResult
 * @param testcase_fmt  Message describing the testcase
 * @param actual_fmt    Message describing the actual result
 * @param ...           Arguments for the two format strings
 */
void test_run(void *testcase, test_executor executor, TestResult *tr, char *testcase_fmt,
              char *actual_fmt, ...);

/**
 * @brief Prints a message indicating the start of a test
 *
 * @param module            The module under test
 * @param description_fmt   A description of the test
 * @param ...               Arguments for the format string
 */
TestResult test_init(const char *module, char *description_fmt, ...);

/**
 * @brief Prints a message indicating the start of a test
 *
 * @param impl              The Implementation under test
 * @param description_fmt   A description of the test
 * @param ...               Arguments for the format string
 */
TestResult test_init_impl(Implementation impl, char *description_fmt, ...);

/**
 * @brief Prints the result of a test
 *
 * @param tr    The test result
 */
void test_finalize(TestResult tr);

#endif