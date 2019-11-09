/* jc_test.h	Copyright 2016- Mathias Westerdahl
 *
 * https://github.com/JCash
 *
 * BRIEF:
 *
 * 		A tiny single header C/C++ test framework
 * 		Made sure to compile with hardest warning/error levels possible
 *
 * HISTORY:
 *
 *		0.2		2016-12-29 	Added stdbool.h. Some C99 compile warnings
 * 		0.1					Initial version
 *
 * USAGE:
 *
 *
 */

#ifndef JC_TEST_H
#define JC_TEST_H

#include <stdlib.h>
#include <stdbool.h>

#ifdef __cplusplus
#define JC_TEST_CAST(_TYPE_, _EXPR_)			reinterpret_cast< _TYPE_ >( _EXPR_ )
#define JC_TEST_STATIC_CAST(_TYPE_, _EXPR_)		static_cast< _TYPE_ >( _EXPR_ )
#else
#define JC_TEST_CAST(_TYPE_, _EXPR_)			((_TYPE_)(_EXPR_))
#define JC_TEST_STATIC_CAST(_TYPE_, _EXPR_)		((_TYPE_)(_EXPR_))
#endif

#ifndef JC_TEST_PRINTF
#include <stdio.h>
#define JC_TEST_PRINTF printf
#endif

#ifndef JC_TEST_FMT
#define JC_TEST_FMT	"%s( %d ): %s\n"
#endif

#define JC_TEST_PASS	0
#define JC_TEST_FAIL	1


#ifndef JC_TEST_TIMING_FUNC
	#if defined(_MSC_VER)
		#include <Windows.h>
	#else
		#include <sys/time.h>
	#endif
#define JC_TEST_TIMING_FUNC 		jc_test_get_time // returns micro seconds
#endif

typedef void (*jc_test_func)(void* ctx);

// Returns the user defined context for the fixture
typedef void* (*jc_fixture_setup_func)(void);


typedef struct jc_test_entry
{
	const char* 	name;
	jc_test_func	test;
} jc_test_entry;

typedef struct jc_test_stats
{
	int num_pass;
	int num_fail;
	int num_assertions;
	int num_tests;
	unsigned long long totaltime;
} jc_test_stats;

typedef struct jc_test_fixture
{
	const char* 			name;
	void*					ctx;
	jc_fixture_setup_func 	fixture_setup;
	jc_test_func			fixture_teardown;
	jc_test_func			test_setup;
	jc_test_func			test_teardown;
	jc_test_stats			stats;
	int						fail;
#ifdef __x86_64__
	int _pad;
#endif
	jc_test_entry	tests[256];
} jc_test_fixture;

typedef struct jc_test_state
{
	jc_test_stats		stats;
	int num_fixtures;
	int _padding[3];
	jc_test_fixture* 	current_fixture;
	jc_test_fixture* 	fixtures[256];
} jc_test_state;

#ifdef JC_TEST_IMPLEMENTATION
static jc_test_state jc_test_global_state = { {0, 0, 0, 0, 0}, 0, {0, 0, 0}, 0, {0} };
#else
extern jc_test_state jc_test_global_state;
#endif

