//---------------------------------------------------------------------------//
// Copyright (c) Nil Foundation and its affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//---------------------------------------------------------------------------//

#ifndef EVM1_ASSIGNER_INCLUDE_NIL_BLUEPRINT_ASSIGNER_HPP_
#define EVM1_ASSIGNER_INCLUDE_NIL_BLUEPRINT_ASSIGNER_HPP_

#include <evmc/evmc.hpp>

#include <nil/crypto3/algebra/curves/pallas.hpp>
#include <nil/blueprint/blueprint/plonk/assignment.hpp>

#include <bytecode.hpp>
#include <baseline.hpp>
#include <execution_state.hpp>

namespace nil {
    namespace blueprint {

        template<typename BlueprintFieldType>
        struct assigner {

            using ArithmetizationType = crypto3::zk::snark::plonk_constraint_system<BlueprintFieldType>;

            assigner(std::vector<assignment<ArithmetizationType>> &assignments):  m_assignments(assignments) {}

            // TODO error handling
            void handle_bytcode(size_t original_code_size, const uint8_t* code) {
                return process_bytecode_input<BlueprintFieldType>(original_code_size, code, m_assignments);
            }

            // TODO error handling
            void handle_rw(std::vector<rw_operation<BlueprintFieldType>>& rw_trace) {
                return process_rw_operations<BlueprintFieldType>(rw_trace, m_assignments);
            }

            std::vector<assignment<ArithmetizationType>> &m_assignments;
        };

        template<typename BlueprintFieldType>
        static evmc::Result evaluate(const evmc_host_interface* host, evmc_host_context* ctx,
                                evmc_revision rev, const evmc_message* msg, const uint8_t* code_ptr, size_t code_size,
                                std::shared_ptr<nil::blueprint::assigner<BlueprintFieldType>> assigner) {
            const evmone::bytes_view container{code_ptr, code_size};
            const auto code_analysis = evmone::baseline::analyze(rev, container);
            const auto data = code_analysis.eof_header.get_data(container);
            evmone::ExecutionState<BlueprintFieldType> state(*msg, rev, *host, ctx, container, data, 0, assigner);

            state.analysis.baseline = &code_analysis;  // Assign code analysis for instruction implementations.
            const auto code = code_analysis.executable_code;

            // fill assignments for bytecode circuit
            assigner->handle_bytcode(state.original_code.size(), code.data());

            int64_t gas = msg->gas;

            const auto& cost_table = evmone::baseline::get_baseline_cost_table(state.rev, code_analysis.eof_header.version);

            gas = evmone::baseline::dispatch<false>(cost_table, state, msg->gas, code.data());

            // fill assignments for read/write circuit
            assigner->handle_rw(state.rw_trace);

            const auto gas_left = (state.status == EVMC_SUCCESS || state.status == EVMC_REVERT) ? gas : 0;
            const auto gas_refund = (state.status == EVMC_SUCCESS) ? state.gas_refund : 0;

            assert(state.output_size != 0 || state.output_offset == 0);
            const auto evmc_result = evmc::make_result(state.status, gas_left, gas_refund,
                state.output_size != 0 ? &state.memory[state.output_offset] : nullptr, state.output_size);
            return evmc::Result{evmc_result};
        }

    }     // namespace blueprint
}    // namespace nil

#endif    // EVM1_ASSIGNER_INCLUDE_NIL_BLUEPRINT_ASSIGNER_HPP_
