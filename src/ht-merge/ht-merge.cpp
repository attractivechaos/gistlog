#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/resource.h>
#include <sys/time.h>

/*******************
 * Common routines *
 *******************/

static inline uint64_t kom_splitmix64(uint64_t *x)
{
	uint64_t z = ((*x) += 0x9e3779b97f4a7c15ULL);
	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
	z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
	return z ^ (z >> 31);
}

double kom_cputime(void)
{
	struct rusage r;
	getrusage(RUSAGE_SELF, &r);
	return r.ru_utime.tv_sec + r.ru_stime.tv_sec + 1e-6 * (r.ru_utime.tv_usec + r.ru_stime.tv_usec);
}

int64_t kom_parse_num(const char *str, char **q)
{
	double x;
	char *p;
	x = strtod(str, &p);
	if (*p == 'G' || *p == 'g') x *= 1e9, ++p;
	else if (*p == 'M' || *p == 'm') x *= 1e6, ++p;
	else if (*p == 'K' || *p == 'k') x *= 1e3, ++p;
	if (q) *q = p;
	return (int64_t)(x + .499);
}

/*****************
 * Hash function *
 *****************/

static inline uint64_t htm_hash64(uint64_t x) // from https://nullprogram.com/blog/2018/07/31/
{
	x ^= x >> 32;
	x *= 0xd6e8feb86659fd93ULL;
	x ^= x >> 32;
	x *= 0xd6e8feb86659fd93ULL;
	x ^= x >> 32;
	return x;
}

#ifdef USE_RAND
class Hasher {
	uint64_t rng, hash_seed;
public:
	Hasher(uint64_t rng_seed) {
		rng = rng_seed;
		hash_seed = kom_splitmix64(&rng);
	}
	inline size_t operator()(const uint64_t x) const {
		return htm_hash64(x ^ hash_seed);
	}
};
#else
struct Hasher {
	inline size_t operator()(const uint64_t x) const {
		return htm_hash64(x);
	}
};
#endif

/*******
 * C++ *
 *******/

#ifdef HAVE_ABSEIL
#include <absl/container/flat_hash_map.h>
#endif

#ifdef HAVE_BOOST
#include <boost/unordered/unordered_flat_map.hpp>
#endif

#include "unordered_dense.h"

template<typename T>
void htm_gen_cpp(T &h, uint32_t N, uint64_t *rng)
{
	uint32_t i;
	for (i = 0; i < N; ++i) {
		uint64_t x = kom_splitmix64(rng);
		++h[x];
	}
}

template<typename T>
void htm_merge_cpp(T &h0, const T &h1, int to_reserve)
{
	if (to_reserve)
		h0.reserve(h0.size() + h1.size());
	for (auto const &i : h1)
		h0[i.first] += i.second;
}

template<typename T>
class CppEval {
public:
	CppEval(const char *label, uint32_t N, uint64_t *rng, int no_reserve) {
#ifdef USE_RAND
		Hasher f1(42), f2(43);
		T h0(8, f1), h1(8, f2);
#else
		T h0, h1;
#endif
		htm_gen_cpp(h0, N, rng);
		double t0 = kom_cputime();
		htm_gen_cpp(h1, N*2, rng);
		double t1 = kom_cputime();
		htm_merge_cpp(h0, h1, !no_reserve);
		double t2 = kom_cputime();
		printf("%s\t%.3f\t%.3f\n", label, t1 - t0, t2 - t1);
	}
};

/**********
 * khashl *
 **********/

//#define kh_quadratic
#include "khashl.h"
KHASHL_MAP_INIT(KH_LOCAL, map64_t, map64, uint64_t, uint64_t, htm_hash64, kh_eq_generic)

map64_t *htm_gen_khashl(uint32_t N, uint64_t *rng, uint32_t seed)
{
	uint32_t i;
	map64_t *h;
	h = map64_init3(0, seed);
	for (i = 0; i < N; ++i) {
		uint64_t x = kom_splitmix64(rng);
		int absent;
		khint_t k = map64_put(h, x, &absent);
		if (absent) kh_val(h, k) = 0;
		++kh_val(h, k);
	}
	return h;
}

void htm_merge_khashl(map64_t *h0, const map64_t *h1, int to_reserve)
{
	khint_t k0, k1;
	if (to_reserve)
		map64_resize(h0, ((uint64_t)kh_size(h0) + kh_size(h1)) * 4 / 3 + 1);
	kh_foreach(h1, k1) {
		int absent;
		k0 = map64_put(h0, kh_key(h1, k1), &absent);
		if (absent) kh_val(h0, k0) = 0;
		kh_val(h0, k0) += kh_val(h1, k1);
	}
}

void htm_eval_khashl(uint32_t N, uint64_t *rng, int no_reserve)
{
	map64_t *h0 = htm_gen_khashl(N, rng, 42);
	double t0 = kom_cputime();
	map64_t *h1 = htm_gen_khashl(N*2, rng, 43);
	double t1 = kom_cputime();
	htm_merge_khashl(h0, h1, !no_reserve);
	double t2 = kom_cputime();
	printf("khashl\t%.3f\t%.3f\n", t1 - t0, t2 - t1);
	map64_destroy(h0);
	map64_destroy(h1);
}

/*****************
 * main function *
 *****************/

#include "ketopt.h"

int main(int argc, char *argv[])
{
	uint64_t rng = 11;
	uint32_t N = 19000000;
	double t0, t1, t2;
	int c, algo = 0, no_reserve = 0;
	ketopt_t o = KETOPT_INIT;

	while ((c = ketopt(&o, argc, argv, 1, "rn:a:", 0)) >= 0) {
		if (c == 'r') no_reserve = 1;
		else if (c == 'n') N = kom_parse_num(o.arg, 0);
		else if (c == 'a') algo = atoi(o.arg);
		else abort(); // unknown option
	}
	if (argc - o.ind == 0) {
		FILE *fp = stderr;
		fprintf(fp, "Usage: ht-merge [options] <algorithm>\n");
		fprintf(fp, "Options:\n");
		fprintf(fp, "  -n NUM      number of elements [19m]\n");
		fprintf(fp, "  -r          reserve before merge\n");
		fprintf(fp, "Algorithm:\n");
		fprintf(fp, "  1=khashl, 2=std, 3=unordered_dense, 4=boost, 5=abseil\n");
		return 1; 
	}
	algo = atoi(argv[o.ind]);
	if (algo == 1) htm_eval_khashl(N, &rng, no_reserve);
	else if (algo == 2) CppEval<std::unordered_map<uint64_t, uint64_t, Hasher>> run("std::unordered_map", N, &rng, no_reserve);
	else if (algo == 3) CppEval<ankerl::unordered_dense::map<uint64_t, uint64_t, Hasher>> run("unordered_dense", N, &rng, no_reserve);
#ifdef HAVE_BOOST
	else if (algo == 4) CppEval<boost::unordered_flat_map<uint64_t, uint64_t, Hasher>> run("boost", N, &rng, no_reserve);
#endif
#ifdef HAVE_ABSEIL
	else if (algo == 5) CppEval<absl::flat_hash_map<uint64_t, uint64_t, Hasher>> run("abseil", N, &rng, no_reserve);
#endif
	else abort(); // unknown algorithm
	return 0;
}
