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

namespace nil {
    namespace blueprint {

        template<typename BlueprintFieldType>
        struct assigner {

            using ArithmetizationType = crypto3::zk::snark::plonk_constraint_system<BlueprintFieldType>;

            assigner(std::vector<assignment<ArithmetizationType>> &assignments):  m_assignments(assignments) {}

            std::vector<assignment<ArithmetizationType>> &m_assignments;
        };
    }     // namespace blueprint
}    // namespace nil

#endif    // EVM1_ASSIGNER_INCLUDE_NIL_BLUEPRINT_ASSIGNER_HPP_
