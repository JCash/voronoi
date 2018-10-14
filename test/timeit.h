#pragma once

#include <vector>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <string>

#define USE_CHRONO 1
#if defined(USE_CHRONO)
#include <chrono>
#endif

#include <sys/time.h>
inline double get_time_usec()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec * 1000000.0 + (double)tv.tv_usec;
}

/*
//  Windows
#ifdef _WIN32
#include <intrin.h>
uint64_t rdtsc(){
    return __rdtsc();
}

#else
uint64_t rdtsc(){
    unsigned int lo,hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}
#endif

#ifdef __i386__
#  define RDTSC_DIRTY "%eax", "%ebx", "%ecx", "%edx"
#elif __x86_64__
#  define RDTSC_DIRTY "%rax", "%rbx", "%rcx", "%rdx"
#else
# error unknown platform
#endif

#define RDTSC_START(cycles)                                \
    do {                                                   \
        register unsigned cyc_high, cyc_low;               \
        asm volatile("CPUID\n\t"                           \
                     "RDTSC\n\t"                           \
                     "mov %%edx, %0\n\t"                   \
                     "mov %%eax, %1\n\t"                   \
                     : "=r" (cyc_high), "=r" (cyc_low)     \
                     :: RDTSC_DIRTY);                      \
        (cycles) = ((uint64_t)cyc_high << 32) | cyc_low;   \
    } while (0)

#define RDTSC_STOP(cycles)                                 \
    do {                                                   \
        register unsigned cyc_high, cyc_low;               \
        asm volatile("RDTSCP\n\t"                          \
                     "mov %%edx, %0\n\t"                   \
                     "mov %%eax, %1\n\t"                   \
                     "CPUID\n\t"                           \
                     : "=r" (cyc_high), "=r" (cyc_low)     \
                     :: RDTSC_DIRTY);                      \
        (cycles) = ((uint64_t)cyc_high << 32) | cyc_low;   \
    } while(0)
*/

/*
// https://idea.popcount.org/2013-01-28-counting-cycles---rdtsc/
inline uint64_t rdtsc_start()
{
	uint64_t counter = 0;
    do {
        register unsigned cyc_high, cyc_low;
        asm volatile("CPUID\n\t"
                     "RDTSC\n\t"
                     "mov %%edx, %0\n\t"
                     "mov %%eax, %1\n\t"
                     : "=r" (cyc_high), "=r" (cyc_low)
                     :: RDTSC_DIRTY);
        counter = ((uint64_t)cyc_high << 32) | cyc_low;
    } while (0);
    return counter;
}

inline uint64_t rdtsc_stop()
{
	uint64_t counter;
    do {
        register unsigned cyc_high, cyc_low;
        asm volatile("RDTSCP\n\t"
                     "mov %%edx, %0\n\t"
                     "mov %%eax, %1\n\t"
                     "CPUID\n\t"
                     : "=r" (cyc_high), "=r" (cyc_low)
                     :: RDTSC_DIRTY);
        counter = ((uint64_t)cyc_high << 32) | cyc_low;
    } while(0);
    return counter;
}

// http://stackoverflow.com/questions/850774/how-to-determine-the-hardware-cpu-and-ram-on-a-machine
*/

class CTimeIt
{
#if defined(USE_CHRONO)
	std::chrono::duration<double> m_Average;
	std::chrono::duration<double> m_Median;
	std::chrono::duration<double> m_Min;
	std::chrono::duration<double> m_Max;
#else
	double m_Average;
	double m_Median;
	double m_Min;
	double m_Max;
#endif
	size_t m_Count;
public:

	template<typename FuncResult, typename SetupFunc, typename Func, typename... FuncArgs>
	FuncResult run(size_t count, SetupFunc setupfunc, Func func, FuncArgs&... args)
	{
		m_Count = count;

#if defined(USE_CHRONO)
		std::vector<std::chrono::duration<double> > times;
#else
		std::vector<double> times;
#endif
		times.resize(m_Count);

		FuncResult result = FuncResult();
		for( size_t i = 0; i < m_Count; ++i )
		{
			setupfunc(args...);
			
#if defined(USE_CHRONO)
			auto tstart = std::chrono::high_resolution_clock::now();
#else
			auto tstart = get_time_usec();
#endif

			result = func(args...);

#if defined(USE_CHRONO)
			auto tend = std::chrono::high_resolution_clock::now();
#else
			auto tend = get_time_usec();
#endif

			times[i] = tend - tstart;
		}

		calc_times( times );

		return result;
	}