#if defined(_MSC_VER)
  #pragma section(".CRT$XCU",read)
  #define JC_TEST_INITIALIZER(_NAME_) \
    static void __cdecl jc_test_global_init_##_NAME_(void); \
    __declspec(allocate(".CRT$XCU")) void (__cdecl* jc_test_global_init_##_NAME_##_)(void) = jc_test_global_init_##_NAME_; \
    static void __cdecl jc_test_global_init_##_NAME_(void)
#else
  #define JC_TEST_INITIALIZER(_NAME_) \
    static void jc_test_global_init_##_NAME_(void) __attribute__((constructor)); \
    static void jc_test_global_init_##_NAME_(void)
#endif

#ifdef __x86_64__
#define TEST_BEGIN(_NAME_, _FIXTURESETUP_, _FIXTURETEARDOWN_, _TESTSETUP_, _TESTTEARDOWN_)	\
	static jc_test_fixture __jc_test_fixture_##_NAME_ = { #_NAME_, 0, \
			JC_TEST_CAST(jc_fixture_setup_func, (_FIXTURESETUP_)), JC_TEST_CAST(jc_test_func, (_FIXTURETEARDOWN_)), \
			JC_TEST_CAST(jc_test_func, (_TESTSETUP_)), JC_TEST_CAST(jc_test_func, (_TESTTEARDOWN_)), \
			{0, 0, 0, 0, 0}, 0, 0, {
#else

#define TEST_BEGIN(_NAME_, _FIXTURESETUP_, _FIXTURETEARDOWN_, _TESTSETUP_, _TESTTEARDOWN_)  \
    static jc_test_fixture __jc_test_fixture_##_NAME_ = { #_NAME_, 0, \
            JC_TEST_CAST(jc_fixture_setup_func, (_FIXTURESETUP_)), JC_TEST_CAST(jc_test_func, (_FIXTURETEARDOWN_)), \
            JC_TEST_CAST(jc_test_func, (_TESTSETUP_)), JC_TEST_CAST(jc_test_func, (_TESTTEARDOWN_)), \
            {0, 0, 0, 0, 0}, 0, {
#endif

#define TEST_END(_NAME_)								{0, 0} } }; \
														JC_TEST_INITIALIZER(_NAME_) \
														{ \
															jc_test_global_state.fixtures[jc_test_global_state.num_fixtures] = &__jc_test_fixture_##_NAME_; \
															++jc_test_global_state.num_fixtures; \
														}


#define TEST(_TEST_)									{ #_TEST_, JC_TEST_CAST(jc_test_func, (_TEST_)) },

extern int jc_test_run_test_fixture(jc_test_fixture* fixture);
extern int jc_test_run_all_tests(jc_test_state* state);
extern void jc_test_assert(jc_test_fixture* fixture, bool cond, const char* msg);
extern unsigned long long jc_test_get_time(void);

#define TEST_RUN(_NAME_)	jc_test_run_test_fixture( & __jc_test_fixture_##_NAME_ )
#define TEST_RUN_ALL()		jc_test_run_all_tests( &jc_test_global_state )


#define TEST_ASSERT_BASE( _FILE, _LINE, _STMT, _COND, _MSG) \
	++jc_test_global_state.current_fixture->stats.num_assertions; \
	if( !(_COND) ) \
	{ \
		JC_TEST_PRINTF("\n\t" JC_TEST_FMT "\t%s( %s )\n", (_FILE), (_LINE), (_MSG), #_STMT, #_COND ); \
		jc_test_global_state.current_fixture->fail = JC_TEST_FAIL; \
		return; \
	}

#define TEST_ASSERT_BASE_EQ( _FILE, _LINE, _STMT, _A, _B, _MSG) \
	++jc_test_global_state.current_fixture->stats.num_assertions; \
	if( !((_A) == (_B)) ) \
	{ \
		JC_TEST_PRINTF("\n\t" JC_TEST_FMT "\t%s( %s, %s )\n", (_FILE), (_LINE), (_MSG), #_STMT, #_A, #_B ); \
		jc_test_global_state.current_fixture->fail = JC_TEST_FAIL; \
		return; \
	}

#define TEST_ASSERT_BASE_NE( _FILE, _LINE, _STMT, _A, _B, _MSG) \
	++jc_test_global_state.current_fixture->stats.num_assertions; \
	if( ((_A) == (_B)) ) \
	{ \
		JC_TEST_PRINTF("\n\t" JC_TEST_FMT "\t%s( %s, %s )\n", (_FILE), (_LINE), (_MSG), #_STMT, #_A, #_B ); \
		jc_test_global_state.current_fixture->fail = JC_TEST_FAIL; \
		return; \
	}

#define TEST_ASSERT_TRUE( _COND )	TEST_ASSERT_BASE( __FILE__, __LINE__, TEST_ASSERT, _COND, "" );
#define TEST_ASSERT_EQ( _A, _B)		TEST_ASSERT_BASE_EQ( __FILE__, __LINE__, TEST_ASSERT_EQ, _A, _B, "" );
#define TEST_ASSERT_NE( _A, _B)		TEST_ASSERT_BASE_NE( __FILE__, __LINE__, TEST_ASSERT_NE, _A, _B, "" );

#ifndef JC_UNDEF_SHORT_NAMES
#define RUN			TEST_RUN
#define RUN_ALL		TEST_RUN_ALL
#define ASSERT_TRUE	TEST_ASSERT_TRUE
#define ASSERT_EQ	TEST_ASSERT_EQ
#define ASSERT_NE	TEST_ASSERT_NE
#endif


#endif // JC_TEST_H

#ifdef JC_TEST_IMPLEMENTATION
#undef JC_TEST_IMPLEMENTATION

#ifdef JC_TEST_NO_COLORS
	#define JC_TEST_CLR_DEFAULT ""
	#define JC_TEST_CLR_RED  	""
	#define JC_TEST_CLR_GREEN  	""
	#define JC_TEST_CLR_YELLOW  ""
	#define JC_TEST_CLR_BLUE  	""
	#define JC_TEST_CLR_MAGENTA ""
	#define JC_TEST_CLR_CYAN  	""
	#define JC_TEST_CLR_WHITE  	""
#else
	#define JC_TEST_CLR_DEFAULT "\x1B[0m"
	#define JC_TEST_CLR_RED  	"\x1B[31m"
	#define JC_TEST_CLR_GREEN  	"\x1B[32m"
	#define JC_TEST_CLR_YELLOW  "\x1B[33m"
	#define JC_TEST_CLR_BLUE  	"\x1B[34m"
	#define JC_TEST_CLR_MAGENTA "\x1B[35m"
	#define JC_TEST_CLR_CYAN  	"\x1B[36m"
	#define JC_TEST_CLR_WHITE  	"\x1B[37m"
#endif

static void jc_test_report_time(unsigned long long t) // Micro seconds
{
#ifdef _MSC_VER
	#define JC_TEST_MICROSECONDS_STR "us"
#else
	#define JC_TEST_MICROSECONDS_STR "\u00b5s"
#endif

	if( t < 5000 )
		JC_TEST_PRINTF("%g %s", (double)t, JC_TEST_MICROSECONDS_STR);
	else if( t < 500000 )
		JC_TEST_PRINTF("%g %s", t / 1000.0, "ms");
	else
		JC_TEST_PRINTF("%g %s", t / 1000000.0, "s");
}

int jc_test_run_test_fixture(jc_test_fixture* fixture)
{
	jc_test_global_state.current_fixture = fixture;

	fixture->stats.totaltime = 0;
	unsigned long long timestart = JC_TEST_TIMING_FUNC();

	JC_TEST_PRINTF("%s%s%s\n", JC_TEST_CLR_CYAN, fixture->name, JC_TEST_CLR_DEFAULT);
	if(fixture->fixture_setup != 0)
	{
		fixture->ctx = fixture->fixture_setup();
	}

	size_t count = 0;
	jc_test_entry* test = &fixture->tests[count];
	while( test->test && count < sizeof(fixture->tests)/sizeof(fixture->tests[0]) )
	{
		fixture->fail = JC_TEST_PASS;

		JC_TEST_PRINTF("    %s", test->name);

		unsigned long long teststart = 0;
		unsigned long long testend = 0;

		jc_test_func fns[3];
		fns[0] = fixture->test_setup;
		fns[1] = test->test;
		fns[2] = fixture->test_teardown;
		for( int i = 0; i < 3; ++i )
		{
			if( !fns[i] )
			{
				continue;
			}

			if( i == 1 )
			{
				teststart = JC_TEST_TIMING_FUNC();
			}

			fns[i](fixture->ctx);

			if( i == 1 )
			{
				testend = JC_TEST_TIMING_FUNC();
			}

			if( fixture->fail != JC_TEST_PASS )
			{
				break;
			}
		}

		JC_TEST_PRINTF("\r    %s %s (", test->name, fixture->fail == JC_TEST_PASS ? (JC_TEST_CLR_GREEN "PASS" JC_TEST_CLR_DEFAULT) : (JC_TEST_CLR_RED "FAIL" JC_TEST_CLR_DEFAULT) );
		jc_test_report_time(testend - teststart);
		JC_TEST_PRINTF(")\n");

		if( fixture->fail == JC_TEST_PASS )
			++fixture->stats.num_pass;
		else
			++fixture->stats.num_fail;
		++fixture->stats.num_tests;

		++count;
		test = &fixture->tests[count];
	}

	if(fixture->fixture_teardown != 0)
	{
		fixture->fixture_teardown(fixture->ctx);
	}

	unsigned long long timeend = JC_TEST_TIMING_FUNC();
	fixture->stats.totaltime = timeend - timestart;
	JC_TEST_PRINTF("%s took ", fixture->name);
	jc_test_report_time(fixture->stats.totaltime);
	JC_TEST_PRINTF("\n");

	jc_test_global_state.current_fixture = 0;

	return fixture->stats.num_fail;
}

int jc_test_run_all_tests(jc_test_state* state)
{
	int num_fail = 0;
	state->stats.totaltime = 0;
	for( int i = 0; i < state->num_fixtures; ++i )
	{
		if( state->fixtures[i] )
		{
			num_fail += jc_test_run_test_fixture( state->fixtures[i] );

			state->stats.num_assertions += state->fixtures[i]->stats.num_assertions;
			state->stats.num_pass += state->fixtures[i]->stats.num_pass;
			state->stats.num_fail += state->fixtures[i]->stats.num_fail;
			state->stats.num_tests += state->fixtures[i]->stats.num_tests;
			state->stats.totaltime += state->fixtures[i]->stats.totaltime;
		}
	}

	JC_TEST_PRINTF("Ran %d tests, with %d assertions in ", state->stats.num_tests, state->stats.num_assertions);
	jc_test_report_time(state->stats.totaltime);
	JC_TEST_PRINTF("\n");
	if( state->stats.num_fail)
		JC_TEST_PRINTF("%d tests passed, and %d tests %sFAILED%s\n", state->stats.num_pass, state->stats.num_fail, JC_TEST_CLR_RED, JC_TEST_CLR_DEFAULT);
	else
		JC_TEST_PRINTF("%d tests %sPASSED%s\n", state->stats.num_pass, JC_TEST_CLR_GREEN, JC_TEST_CLR_DEFAULT);

	return num_fail;
}

#if defined(_MSC_VER)

unsigned long long jc_test_get_time(void)
{
	LARGE_INTEGER tickPerSecond;
	LARGE_INTEGER tick;
	QueryPerformanceFrequency(&tickPerSecond);
	QueryPerformanceCounter(&tick);
	return tick.QuadPart / (tickPerSecond.QuadPart / 1000000);
}

#else

unsigned long long jc_test_get_time(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned long long)tv.tv_sec * 1000000ULL + (unsigned long long)tv.tv_usec;
}

#endif

#endif
