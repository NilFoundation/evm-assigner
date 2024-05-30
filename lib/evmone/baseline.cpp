// evmone: Fast Ethereum Virtual Machine implementation
// Copyright 2020 The evmone Authors.
// SPDX-License-Identifier: Apache-2.0

#include "baseline.hpp"
#include "baseline_instruction_table.hpp"
#include "eof.hpp"
#include "execution_state.hpp"
#include "instructions.hpp"
#include "vm.hpp"
#include <memory>

#include <nil/crypto3/algebra/curves/pallas.hpp>
#include <nil/blueprint/blueprint/plonk/assignment.hpp>
#include <nil/blueprint/blueprint/plonk/circuit.hpp>
#include <nil/blueprint/component.hpp>
#include <nil/blueprint/handler.hpp>
//#include <nil/blueprint/components/zkevm/circuits/bytecode.hpp>

#include <nil/crypto3/hash/algorithm/hash.hpp>
#include <nil/crypto3/hash/keccak.hpp>
#include <nil/crypto3/random/algebraic_engine.hpp>

#ifdef NDEBUG
#define release_inline gnu::always_inline, msvc::forceinline
#else
#define release_inline
#endif

#if defined(__GNUC__)
#define ASM_COMMENT(COMMENT) asm("# " #COMMENT)  // NOLINT(hicpp-no-assembler)
#else
#define ASM_COMMENT(COMMENT)
#endif

