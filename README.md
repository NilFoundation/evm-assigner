# evm1_assigner

Simulator of Ethereum Virtual Machine pipeline for fill assignment table associate with evm1 circuit.

Based on a C++ implementation of the Ethereum Virtual Machine (EVM).
Created by members of the [Ipsilon] (ex-[Ewasm]) team.

## Dependencies

- [blueprint](https://github.com/NilFoundation/zkllvm-blueprint)
- [crypto3](https://github.com/NilFoundation/crypto3)

## Build

### Configure cmake

```
cmake -G "Ninja" -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_ASSIGNER_TESTS=TRUE

```

### Build test

```
cmake --build build -t assigner_tests
```