	/**
	 * @return Returns the average (in seconds)
	 */
	float average() const
	{
#if defined(USE_CHRONO)
		return m_Average.count();
#else
		return m_Average / 1000000.0;
#endif
	}

	/**
	 * @return Returns the median (in seconds)
	 */
	float median() const
	{
#if defined(USE_CHRONO)
		return m_Median.count();
#else
		return m_Median / 1000000.0;
#endif
	}

	/**
	 * @return Returns the fastest run (in seconds)
	 */
	float fastest() const
	{
#if defined(USE_CHRONO)
		return m_Min.count();
#else
		return m_Min / 1000000.0;
#endif
	}

	/**
	 * @return Returns the longest run (in seconds)
	 */
	float longest() const
	{
#if defined(USE_CHRONO)
		return m_Max.count();
#else
		return m_Max / 1000000.0;
#endif
	}

	static void report_time(std::ostream& stream, float t, float multiplier)
	{
		if( multiplier == 0.0f )
		{
			if( t < 0.000001f )
				stream << t * 1000000000.0 << " ns";
			else if( t < 0.001f )
				stream << t * 1000000.0 << " \u00b5s";
			else if( t < 0.1f )
				stream << t * 1000.0 << " ms";
			else
				stream << t << " s";
		}
		else
		{
			if( multiplier == 1000000000.0f )
				stream << t * multiplier << " ns";
			else if( multiplier == 1000000.0f )
				stream << t * multiplier << " \u00b5s";
			else if( multiplier == 1000.0f )
				stream << t * multiplier << " ms";
			else if( multiplier == 1.0f )
				stream << t * multiplier << " s";
			else
				stream << t * multiplier;
		}
	}

	void report(std::ostream& stream, const std::string& title, float multiplier = 0.0f) const
	{
		stream << std::fixed << std::setprecision(3);
		stream << title << "\titerations: " << m_Count;
		stream << "\tavg: "; report_time(stream, average(), multiplier);
		stream << "\tmedian: "; report_time(stream, median(), multiplier);
		stream << "\tmin: "; report_time(stream, fastest(), multiplier);
		stream << "\tmax: "; report_time(stream, longest(), multiplier);
		stream << std::endl;
	}

private:
	

#if defined(USE_CHRONO)
	void calc_times(std::vector<std::chrono::duration<double> >& times)
	{
		std::sort( times.begin(), times.end() );

		std::chrono::duration<float> total(0);
		for( size_t i = 0; i < m_Count; ++i )
		{
			total += times[i];
		}

		m_Average = total / (double)m_Count;
		size_t middle = m_Count / 2;
		if( m_Count & 1 )
			m_Median = times[middle];
		else
			m_Median = (times[middle-1] + times[middle]) / 2.0f;

		m_Min = times[0];
		m_Max = times[times.size()-1];
	}
#else
	void calc_times(std::vector<double>& times)
	{
		std::sort( times.begin(), times.end() );

		double total = 0;
		for( size_t i = 0; i < m_Count; ++i )
		{
			total += times[i];
		}

		m_Average = total / (float)m_Count;
		size_t middle = m_Count / 2;
		if( m_Count & 1 )
			m_Median = times[middle];
		else
			m_Median = (times[middle-1] + times[middle]) / 2.0f;

		m_Min = times[0];
		m_Max = times[times.size()-1];
	}
#endif
};

/*
struct TimeScope
{
	const char* name;
	std::chrono::high_resolution_clock::time_point start;
	TimeScope(const char* _name) : name(_name), start(std::chrono::high_resolution_clock::now()) {}
	~TimeScope()
	{
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<float> t = end - start;
		std::cout << std::fixed << std::setprecision(3);
		std::cout << name << " took "; CTimeIt::report_time( std::cout, t.count(), 0 ); std::cout << std::endl;
	}
};
*/

#define TIMESCOPE( _NAME_ )	struct TimeScope __timescope##_NAME_( #_NAME_ )