namespace evmone::baseline
{
namespace
{
CodeAnalysis::JumpdestMap analyze_jumpdests(bytes_view code)
{
    // To find if op is any PUSH opcode (OP_PUSH1 <= op <= OP_PUSH32)
    // it can be noticed that OP_PUSH32 is INT8_MAX (0x7f) therefore
    // static_cast<int8_t>(op) <= OP_PUSH32 is always true and can be skipped.
    static_assert(OP_PUSH32 == std::numeric_limits<int8_t>::max());

    CodeAnalysis::JumpdestMap map(code.size());  // Allocate and init bitmap with zeros.
    for (size_t i = 0; i < code.size(); ++i)
    {
        const auto op = code[i];
        if (static_cast<int8_t>(op) >= OP_PUSH1)  // If any PUSH opcode (see explanation above).
            i += op - size_t{OP_PUSH1 - 1};       // Skip PUSH data.
        else if (INTX_UNLIKELY(op == OP_JUMPDEST))
            map[i] = true;
    }

    return map;
}

std::unique_ptr<uint8_t[]> pad_code(bytes_view code)
{
    // We need at most 33 bytes of code padding: 32 for possible missing all data bytes of PUSH32
    // at the very end of the code; and one more byte for STOP to guarantee there is a terminating
    // instruction at the code end.
    constexpr auto padding = 32 + 1;

    // Using "raw" new operator instead of std::make_unique() to get uninitialized array.
    std::unique_ptr<uint8_t[]> padded_code{new uint8_t[code.size() + padding]};
    std::copy(std::begin(code), std::end(code), padded_code.get());
    std::fill_n(&padded_code[code.size()], padding, uint8_t{OP_STOP});
    return padded_code;
}


CodeAnalysis analyze_legacy(bytes_view code)
{
    // TODO: The padded code buffer and jumpdest bitmap can be created with single allocation.
    return {pad_code(code), code.size(), analyze_jumpdests(code)};
}

CodeAnalysis analyze_eof1(bytes_view container)
{
    auto header = read_valid_eof1_header(container);

    // Extract all code sections as single buffer reference.
    // TODO: It would be much easier if header had code_sections_offset and data_section_offset
    //       with code_offsets[] being relative to code_sections_offset.
    const auto code_sections_offset = header.code_offsets[0];
    const auto code_sections_end = size_t{header.code_offsets.back()} + header.code_sizes.back();
    const auto executable_code =
        container.substr(code_sections_offset, code_sections_end - code_sections_offset);

    return CodeAnalysis{executable_code, std::move(header)};
}
}  // namespace

CodeAnalysis analyze(evmc_revision rev, bytes_view code)
{
    if (rev < EVMC_PRAGUE || !is_eof_container(code))
        return analyze_legacy(code);
    return analyze_eof1(code);
}

namespace
{
/// Checks instruction requirements before execution.
///
/// This checks:
/// - if the instruction is defined
/// - if stack height requirements are fulfilled (stack overflow, stack underflow)
/// - charges the instruction base gas cost and checks is there is any gas left.
///
/// @tparam         Op            Instruction opcode.
/// @param          cost_table    Table of base gas costs.
/// @param [in,out] gas_left      Gas left.
/// @param          stack_top     Pointer to the stack top item.
/// @param          stack_bottom  Pointer to the stack bottom.
///                               The stack height is stack_top - stack_bottom.
/// @return  Status code with information which check has failed
///          or EVMC_SUCCESS if everything is fine.
template <Opcode Op>
inline evmc_status_code check_requirements(const CostTable& cost_table, int64_t& gas_left,
    const uint256* stack_top, const uint256* stack_bottom) noexcept
{
    static_assert(
        !instr::has_const_gas_cost(Op) || instr::gas_costs[EVMC_FRONTIER][Op] != instr::undefined,
        "undefined instructions must not be handled by check_requirements()");

    auto gas_cost = instr::gas_costs[EVMC_FRONTIER][Op];  // Init assuming const cost.
    if constexpr (!instr::has_const_gas_cost(Op))
    {
        gas_cost = cost_table[Op];  // If not, load the cost from the table.

        // Negative cost marks an undefined instruction.
        // This check must be first to produce correct error code.
        if (INTX_UNLIKELY(gas_cost < 0))
            return EVMC_UNDEFINED_INSTRUCTION;
    }

    // Check stack requirements first. This is order is not required,
    // but it is nicer because complete gas check may need to inspect operands.
    if constexpr (instr::traits[Op].stack_height_change > 0)
    {
        static_assert(instr::traits[Op].stack_height_change == 1,
            "unexpected instruction with multiple results");
        if (INTX_UNLIKELY(stack_top == stack_bottom + StackSpace::limit))
            return EVMC_STACK_OVERFLOW;
    }
    if constexpr (instr::traits[Op].stack_height_required > 0)
    {
        // Check stack underflow using pointer comparison <= (better optimization).
        static constexpr auto min_offset = instr::traits[Op].stack_height_required - 1;
        if (INTX_UNLIKELY(stack_top <= stack_bottom + min_offset))
            return EVMC_STACK_UNDERFLOW;
    }

    if (INTX_UNLIKELY((gas_left -= gas_cost) < 0))
        return EVMC_OUT_OF_GAS;

    return EVMC_SUCCESS;
}


/// The execution position.
struct Position
{
    code_iterator code_it;  ///< The position in the code.
    uint256* stack_top;     ///< The pointer to the stack top.
};

/// Helpers for invoking instruction implementations of different signatures.
/// @{
[[release_inline]] inline code_iterator invoke(void (*instr_fn)(StackTop) noexcept, Position pos,
    int64_t& /*gas*/, ExecutionState& /*state*/) noexcept
{
    instr_fn(pos.stack_top);
    return pos.code_it + 1;
}

[[release_inline]] inline code_iterator invoke(
    Result (*instr_fn)(StackTop, int64_t, ExecutionState&) noexcept, Position pos, int64_t& gas,
    ExecutionState& state) noexcept
{
    const auto o = instr_fn(pos.stack_top, gas, state);
    gas = o.gas_left;
    if (o.status != EVMC_SUCCESS)
    {
        state.status = o.status;
        return nullptr;
    }
    return pos.code_it + 1;
}

[[release_inline]] inline code_iterator invoke(void (*instr_fn)(StackTop, ExecutionState&) noexcept,
    Position pos, int64_t& /*gas*/, ExecutionState& state) noexcept
{
    instr_fn(pos.stack_top, state);
    return pos.code_it + 1;
}

[[release_inline]] inline code_iterator invoke(
    code_iterator (*instr_fn)(StackTop, ExecutionState&, code_iterator) noexcept, Position pos,
    int64_t& /*gas*/, ExecutionState& state) noexcept
{
    return instr_fn(pos.stack_top, state, pos.code_it);
}

[[release_inline]] inline code_iterator invoke(
    TermResult (*instr_fn)(StackTop, int64_t, ExecutionState&) noexcept, Position pos, int64_t& gas,
    ExecutionState& state) noexcept
{
    const auto result = instr_fn(pos.stack_top, gas, state);
    gas = result.gas_left;
    state.status = result.status;
    return nullptr;
}
/// @}

/// A helper to invoke the instruction implementation of the given opcode Op.
template <Opcode Op>
[[release_inline]] inline Position invoke(const CostTable& cost_table, const uint256* stack_bottom,
    Position pos, int64_t& gas, ExecutionState& state) noexcept
{
    if (const auto status = check_requirements<Op>(cost_table, gas, pos.stack_top, stack_bottom);
        status != EVMC_SUCCESS)
    {
        state.status = status;
        return {nullptr, pos.stack_top};
    }
    const auto new_pos = invoke(instr::core::impl<Op>, pos, gas, state);
    const auto new_stack_top = pos.stack_top + instr::traits[Op].stack_height_change;
    return {new_pos, new_stack_top};
}

/// PRINTF opcode consumes variable number of operands, determined dynamically by the format string.
/// This `invoke` specialization determines the stack shift according to the result of the Opcode
/// handler.
template <>
[[release_inline]] inline Position invoke<Opcode::OP_PRINTF>(const CostTable& cost_table, const uint256* stack_bottom,
    Position pos, int64_t& gas, ExecutionState& state) noexcept
{
    static constexpr Opcode Op = Opcode::OP_PRINTF;

    if (const auto status = check_requirements<Op>(cost_table, gas, pos.stack_top, stack_bottom);
        status != EVMC_SUCCESS)
    {
        state.status = status;
        return {nullptr, pos.stack_top};
    }
    auto res = instr::core::impl<Op>(pos.stack_top, gas, state);
    gas = res.gas_left;
    code_iterator code_it;
    if (res.status != EVMC_SUCCESS)
    {
        state.status = res.status;
        code_it = nullptr;
    } else
    {
        code_it = pos.code_it + 1;
    }

    const auto new_stack_top = pos.stack_top - res.stack_pop;
    return {code_it, new_stack_top};
}


template <bool TracingEnabled>
int64_t dispatch(const CostTable& cost_table, ExecutionState& state, int64_t gas,
    const uint8_t* code, Tracer* tracer = nullptr) noexcept
{
    const auto stack_bottom = state.stack_space.bottom();

    // Code iterator and stack top pointer for interpreter loop.
    Position position{code, stack_bottom};

    //BYTECODE CODE
    
    using value_type = typename nil::crypto3::algebra::curves::pallas::base_field_type::value_type;
    using hash_type = nil::crypto3::hashes::keccak_1600<256>;
    //using component_type = blueprint::components::zkevm_bytecode<ArithmetizationType, BlueprintFieldType>;
    std::cout << "MYINFO: <Enter dispatch function>" << std::endl;
    std::cout << "MYINFO: bytecode size: " << state.original_code.size() << std::endl;
    std::cout << "MYINFO: bytecode: " << std::endl;
    std::vector<std::uint8_t> bytecode;
    for (int i = 0; i < state.original_code.size(); i++) {
        const auto op = code[i];
        bytecode.push_back(op);
        std::cout << (uint)op << " ";
    }
    std::cout << std::endl;

    std::cout << "MYINFO: <get hashcode>:" << std::endl;
    std::string hash = nil::crypto3::hash<nil::crypto3::hashes::keccak_1600<256>>(bytecode.begin(), bytecode.end());
    std::string str_hi = hash.substr(0, hash.size()-32);
    std::string str_lo = hash.substr(hash.size()-32, 32);
    value_type hash_hi;
    value_type hash_lo;
    for( std::size_t j = 0; j < str_hi.size(); j++ ){hash_hi *=16; hash_hi += str_hi[j] >= '0' && str_hi[j] <= '9'? str_hi[j] - '0' : str_hi[j] - 'a' + 10;}
    for( std::size_t j = 0; j < str_lo.size(); j++ ){hash_lo *=16; hash_lo += str_lo[j] >= '0' && str_lo[j] <= '9'? str_lo[j] - '0' : str_lo[j] - 'a' + 10;}
    std::cout << std::hex <<  "Contract hash = " << hash << " h:" << hash_hi << " l:" << hash_lo << std::dec << std::endl;
    //public_input.push_back(hash_hi);
    //public_input.push_back(hash_lo);
    static constexpr std::size_t TAG = 0;
    static constexpr std::size_t INDEX = 1;
    static constexpr std::size_t VALUE = 2;
    static constexpr std::size_t IS_OPCODE = 3;
    static constexpr std::size_t PUSH_SIZE = 4;
    static constexpr std::size_t LENGTH_LEFT = 5;
    static constexpr std::size_t HASH_HI = 6;
    static constexpr std::size_t HASH_LO = 7;
    static constexpr std::size_t VALUE_RLC = 8;
    static constexpr std::size_t RLC_CHALLENGE = 9;

    value_type rlc_challenge = 15;
    std::size_t cur = 0;
    std::size_t start_row_index = 0;
    std::size_t prev_length = 0;
    value_type prev_vrlc = 0;
    value_type push_size = 0;
    for(std::size_t j = 0; j < state.original_code.size(); j++, cur++){
        std::uint8_t byte = bytecode[j];
        state.m_handler->set_witness(0, VALUE, start_row_index + cur,  (uint256)bytecode[j]);
        state.m_handler->set_witness(0, HASH_HI, start_row_index + cur, nil::blueprint::handler<
                typename nil::crypto3::algebra::curves::pallas::base_field_type>::to_uint256(hash_hi));
        state.m_handler->set_witness(0, HASH_LO, start_row_index + cur, nil::blueprint::handler<
                typename nil::crypto3::algebra::curves::pallas::base_field_type>::to_uint256(hash_lo));
        state.m_handler->set_witness(0, RLC_CHALLENGE, start_row_index + cur, nil::blueprint::handler<
                typename nil::crypto3::algebra::curves::pallas::base_field_type>::to_uint256(rlc_challenge));
        if( j == 0) {
            // HEADER
            state.m_handler->set_witness(0, TAG, start_row_index + cur, 0);
            state.m_handler->set_witness(0, INDEX, start_row_index + cur, 0);
            state.m_handler->set_witness(0, IS_OPCODE, start_row_index + cur, 0);
            state.m_handler->set_witness(0, PUSH_SIZE, start_row_index + cur, 0);
            prev_length = bytecode[j];
            state.m_handler->set_witness(0, LENGTH_LEFT, start_row_index + cur, (uint256)bytecode[j]);
            prev_vrlc = 0;
            state.m_handler->set_witness(0, VALUE_RLC, start_row_index + cur, 0);
            push_size = 0;
        } else {
            // BYTE
            state.m_handler->set_witness(0, TAG, start_row_index + cur, 1);
            state.m_handler->set_witness(0, INDEX, start_row_index + cur, j-1);
            state.m_handler->set_witness(0, LENGTH_LEFT, start_row_index + cur, prev_length - 1);
            prev_length = prev_length - 1;
            if (push_size == 0) {
                state.m_handler->set_witness(0, IS_OPCODE, start_row_index + cur, 1);
                if(byte > 0x5f && byte < 0x80) push_size = byte - 0x5f;
            } else {
                state.m_handler->set_witness(0, IS_OPCODE, start_row_index + cur, 0);
                push_size--;
            }
            state.m_handler->set_witness(0, PUSH_SIZE, start_row_index + cur, nil::blueprint::handler<
                typename nil::crypto3::algebra::curves::pallas::base_field_type>::to_uint256(push_size));
            state.m_handler->set_witness(0, VALUE_RLC, start_row_index + cur, nil::blueprint::handler<
                typename nil::crypto3::algebra::curves::pallas::base_field_type>::to_uint256(prev_vrlc * rlc_challenge + byte));
            prev_vrlc = prev_vrlc * rlc_challenge + byte;
        }
    }

    while (true)  // Guaranteed to terminate because padded code ends with STOP.
    {
        if constexpr (TracingEnabled)
        {
            const auto offset = static_cast<uint32_t>(position.code_it - code);
            const auto stack_height = static_cast<int>(position.stack_top - stack_bottom);
            if (offset < state.original_code.size())  // Skip STOP from code padding.
            {
                tracer->notify_instruction_start(
                    offset, position.stack_top, stack_height, gas, state);
            }
        }

        const auto op = *position.code_it;
        switch (op)
        {
#define ON_OPCODE(OPCODE)                                                                     \
    case OPCODE:                                                                              \
        ASM_COMMENT(OPCODE);                                                                  \
        if (const auto next = invoke<OPCODE>(cost_table, stack_bottom, position, gas, state); \
            next.code_it == nullptr)                                                          \
        {                                                                                     \
            return gas;                                                                       \
        }                                                                                     \
        else                                                                                  \
        {                                                                                     \
            /* Update current position only when no error,                                    \
               this improves compiler optimization. */                                        \
            position = next;                                                                  \
        }                                                                                     \
        break;

            MAP_OPCODES
#undef ON_OPCODE

        default:
            state.status = EVMC_UNDEFINED_INSTRUCTION;
            return gas;
        }
    }
    intx::unreachable();
}

}  // namespace

evmc_result execute(
    const VM& vm, int64_t gas, ExecutionState& state, const CodeAnalysis& analysis) noexcept
{
    state.analysis.baseline = &analysis;  // Assign code analysis for instruction implementations.

    const auto code = analysis.executable_code;

    const auto& cost_table = get_baseline_cost_table(state.rev, analysis.eof_header.version);

    auto* tracer = vm.get_tracer();
    if (INTX_UNLIKELY(tracer != nullptr))
    {
        tracer->notify_execution_start(state.rev, *state.msg, analysis.executable_code);
        gas = dispatch<true>(cost_table, state, gas, code.data(), tracer);
    }
    else
    {
        gas = dispatch<false>(cost_table, state, gas, code.data());
    }

    const auto gas_left = (state.status == EVMC_SUCCESS || state.status == EVMC_REVERT) ? gas : 0;
    const auto gas_refund = (state.status == EVMC_SUCCESS) ? state.gas_refund : 0;

    assert(state.output_size != 0 || state.output_offset == 0);
    const auto result = evmc::make_result(state.status, gas_left, gas_refund,
        state.output_size != 0 ? &state.memory[state.output_offset] : nullptr, state.output_size);

    if (INTX_UNLIKELY(tracer != nullptr))
        tracer->notify_execution_end(result);

    return result;
}

evmc_result execute(evmc_vm* c_vm, const evmc_host_interface* host, evmc_host_context* ctx,
    evmc_revision rev, const evmc_message* msg, const uint8_t* code, size_t code_size) noexcept
{
    auto vm = static_cast<VM*>(c_vm);
    const bytes_view container{code, code_size};
    const auto code_analysis = analyze(rev, container);
    const auto data = code_analysis.eof_header.get_data(container);
    auto state = std::make_unique<ExecutionState>(*msg, rev, *host, ctx, container, data);
    return execute(*vm, msg->gas, *state, code_analysis);
}
}  // namespace evmone::baseline
