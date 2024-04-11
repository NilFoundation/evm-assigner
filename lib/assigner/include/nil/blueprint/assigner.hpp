//---------------------------------------------------------------------------//
// Copyright (c) Nil Foundation and its affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//---------------------------------------------------------------------------//

#ifndef EVM1_ASSIGNER_INCLUDE_NIL_BLUEPRINT_ASSERTS_HPP_
#define EVM1_ASSIGNER_INCLUDE_NIL_BLUEPRINT_ASSERTS_HPP_

#include <nil/blueprint/handler.hpp>
#include <nil/crypto3/algebra/curves/pallas.hpp>

#include <evmone/evmone.h>
#include <vm.hpp>
#include <baseline.hpp>
#include <execution_state.hpp>

namespace nil {
    namespace blueprint {

        template<typename BlueprintFieldType>
        struct assigner {

            using ArithmetizationType = crypto3::zk::snark::plonk_constraint_system<BlueprintFieldType>;

            assigner(crypto3::zk::snark::plonk_table_description<BlueprintFieldType> desc,
                     std::vector<assignment<ArithmetizationType>> &assignments) {
                handler_ptr = std::make_shared<handler<BlueprintFieldType>>(desc, assignments);
            }

            void evaluate(evmc_vm* c_vm, const evmc_host_interface* host, evmc_host_context* ctx,
                          evmc_revision rev, const evmc_message* msg, const uint8_t* code, size_t code_size) {

                auto vm = static_cast<evmone::VM*>(c_vm);
                const evmone::bytes_view container{code, code_size};
                const auto code_analysis = evmone::baseline::analyze(rev, container);
                const auto data = code_analysis.eof_header.get_data(container);
                auto state = std::make_unique<evmone::ExecutionState>(*msg, rev, *host, ctx, container, data);
                state->set_handler(handler_ptr);

                auto res = execute(*vm, msg->gas, *state, code_analysis);
                // TODO check res
            }

        private:

            std::shared_ptr<handler_base> handler_ptr;
        };

    }     // namespace blueprint
}    // namespace nil

#endif    // EVM1_ASSIGNER_INCLUDE_NIL_BLUEPRINT_ASSERTS_HPP_