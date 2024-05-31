// evmone: Fast Ethereum Virtual Machine implementation
// Copyright 2022 The evmone Authors.
// SPDX-License-Identifier: Apache-2.0
#pragma once

/// The default macro for ON_OPCODE_IDENTIFIER. It redirects to ON_OPCODE.
#define ON_OPCODE_IDENTIFIER_DEFAULT(OPCODE, NAME) ON_OPCODE(OPCODE)

/// The default macro for ON_OPCODE_UNDEFINED. Empty implementation to ignore undefined opcodes.
#define ON_OPCODE_UNDEFINED_DEFAULT(OPCODE)


#define ON_OPCODE_IDENTIFIER ON_OPCODE_IDENTIFIER_DEFAULT
#define ON_OPCODE_UNDEFINED ON_OPCODE_UNDEFINED_DEFAULT


/// The "X Macro" for opcodes and their matching identifiers.
///
/// The MAP_OPCODES is an extended variant of X Macro idiom.
/// It has 3 knobs for users.
///
/// 1. The ON_OPCODE(OPCODE) macro must be defined. It will receive all defined opcodes from
///    the evmc_opcode enum.
/// 2. The ON_OPCODE_UNDEFINED(OPCODE) macro may be defined to receive
///    the values of all undefined opcodes.
///    This macro is by default alias to ON_OPCODE_UNDEFINED_DEFAULT therefore users must first
///    undef it and restore the alias after usage.
/// 3. The ON_OPCODE_IDENTIFIER(OPCODE, IDENTIFIER) macro may be defined to receive
///    the pairs of all defined opcodes and their matching identifiers.
///    This macro is by default alias to ON_OPCODE_IDENTIFIER_DEFAULT therefore users must first
///    undef it and restore the alias after usage.
///
/// See for more about X Macros: https://en.wikipedia.org/wiki/X_Macro.
#define MAP_OPCODES                                         \
    ON_OPCODE_IDENTIFIER(OP_STOP, operation<BlueprintFieldType>::stop)                     \
    ON_OPCODE_IDENTIFIER(OP_ADD, operation<BlueprintFieldType>::add)                       \
    ON_OPCODE_IDENTIFIER(OP_MUL, operation<BlueprintFieldType>::mul)                       \
    ON_OPCODE_IDENTIFIER(OP_SUB, operation<BlueprintFieldType>::sub)                       \
    ON_OPCODE_IDENTIFIER(OP_DIV, operation<BlueprintFieldType>::div)                       \
    ON_OPCODE_IDENTIFIER(OP_SDIV, operation<BlueprintFieldType>::sdiv)                     \
    ON_OPCODE_IDENTIFIER(OP_MOD, operation<BlueprintFieldType>::mod)                       \
    ON_OPCODE_IDENTIFIER(OP_SMOD, operation<BlueprintFieldType>::smod)                     \
    ON_OPCODE_IDENTIFIER(OP_ADDMOD, operation<BlueprintFieldType>::addmod)                 \
    ON_OPCODE_IDENTIFIER(OP_MULMOD, operation<BlueprintFieldType>::mulmod)                 \
    ON_OPCODE_IDENTIFIER(OP_EXP, operation<BlueprintFieldType>::exp)                       \
    ON_OPCODE_IDENTIFIER(OP_SIGNEXTEND, operation<BlueprintFieldType>::signextend)         \
    ON_OPCODE_UNDEFINED(0x0c)                               \
    ON_OPCODE_UNDEFINED(0x0d)                               \
    ON_OPCODE_UNDEFINED(0x0e)                               \
    ON_OPCODE_UNDEFINED(0x0f)                               \
                                                            \
    ON_OPCODE_IDENTIFIER(OP_LT, operation<BlueprintFieldType>::lt)                         \
    ON_OPCODE_IDENTIFIER(OP_GT, operation<BlueprintFieldType>::gt)                         \
    ON_OPCODE_IDENTIFIER(OP_SLT, operation<BlueprintFieldType>::slt)                       \
    ON_OPCODE_IDENTIFIER(OP_SGT, operation<BlueprintFieldType>::sgt)                       \
    ON_OPCODE_IDENTIFIER(OP_EQ, operation<BlueprintFieldType>::eq)                         \
    ON_OPCODE_IDENTIFIER(OP_ISZERO, operation<BlueprintFieldType>::iszero)                 \
    ON_OPCODE_IDENTIFIER(OP_AND, operation<BlueprintFieldType>::and_)                      \
    ON_OPCODE_IDENTIFIER(OP_OR, operation<BlueprintFieldType>::or_)                        \
    ON_OPCODE_IDENTIFIER(OP_XOR, operation<BlueprintFieldType>::xor_)                      \
    ON_OPCODE_IDENTIFIER(OP_NOT, operation<BlueprintFieldType>::not_)                      \
    ON_OPCODE_IDENTIFIER(OP_BYTE, operation<BlueprintFieldType>::byte)                     \
    ON_OPCODE_IDENTIFIER(OP_SHL, operation<BlueprintFieldType>::shl)                       \
    ON_OPCODE_IDENTIFIER(OP_SHR, operation<BlueprintFieldType>::shr)                       \
    ON_OPCODE_IDENTIFIER(OP_SAR, operation<BlueprintFieldType>::sar)                       \
    ON_OPCODE_UNDEFINED(0x1e)                               \
    ON_OPCODE_UNDEFINED(0x1f)                               \
                                                            \
    ON_OPCODE_IDENTIFIER(OP_KECCAK256, operation<BlueprintFieldType>::keccak256)           \
    ON_OPCODE_UNDEFINED(0x21)                               \
    ON_OPCODE_UNDEFINED(0x22)                               \
    ON_OPCODE_UNDEFINED(0x23)                               \
    ON_OPCODE_UNDEFINED(0x24)                               \
    ON_OPCODE_UNDEFINED(0x25)                               \
    ON_OPCODE_UNDEFINED(0x26)                               \
    ON_OPCODE_UNDEFINED(0x27)                               \
    ON_OPCODE_UNDEFINED(0x28)                               \
    ON_OPCODE_UNDEFINED(0x29)                               \
    ON_OPCODE_UNDEFINED(0x2a)                               \
    ON_OPCODE_UNDEFINED(0x2b)                               \
    ON_OPCODE_UNDEFINED(0x2c)                               \
    ON_OPCODE_UNDEFINED(0x2d)                               \
    ON_OPCODE_UNDEFINED(0x2e)                               \
    ON_OPCODE_UNDEFINED(0x2f)                               \
                                                            \
    ON_OPCODE_IDENTIFIER(OP_ADDRESS, operation<BlueprintFieldType>::address)               \
    ON_OPCODE_IDENTIFIER(OP_BALANCE, operation<BlueprintFieldType>::balance)               \
    ON_OPCODE_IDENTIFIER(OP_ORIGIN, operation<BlueprintFieldType>::origin)                 \
    ON_OPCODE_IDENTIFIER(OP_CALLER, operation<BlueprintFieldType>::caller)                 \
    ON_OPCODE_IDENTIFIER(OP_CALLVALUE, operation<BlueprintFieldType>::callvalue)           \
    ON_OPCODE_IDENTIFIER(OP_CALLDATALOAD, operation<BlueprintFieldType>::calldataload)     \
    ON_OPCODE_IDENTIFIER(OP_CALLDATASIZE, operation<BlueprintFieldType>::calldatasize)     \
    ON_OPCODE_IDENTIFIER(OP_CALLDATACOPY, operation<BlueprintFieldType>::calldatacopy)     \
    ON_OPCODE_IDENTIFIER(OP_CODESIZE, operation<BlueprintFieldType>::codesize)             \
    ON_OPCODE_IDENTIFIER(OP_CODECOPY, operation<BlueprintFieldType>::codecopy)             \
    ON_OPCODE_IDENTIFIER(OP_GASPRICE, operation<BlueprintFieldType>::gasprice)             \
    ON_OPCODE_IDENTIFIER(OP_EXTCODESIZE, operation<BlueprintFieldType>::extcodesize)       \
    ON_OPCODE_IDENTIFIER(OP_EXTCODECOPY, operation<BlueprintFieldType>::extcodecopy)       \
    ON_OPCODE_IDENTIFIER(OP_RETURNDATASIZE, operation<BlueprintFieldType>::returndatasize) \
    ON_OPCODE_IDENTIFIER(OP_RETURNDATACOPY, operation<BlueprintFieldType>::returndatacopy) \
    ON_OPCODE_IDENTIFIER(OP_EXTCODEHASH, operation<BlueprintFieldType>::extcodehash)       \
                                                            \
    ON_OPCODE_IDENTIFIER(OP_BLOCKHASH, operation<BlueprintFieldType>::blockhash)           \
    ON_OPCODE_IDENTIFIER(OP_COINBASE, operation<BlueprintFieldType>::coinbase)             \
    ON_OPCODE_IDENTIFIER(OP_TIMESTAMP, operation<BlueprintFieldType>::timestamp)           \
    ON_OPCODE_IDENTIFIER(OP_NUMBER, operation<BlueprintFieldType>::number)                 \
    ON_OPCODE_IDENTIFIER(OP_PREVRANDAO, operation<BlueprintFieldType>::prevrandao)         \
    ON_OPCODE_IDENTIFIER(OP_GASLIMIT, operation<BlueprintFieldType>::gaslimit)             \
    ON_OPCODE_IDENTIFIER(OP_CHAINID, operation<BlueprintFieldType>::chainid)               \
    ON_OPCODE_IDENTIFIER(OP_SELFBALANCE, operation<BlueprintFieldType>::selfbalance)       \
    ON_OPCODE_IDENTIFIER(OP_BASEFEE, operation<BlueprintFieldType>::basefee)               \
    ON_OPCODE_IDENTIFIER(OP_BLOBHASH, operation<BlueprintFieldType>::blobhash)             \
    ON_OPCODE_IDENTIFIER(OP_BLOBBASEFEE, operation<BlueprintFieldType>::blobbasefee)       \
    ON_OPCODE_UNDEFINED(0x4b)                               \
    ON_OPCODE_UNDEFINED(0x4c)                               \
    ON_OPCODE_UNDEFINED(0x4d)                               \
    ON_OPCODE_UNDEFINED(0x4e)                               \
    ON_OPCODE_UNDEFINED(0x4f)                               \
                                                            \
    ON_OPCODE_IDENTIFIER(OP_POP, operation<BlueprintFieldType>::pop)                       \
    ON_OPCODE_IDENTIFIER(OP_MLOAD, operation<BlueprintFieldType>::template mload<typename nil::blueprint::zkevm_word<BlueprintFieldType>::value_type>)          \
    ON_OPCODE_IDENTIFIER(OP_MLOAD8, operation<BlueprintFieldType>::template mload<uint8_t>)         \
    ON_OPCODE_IDENTIFIER(OP_MLOAD16, operation<BlueprintFieldType>::template mload<uint16_t>)       \
    ON_OPCODE_IDENTIFIER(OP_MLOAD32, operation<BlueprintFieldType>::template mload<uint32_t>)       \
    ON_OPCODE_IDENTIFIER(OP_MLOAD64, operation<BlueprintFieldType>::template mload<uint64_t>)       \
    ON_OPCODE_IDENTIFIER(OP_MSTORE, operation<BlueprintFieldType>::template mstore<typename nil::blueprint::zkevm_word<BlueprintFieldType>::value_type>)        \
    ON_OPCODE_IDENTIFIER(OP_MSTORE8, operation<BlueprintFieldType>::template mstore<uint8_t>)       \
    ON_OPCODE_IDENTIFIER(OP_MSTORE16, operation<BlueprintFieldType>::template mstore<uint16_t>)     \
    ON_OPCODE_IDENTIFIER(OP_MSTORE32, operation<BlueprintFieldType>::template mstore<uint32_t>)     \
    ON_OPCODE_IDENTIFIER(OP_MSTORE64, operation<BlueprintFieldType>::template mstore<uint64_t>)     \
    ON_OPCODE_IDENTIFIER(OP_SLOAD, operation<BlueprintFieldType>::sload)                   \
    ON_OPCODE_IDENTIFIER(OP_SSTORE, operation<BlueprintFieldType>::sstore)                 \
    ON_OPCODE_IDENTIFIER(OP_JUMP, operation<BlueprintFieldType>::jump)                     \
    ON_OPCODE_IDENTIFIER(OP_JUMPI, operation<BlueprintFieldType>::jumpi)                   \
    ON_OPCODE_IDENTIFIER(OP_PC, operation<BlueprintFieldType>::pc)                         \
    ON_OPCODE_IDENTIFIER(OP_MSIZE, operation<BlueprintFieldType>::msize)                   \
    ON_OPCODE_IDENTIFIER(OP_GAS, operation<BlueprintFieldType>::gas)                       \
    ON_OPCODE_IDENTIFIER(OP_JUMPDEST, operation<BlueprintFieldType>::jumpdest)             \
    ON_OPCODE_IDENTIFIER(OP_TLOAD, operation<BlueprintFieldType>::tload)                   \
    ON_OPCODE_IDENTIFIER(OP_TSTORE, operation<BlueprintFieldType>::tstore)                 \
    ON_OPCODE_IDENTIFIER(OP_MCOPY, operation<BlueprintFieldType>::mcopy)                   \
    ON_OPCODE_IDENTIFIER(OP_PUSH0, operation<BlueprintFieldType>::push0)                   \
                                                            \
    ON_OPCODE_IDENTIFIER(OP_PUSH1, operation<BlueprintFieldType>::template push<1>)                 \
    ON_OPCODE_IDENTIFIER(OP_PUSH2, operation<BlueprintFieldType>::template push<2>)                 \
    ON_OPCODE_IDENTIFIER(OP_PUSH3, operation<BlueprintFieldType>::template push<3>)                 \
    ON_OPCODE_IDENTIFIER(OP_PUSH4, operation<BlueprintFieldType>::template push<4>)                 \
    ON_OPCODE_IDENTIFIER(OP_PUSH5, operation<BlueprintFieldType>::template push<5>)                 \
    ON_OPCODE_IDENTIFIER(OP_PUSH6, operation<BlueprintFieldType>::template push<6>)                 \
    ON_OPCODE_IDENTIFIER(OP_PUSH7, operation<BlueprintFieldType>::template push<7>)                 \
    ON_OPCODE_IDENTIFIER(OP_PUSH8, operation<BlueprintFieldType>::template push<8>)                 \
    ON_OPCODE_IDENTIFIER(OP_PUSH9, operation<BlueprintFieldType>::template push<9>)                 \
    ON_OPCODE_IDENTIFIER(OP_PUSH10, operation<BlueprintFieldType>::template push<10>)               \
    ON_OPCODE_IDENTIFIER(OP_PUSH11, operation<BlueprintFieldType>::template push<11>)               \
    ON_OPCODE_IDENTIFIER(OP_PUSH12, operation<BlueprintFieldType>::template push<12>)               \
    ON_OPCODE_IDENTIFIER(OP_PUSH13, operation<BlueprintFieldType>::template push<13>)               \
    ON_OPCODE_IDENTIFIER(OP_PUSH14, operation<BlueprintFieldType>::template push<14>)               \
    ON_OPCODE_IDENTIFIER(OP_PUSH15, operation<BlueprintFieldType>::template push<15>)               \
                                                            \
    ON_OPCODE_IDENTIFIER(OP_PUSH16, operation<BlueprintFieldType>::template push<16>)               \
    ON_OPCODE_IDENTIFIER(OP_PUSH17, operation<BlueprintFieldType>::template push<17>)               \
    ON_OPCODE_IDENTIFIER(OP_PUSH18, operation<BlueprintFieldType>::template push<18>)               \
    ON_OPCODE_IDENTIFIER(OP_PUSH19, operation<BlueprintFieldType>::template push<19>)               \
    ON_OPCODE_IDENTIFIER(OP_PUSH20, operation<BlueprintFieldType>::template push<20>)               \
    ON_OPCODE_IDENTIFIER(OP_PUSH21, operation<BlueprintFieldType>::template push<21>)               \
    ON_OPCODE_IDENTIFIER(OP_PUSH22, operation<BlueprintFieldType>::template push<22>)               \
    ON_OPCODE_IDENTIFIER(OP_PUSH23, operation<BlueprintFieldType>::template push<23>)               \
    ON_OPCODE_IDENTIFIER(OP_PUSH24, operation<BlueprintFieldType>::template push<24>)               \
    ON_OPCODE_IDENTIFIER(OP_PUSH25, operation<BlueprintFieldType>::template push<25>)               \
    ON_OPCODE_IDENTIFIER(OP_PUSH26, operation<BlueprintFieldType>::template push<26>)               \
    ON_OPCODE_IDENTIFIER(OP_PUSH27, operation<BlueprintFieldType>::template push<27>)               \
    ON_OPCODE_IDENTIFIER(OP_PUSH28, operation<BlueprintFieldType>::template push<28>)               \
    ON_OPCODE_IDENTIFIER(OP_PUSH29, operation<BlueprintFieldType>::template push<29>)               \
    ON_OPCODE_IDENTIFIER(OP_PUSH30, operation<BlueprintFieldType>::template push<30>)               \
    ON_OPCODE_IDENTIFIER(OP_PUSH31, operation<BlueprintFieldType>::template push<31>)               \
    ON_OPCODE_IDENTIFIER(OP_PUSH32, operation<BlueprintFieldType>::template push<32>)               \
                                                            \
    ON_OPCODE_IDENTIFIER(OP_DUP1, operation<BlueprintFieldType>::template dup<1>)                   \
    ON_OPCODE_IDENTIFIER(OP_DUP2, operation<BlueprintFieldType>::template dup<2>)                   \
    ON_OPCODE_IDENTIFIER(OP_DUP3, operation<BlueprintFieldType>::template dup<3>)                   \
    ON_OPCODE_IDENTIFIER(OP_DUP4, operation<BlueprintFieldType>::template dup<4>)                   \
    ON_OPCODE_IDENTIFIER(OP_DUP5, operation<BlueprintFieldType>::template dup<5>)                   \
    ON_OPCODE_IDENTIFIER(OP_DUP6, operation<BlueprintFieldType>::template dup<6>)                   \
    ON_OPCODE_IDENTIFIER(OP_DUP7, operation<BlueprintFieldType>::template dup<7>)                   \
    ON_OPCODE_IDENTIFIER(OP_DUP8, operation<BlueprintFieldType>::template dup<8>)                   \
    ON_OPCODE_IDENTIFIER(OP_DUP9, operation<BlueprintFieldType>::template dup<9>)                   \
    ON_OPCODE_IDENTIFIER(OP_DUP10, operation<BlueprintFieldType>::template dup<10>)                 \
    ON_OPCODE_IDENTIFIER(OP_DUP11, operation<BlueprintFieldType>::template dup<11>)                 \
    ON_OPCODE_IDENTIFIER(OP_DUP12, operation<BlueprintFieldType>::template dup<12>)                 \
    ON_OPCODE_IDENTIFIER(OP_DUP13, operation<BlueprintFieldType>::template dup<13>)                 \
    ON_OPCODE_IDENTIFIER(OP_DUP14, operation<BlueprintFieldType>::template dup<14>)                 \
    ON_OPCODE_IDENTIFIER(OP_DUP15, operation<BlueprintFieldType>::template dup<15>)                 \
    ON_OPCODE_IDENTIFIER(OP_DUP16, operation<BlueprintFieldType>::template dup<16>)                 \
                                                            \
    ON_OPCODE_IDENTIFIER(OP_SWAP1, operation<BlueprintFieldType>::template swap<1>)                 \
    ON_OPCODE_IDENTIFIER(OP_SWAP2, operation<BlueprintFieldType>::template swap<2>)                 \
    ON_OPCODE_IDENTIFIER(OP_SWAP3, operation<BlueprintFieldType>::template swap<3>)                 \
    ON_OPCODE_IDENTIFIER(OP_SWAP4, operation<BlueprintFieldType>::template swap<4>)                 \
    ON_OPCODE_IDENTIFIER(OP_SWAP5, operation<BlueprintFieldType>::template swap<5>)                 \
    ON_OPCODE_IDENTIFIER(OP_SWAP6, operation<BlueprintFieldType>::template swap<6>)                 \
    ON_OPCODE_IDENTIFIER(OP_SWAP7, operation<BlueprintFieldType>::template swap<7>)                 \
    ON_OPCODE_IDENTIFIER(OP_SWAP8, operation<BlueprintFieldType>::template swap<8>)                 \
    ON_OPCODE_IDENTIFIER(OP_SWAP9, operation<BlueprintFieldType>::template swap<9>)                 \
    ON_OPCODE_IDENTIFIER(OP_SWAP10, operation<BlueprintFieldType>::template swap<10>)               \
    ON_OPCODE_IDENTIFIER(OP_SWAP11, operation<BlueprintFieldType>::template swap<11>)               \
    ON_OPCODE_IDENTIFIER(OP_SWAP12, operation<BlueprintFieldType>::template swap<12>)               \
    ON_OPCODE_IDENTIFIER(OP_SWAP13, operation<BlueprintFieldType>::template swap<13>)               \
    ON_OPCODE_IDENTIFIER(OP_SWAP14, operation<BlueprintFieldType>::template swap<14>)               \
    ON_OPCODE_IDENTIFIER(OP_SWAP15, operation<BlueprintFieldType>::template swap<15>)               \
    ON_OPCODE_IDENTIFIER(OP_SWAP16, operation<BlueprintFieldType>::template swap<16>)               \
                                                            \
    ON_OPCODE_IDENTIFIER(OP_SWAP, operation<BlueprintFieldType>::template swap<0>)                  \
    ON_OPCODE_IDENTIFIER(OP_DUP, operation<BlueprintFieldType>::template dup<0>)                    \
    ON_OPCODE_UNDEFINED(0xa8)                               \
    ON_OPCODE_UNDEFINED(0xa9)                               \
    ON_OPCODE_UNDEFINED(0xaa)                               \
    ON_OPCODE_UNDEFINED(0xab)                               \
    ON_OPCODE_UNDEFINED(0xac)                               \
    ON_OPCODE_UNDEFINED(0xad)                               \
    ON_OPCODE_UNDEFINED(0xae)                               \
    ON_OPCODE_UNDEFINED(0xaf)                               \
                                                            \
    ON_OPCODE_UNDEFINED(0xb3)                               \
    ON_OPCODE_UNDEFINED(0xb4)                               \
    ON_OPCODE_UNDEFINED(0xb5)                               \
    ON_OPCODE_UNDEFINED(0xb6)                               \
    ON_OPCODE_UNDEFINED(0xb7)                               \
    ON_OPCODE_UNDEFINED(0xb8)                               \
    ON_OPCODE_UNDEFINED(0xb9)                               \
    ON_OPCODE_UNDEFINED(0xba)                               \
    ON_OPCODE_UNDEFINED(0xbb)                               \
    ON_OPCODE_UNDEFINED(0xbc)                               \
    ON_OPCODE_UNDEFINED(0xbd)                               \
    ON_OPCODE_UNDEFINED(0xbe)                               \
    ON_OPCODE_UNDEFINED(0xbf)                               \
                                                            \
    ON_OPCODE_UNDEFINED(0xc4)                               \
    ON_OPCODE_UNDEFINED(0xc5)                               \
    ON_OPCODE_UNDEFINED(0xc6)                               \
    ON_OPCODE_UNDEFINED(0xc7)                               \
    ON_OPCODE_UNDEFINED(0xc8)                               \
    ON_OPCODE_UNDEFINED(0xc9)                               \
    ON_OPCODE_UNDEFINED(0xca)                               \
    ON_OPCODE_UNDEFINED(0xcb)                               \
    ON_OPCODE_UNDEFINED(0xcc)                               \
    ON_OPCODE_UNDEFINED(0xcd)                               \
    ON_OPCODE_UNDEFINED(0xce)                               \
    ON_OPCODE_UNDEFINED(0xcf)                               \
                                                            \
    ON_OPCODE_UNDEFINED(0xd0)                               \
    ON_OPCODE_UNDEFINED(0xd1)                               \
    ON_OPCODE_UNDEFINED(0xd2)                               \
    ON_OPCODE_UNDEFINED(0xd3)                               \
    ON_OPCODE_UNDEFINED(0xd4)                               \
    ON_OPCODE_UNDEFINED(0xd5)                               \
    ON_OPCODE_UNDEFINED(0xd6)                               \
    ON_OPCODE_UNDEFINED(0xd7)                               \
    ON_OPCODE_UNDEFINED(0xd8)                               \
    ON_OPCODE_UNDEFINED(0xd9)                               \
    ON_OPCODE_UNDEFINED(0xda)                               \
    ON_OPCODE_UNDEFINED(0xdb)                               \
    ON_OPCODE_UNDEFINED(0xdc)                               \
    ON_OPCODE_UNDEFINED(0xdd)                               \
    ON_OPCODE_UNDEFINED(0xde)                               \
    ON_OPCODE_UNDEFINED(0xdf)                               \
                                                            \
    ON_OPCODE_IDENTIFIER(OP_RJUMP, operation<BlueprintFieldType>::rjump)                   \
    ON_OPCODE_IDENTIFIER(OP_RJUMPI, operation<BlueprintFieldType>::rjumpi)                 \
    ON_OPCODE_IDENTIFIER(OP_RJUMPV, operation<BlueprintFieldType>::rjumpv)                 \
    ON_OPCODE_IDENTIFIER(OP_CALLF, operation<BlueprintFieldType>::callf)                   \
    ON_OPCODE_IDENTIFIER(OP_RETF, operation<BlueprintFieldType>::retf)                     \
    ON_OPCODE_IDENTIFIER(OP_JUMPF, operation<BlueprintFieldType>::jumpf)                   \
    ON_OPCODE_IDENTIFIER(OP_DUPN, operation<BlueprintFieldType>::dupn)                     \
    ON_OPCODE_IDENTIFIER(OP_SWAPN, operation<BlueprintFieldType>::swapn)                   \
    ON_OPCODE_IDENTIFIER(OP_DATALOAD, operation<BlueprintFieldType>::dataload)             \
    ON_OPCODE_IDENTIFIER(OP_DATALOADN, operation<BlueprintFieldType>::dataloadn)           \
    ON_OPCODE_IDENTIFIER(OP_DATASIZE, operation<BlueprintFieldType>::datasize)             \
    ON_OPCODE_IDENTIFIER(OP_DATACOPY, operation<BlueprintFieldType>::datacopy)             \
    ON_OPCODE_UNDEFINED(0xec)                               \
    ON_OPCODE_UNDEFINED(0xed)                               \
    ON_OPCODE_UNDEFINED(0xee)                               \
    ON_OPCODE_UNDEFINED(0xef)                               \
                                                            \
    ON_OPCODE_IDENTIFIER(OP_CREATE, operation<BlueprintFieldType>::create)                 \
    ON_OPCODE_IDENTIFIER(OP_CALL, operation<BlueprintFieldType>::call)                     \
    ON_OPCODE_IDENTIFIER(OP_CALLCODE, operation<BlueprintFieldType>::callcode)             \
    ON_OPCODE_IDENTIFIER(OP_RETURN, operation<BlueprintFieldType>::return_)                \
    ON_OPCODE_IDENTIFIER(OP_DELEGATECALL, operation<BlueprintFieldType>::delegatecall)     \
    ON_OPCODE_IDENTIFIER(OP_CREATE2, operation<BlueprintFieldType>::create2)               \
    ON_OPCODE_UNDEFINED(0xf6)                               \
    ON_OPCODE_UNDEFINED(0xf7)                               \
    ON_OPCODE_UNDEFINED(0xf8)                               \
    ON_OPCODE_UNDEFINED(0xf9)                               \
    ON_OPCODE_IDENTIFIER(OP_STATICCALL, operation<BlueprintFieldType>::staticcall)         \
    ON_OPCODE_UNDEFINED(0xfb)                               \
    ON_OPCODE_UNDEFINED(0xfc)                               \
    ON_OPCODE_IDENTIFIER(OP_REVERT, operation<BlueprintFieldType>::revert)                 \
    ON_OPCODE_IDENTIFIER(OP_INVALID, operation<BlueprintFieldType>::invalid)               \
    ON_OPCODE_IDENTIFIER(OP_SELFDESTRUCT, operation<BlueprintFieldType>::selfdestruct)
