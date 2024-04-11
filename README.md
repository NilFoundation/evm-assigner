# evm-assigner

Simulator of Ethereum Virtual Machine pipeline for fill assignment table associate with evm1 circuit.

Based on a C++ implementation of the Ethereum Virtual Machine (EVM).
Created by members of the [Ipsilon] (ex-[Ewasm]) team.

## Dependencies

### Build tools

- CMake 3.16+
- Clang or GCC (tested with GCC 13)
- Ninja (recommended)

### Libraries

- [evmc](https://github.com/ethereum/evmc)
- [intx](https://github.com/chfast/intx)
- [blueprint](https://github.com/NilFoundation/zkllvm-blueprint)
- [crypto3](https://github.com/NilFoundation/crypto3)

## Nix support

`evm-assigner` supports [Nix](https://nixos.org/) flakes. This means that you can get development-ready environment with:

```bash
nix develop
```

Build package:

```bash
nix build
```

Run tests:

```bash
nix flake check
```

## Build

### Configure cmake

```bash
cmake -G "Ninja" -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_ASSIGNER_TESTS=TRUE

```

### Build test

```bash
cmake --build build -t assigner_tests
```
