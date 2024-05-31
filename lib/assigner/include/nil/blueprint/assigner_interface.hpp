//---------------------------------------------------------------------------//
// Copyright (c) Nil Foundation and its affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//---------------------------------------------------------------------------//

#ifndef EVM1_ASSIGNER_INCLUDE_NIL_BLUEPRINT_ASSIGNER_INTERACE_HPP_
#define EVM1_ASSIGNER_INCLUDE_NIL_BLUEPRINT_ASSIGNER_INTERACE_HPP_

#include <nil/blueprint/assigner.hpp>
#include <vm.hpp>
#include <baseline.hpp>
#include <execution_state.hpp>

namespace nil {
    namespace blueprint {

        template<typename BlueprintFieldType>
        static evmc::Result evaluate(evmc_vm* c_vm, const evmc_host_interface* host, evmc_host_context* ctx,
                                     evmc_revision rev, const evmc_message* msg, const uint8_t* code, size_t code_size,
                                     std::shared_ptr<nil::blueprint::assigner<BlueprintFieldType>> assigner) {
            auto vm = static_cast<evmone::VM*>(c_vm);
            const evmone::bytes_view container{code, code_size};
            const auto code_analysis = evmone::baseline::analyze(rev, container);
            const auto data = code_analysis.eof_header.get_data(container);
            auto state = std::make_unique<evmone::ExecutionState<BlueprintFieldType>>(*msg, rev, *host, ctx, container, data, assigner);

            auto res = execute(*vm, msg->gas, *state, code_analysis);
            return evmc::Result{res};
        }
    }     // namespace blueprint
}    // namespace nil

#endif    // EVM1_ASSIGNER_INCLUDE_NIL_BLUEPRINT_ASSIGNER_INTERACE_HPP_
