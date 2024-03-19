//---------------------------------------------------------------------------//
// Copyright (c) Nil Foundation and its affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//---------------------------------------------------------------------------//

#ifndef EVM1_ASSIGNER_INCLUDE_NIL_BLUEPRINT_HANDLER_HPP_
#define EVM1_ASSIGNER_INCLUDE_NIL_BLUEPRINT_HANDLER_HPP_

#include <nil/blueprint/handler_base.hpp>
#include <nil/blueprint/blueprint/plonk/assignment.hpp>

namespace nil {
    namespace blueprint {

        template<typename BlueprintFieldType>
        class handler : public handler_base {

        public:

            using ArithmetizationType = crypto3::zk::snark::plonk_constraint_system<BlueprintFieldType>;
            using value_type = typename BlueprintFieldType::value_type;

            handler(crypto3::zk::snark::plonk_table_description<BlueprintFieldType> desc,
                    std::vector<assignment<ArithmetizationType>> &assignments):
                    m_assignments(assignments),
                    m_desc(desc) {

            }

            void set_witness(std::size_t table_idx, std::size_t column_idx, std::size_t row_idx, const uint256& v) override {
                if (table_idx >= m_assignments.size()) {
                    m_assignments.push_back(m_desc);
                }
                m_assignments[table_idx].witness(column_idx, row_idx) = to_field(v);
            }

            void set_constant(std::size_t table_idx, std::size_t column_idx, std::size_t row_idx, const uint256& v) override {
                if (table_idx >= m_assignments.size()) {
                    m_assignments.push_back(m_desc);
                }
                m_assignments[table_idx].constant(column_idx, row_idx) = to_field(v);
            }

            void set_public_input(std::size_t table_idx, std::size_t column_idx, std::size_t row_idx, const uint256& v) override {
                if (table_idx >= m_assignments.size()) {
                    m_assignments.push_back(m_desc);
                }
                m_assignments[table_idx].public_input(column_idx, row_idx) = to_field(v);
            }

            void set_selector(std::size_t table_idx, std::size_t column_idx, std::size_t row_idx, const uint256& v) override {
                if (table_idx >= m_assignments.size()) {
                    m_assignments.push_back(m_desc);
                }
                m_assignments[table_idx].selector(column_idx, row_idx) = to_field(v);
            }

            uint256 witness(std::size_t table_idx, std::size_t column_idx, std::size_t row_idx) override {
                return to_uint256(m_assignments[table_idx].witness(column_idx, row_idx));
            }

            uint256 constant(std::size_t table_idx, std::size_t column_idx, std::size_t row_idx) override {
                return to_uint256(m_assignments[table_idx].constant(column_idx, row_idx));
            }

            uint256 public_input(std::size_t table_idx, std::size_t column_idx, std::size_t row_idx) override {
                return to_uint256(m_assignments[table_idx].public_input(column_idx, row_idx));
            }

            uint256 selector(std::size_t table_idx, std::size_t column_idx, std::size_t row_idx) override {
                return to_uint256(m_assignments[table_idx].selector(column_idx, row_idx));
            }

        private:

            value_type to_field(const uint256 v) {
                //TODO: implement conversion: https://github.com/chfast/intx/blob/e0732242e36b3bb08cc8cc23445b2bee7c28b9d0/include/intx/intx.hpp#L967
                value_type field_val;
                return field_val;
            }

            uint256 to_uint256(const value_type v) {
                //TODO: implement conversion: https://github.com/chfast/intx/blob/e0732242e36b3bb08cc8cc23445b2bee7c28b9d0/include/intx/intx.hpp#L967
                uint256 val;
                return val;
            }

            std::vector<assignment<ArithmetizationType>> &m_assignments;
            crypto3::zk::snark::plonk_table_description<BlueprintFieldType> m_desc;
        };

    }     // namespace blueprint
}    // namespace nil

#endif    // EVM1_ASSIGNER_INCLUDE_NIL_BLUEPRINT_HANDLER_BASE_HPP_
