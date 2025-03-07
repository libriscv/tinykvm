#include <malloc.h>
#include <stdio.h>
#include <string.h>
static void test_threads();

static long nprimes = 0;

__attribute__((noinline))
void test_ud2()
{
	asm("ud2");
}

int main(int argc, char** argv)
{
	printf("Hello musl World!\n");

	static const int N = 1000000;
	char prime[N];
	memset(prime, 1, sizeof(prime));
	for (long n = 2; n < N; n++)
	{
		if (prime[n]) {
			nprimes += 1;
			for (long i = n*n; i < N; i += n)
				prime[i] = 0;
		}
	}

	test_threads();
	//test_ud2();
	return 0;
}

#include <assert.h>
static int t = 0;

__attribute__((used))
void bench()
{
	//assert(t == 0);
	//t = 1;
	assert(nprimes == 78498);
}

__attribute__((used))
void bench_write()
{
	assert(t == 0);
	t = 1;
	assert(nprimes == 78498);
}

asm(".global one_vmexit\n" \
".type one_vmexit, function\n" \
"one_vmexit:\n" \
"	out %ax, $1\n" \
"	ret\n");
extern void one_vmexit();

__attribute__((used))
void bench_vmexits(int count)
{
	while (count--) one_vmexit();
}

#include <assert.h>
#include <pthread.h>

static void* thread_function1(void* data)
{
	printf("Inside thread function1, x = %d\n", *(int*) data);
	static __thread int test = 2021;
	printf("test @ %p, test = %d\n", &test, test);
	assert(test == 2021);
	fflush(stdout);
	pthread_exit(NULL);
}
static void* thread_function2(void* data)
{
	printf("Inside thread function2, x = %d\n", *(int*) data);
	static __thread int test = 2022;
	assert(test == 2022);

	printf("Yielding from thread2, expecting to be returned to main thread\n");
	sched_yield();
	printf("Returned to thread2, expecting to exit to after main thread yield\n");

	pthread_exit(NULL);
}

void test_threads()
{
	int x = 666;
	pthread_t t1;
	pthread_t t2;
	int res;

	printf("*** Testing pthread_create and sched_yield...\n");
	res = pthread_create(&t1, NULL, thread_function1, &x);
	if (res < 0) {
		printf("Failed to create thread!\n");
		return;
	}
	pthread_join(t1, NULL);

	res = pthread_create(&t2, NULL, thread_function2, &x);
	if (res < 0) {
		printf("Failed to create thread!\n");
		return;
	}

	printf("Yielding from main thread, expecting to return to thread2\n");
	// return back to finish thread2
	sched_yield();
	printf("After yielding from main thread, looking good!\n");
	// remove the thread
	pthread_join(t2, NULL);

	printf("SUCCESS\n");
}
