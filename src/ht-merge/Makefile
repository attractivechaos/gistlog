CXX=g++
CXXFLAGS=-std=c++17 -O3
CPPFLAGS=
EXE=ht-merge

ifneq ($(boost),)
	CPPFLAGS+=-DHAVE_BOOST
endif
ifneq ($(abseil),)
	CPPFLAGS+=-DHAVE_ABSEIL
	LDFLAGS+=$(shell pkg-config --libs absl_flat_hash_map absl_hash)
endif
ifneq ($(rand),)
	CPPFLAGS+=-DUSE_RAND
endif
ifneq ($(default),)
	CPPFLAGS+=-DUSE_DEFAULT
endif

.PHONY:clean

ht-merge:ht-merge.cpp khashl.h unordered_dense.h stl.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -fr $(EXE) *.o *.dSYM
