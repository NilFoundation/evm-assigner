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

namespace nil {
    namespace blueprint {

        template<typename BlueprintFieldType>
        struct assigner {

            using ArithmetizationType = crypto3::zk::snark::plonk_constraint_system<BlueprintFieldType>;

            assigner(std::vector<assignment<ArithmetizationType>> &assignments) {
                handler_ptr = std::make_shared<handler<BlueprintFieldType>>(assignments);
            }

            evmc::Result evaluate(evmc_vm* c_vm, const evmc_host_interface* host, evmc_host_context* ctx,
                          evmc_revision rev, const evmc_message* msg, const uint8_t* code, size_t code_size) {
                return nil::blueprint::evaluate(handler_ptr, c_vm, host, ctx, rev, msg, code, code_size);
            }

            std::shared_ptr<handler_base> get_handler() {
                return handler_ptr;
            }

        private:

            std::shared_ptr<handler_base> handler_ptr;
        };

    }     // namespace blueprint
}    // namespace nil

#endif    // EVM1_ASSIGNER_INCLUDE_NIL_BLUEPRINT_ASSERTS_HPP_
