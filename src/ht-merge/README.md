To compile:
```sh
# compile with std::unordered_map, ankerl::unordered_dense::map and khashl
make

# additionally compile with absl::flat_hash_map and boost::unordered_flat_map
make boost=1 abseil=1

# enable salted hasher in the user space
make boost=1 abseil=1 rand=1

# use the default hasher in each library (overriding rand=1)
make boost=1 abseil=1 default=1
```

To run:
```sh
# run without preallocation
./ht-merge 3

# run with preallocation
./ht-merge 3 -r

# set N=10 million
./ht-merge 3 -n 10m
```
