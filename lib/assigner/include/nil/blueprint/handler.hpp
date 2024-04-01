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


        public:
            static value_type to_field(const uint256 intx_number) {
                assert(intx_number < modulus);  // TODO: replace with crypto3 assert
                typename BlueprintFieldType::value_type field_value;
                for (unsigned i = 0; i < 4; ++i)
                {
                    typename BlueprintFieldType::integral_type word = intx_number[i];
                    field_value += word << (64 * i);
                }
                return field_value;
            }

            static uint256 to_uint256(const value_type field_value) {
                return integral_to_uint256(
                    typename BlueprintFieldType::integral_type(field_value.data));
            }

        private:
            static constexpr uint256 integral_to_uint256(
                typename BlueprintFieldType::integral_type integral_value)
            {
                intx::uint256 res;
                typename BlueprintFieldType::integral_type word_mask = 1;
                word_mask = word_mask << 64U;
                word_mask -= 1; // Got mask for lowest 64 bits
                auto num = integral_value;
                for (unsigned i = 0; i < 4; ++i)
                {
                    res[i] = static_cast<uint64_t>(num & word_mask);
                    num = num >> 64;
                }
                return res;
            }

            static constexpr uint256 modulus = integral_to_uint256(BlueprintFieldType::modulus);

            std::vector<assignment<ArithmetizationType>> &m_assignments;
            crypto3::zk::snark::plonk_table_description<BlueprintFieldType> m_desc;
        };

    }     // namespace blueprint
}    // namespace nil

#endif    // EVM1_ASSIGNER_INCLUDE_NIL_BLUEPRINT_HANDLER_BASE_HPP_
