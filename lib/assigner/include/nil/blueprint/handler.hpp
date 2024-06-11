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

#include <boost/test/unit_test.hpp>
// #include <boost/test/included/unit_test.hpp>
#include "boost/random/random_device.hpp"
#include <nil/crypto3/hash/algorithm/hash.hpp>
#include <nil/crypto3/hash/keccak.hpp>
#include <nil/crypto3/random/algebraic_engine.hpp>

#include <nil/blueprint/blueprint/zkevm/bytecode.hpp>
// #include "bytecode.hpp"
#include "test_plonk_component.hpp"

std::vector<std::uint8_t> hex_string_to_bytes(std::string const &hex_string) {
    std::vector<std::uint8_t> bytes;
    for (std::size_t i = 2; i < hex_string.size(); i += 2) {
        std::string byte_string = hex_string.substr(i, 2);
        bytes.push_back(std::stoi(byte_string, nullptr, 16));
    }
    return bytes;
}


namespace nil {
    namespace blueprint {

        template<typename BlueprintFieldType>
        class handler : public handler_base {

        public:

            using ArithmetizationType = crypto3::zk::snark::plonk_constraint_system<BlueprintFieldType>;
            using value_type = typename BlueprintFieldType::value_type;

            handler(std::vector<assignment<ArithmetizationType>> &assignments):
                    m_assignments(assignments) {

            }

            void set_witness(std::uint32_t table_idx, std::uint32_t column_idx, std::uint32_t row_idx, const uint256& v) override {
                assert(table_idx < m_assignments.size());
                m_assignments[table_idx].witness(column_idx, row_idx) = to_field(v);
            }

            void set_constant(std::uint32_t table_idx, std::uint32_t column_idx, std::uint32_t row_idx, const uint256& v) override {
                assert(table_idx < m_assignments.size());
                m_assignments[table_idx].constant(column_idx, row_idx) = to_field(v);
            }

            void set_public_input(std::uint32_t table_idx, std::uint32_t column_idx, std::uint32_t row_idx, const uint256& v) override {
                assert(table_idx < m_assignments.size());
                m_assignments[table_idx].public_input(column_idx, row_idx) = to_field(v);
            }

            void set_selector(std::uint32_t table_idx, std::uint32_t column_idx, std::uint32_t row_idx, const uint256& v) override {
                assert(table_idx < m_assignments.size());
                m_assignments[table_idx].selector(column_idx, row_idx) = to_field(v);
            }

            uint256 witness(std::uint32_t table_idx, std::uint32_t column_idx, std::uint32_t row_idx) override {
                return to_uint256(m_assignments[table_idx].witness(column_idx, row_idx));
            }

            uint256 constant(std::uint32_t table_idx, std::uint32_t column_idx, std::uint32_t row_idx) override {
                return to_uint256(m_assignments[table_idx].constant(column_idx, row_idx));
            }

            uint256 public_input(std::uint32_t table_idx, std::uint32_t column_idx, std::uint32_t row_idx) override {
                return to_uint256(m_assignments[table_idx].public_input(column_idx, row_idx));
            }

            uint256 selector(std::uint32_t table_idx, std::uint32_t column_idx, std::uint32_t row_idx) override {
                return to_uint256(m_assignments[table_idx].selector(column_idx, row_idx));
            }

            void test_zkevm_bytecode(
                std::vector<std::vector<std::uint8_t>> bytecodes
            ){
                std::cout << "MYINFO:: test_zkevm_bytecode" << std::endl;
                constexpr std::size_t PublicInputColumns = 1;
                constexpr std::size_t ConstantColumns = 4;
                constexpr std::size_t SelectorColumns = 3;
                constexpr std::size_t WitnessColumns = 15;
                crypto3::zk::snark::plonk_table_description<BlueprintFieldType> desc(
                    WitnessColumns, PublicInputColumns, ConstantColumns, SelectorColumns);
                using ArithmetizationType = crypto3::zk::snark::plonk_constraint_system<BlueprintFieldType>;
                using hash_type = nil::crypto3::hashes::keccak_1600<256>;
                constexpr std::size_t Lambda = 5;
                using AssignmentType = nil::blueprint::assignment<ArithmetizationType>;

                using value_type = typename BlueprintFieldType::value_type;
                using var = crypto3::zk::snark::plonk_variable<value_type>;

                using component_type = nil::blueprint::components::zkevm_bytecode<ArithmetizationType, BlueprintFieldType>;

                std::vector<std::vector<var>> bytecode_vars;
                std::size_t cur = 0;
                for (std::size_t i = 0; i < bytecodes.size(); i++) {
                    std::vector<var> bytecode;
                    bytecode.push_back(var(0, cur, false, var::column_type::public_input)); // length
                    cur++;
                    for (std::size_t j = 0; j < bytecodes[i].size(); j++, cur++) {
                        bytecode.push_back(var(0, cur, false, var::column_type::public_input));
                    }
                    std::cout << "Bytecode " << i << " size: " << bytecode.size() << std::endl;
                    bytecode_vars.push_back(bytecode);
                }
                std::vector<std::pair<var, var>> bytecode_hash_vars;
                for( std::size_t i = 0; i < bytecodes.size(); i++, cur+=2){
                    bytecode_hash_vars.push_back({var(0, cur, false, var::column_type::public_input), var(0, cur+1, false, var::column_type::public_input)});
                }
                var rlc_challenge_var = var(0, cur, false, var::column_type::public_input);
                typename component_type::input_type instance_input(bytecode_vars, bytecode_hash_vars, rlc_challenge_var);

                std::vector<value_type> public_input;
                cur = 0;
                for( std::size_t i = 0; i < bytecodes.size(); i++){
                    public_input.push_back(bytecodes[i].size());
                    cur++;
                    for( std::size_t j = 0; j < bytecodes[i].size(); j++, cur++){
                        public_input.push_back(bytecodes[i][j]);
                    }
                }
                for( std::size_t i = 0; i < bytecodes.size(); i++){
                    std::string hash = nil::crypto3::hash<nil::crypto3::hashes::keccak_1600<256>>(bytecodes[i].begin(), bytecodes[i].end());
                    std::string str_hi = hash.substr(0, hash.size()-32);
                    std::string str_lo = hash.substr(hash.size()-32, 32);
                    value_type hash_hi;
                    value_type hash_lo;
                    for( std::size_t j = 0; j < str_hi.size(); j++ ){hash_hi *=16; hash_hi += str_hi[j] >= '0' && str_hi[j] <= '9'? str_hi[j] - '0' : str_hi[j] - 'a' + 10;}
                    for( std::size_t j = 0; j < str_lo.size(); j++ ){hash_lo *=16; hash_lo += str_lo[j] >= '0' && str_lo[j] <= '9'? str_lo[j] - '0' : str_lo[j] - 'a' + 10;}
                    std::cout << std::hex <<  "Contract hash = " << hash << " h:" << hash_hi << " l:" << hash_lo << std::dec << std::endl;
                    public_input.push_back(hash_hi);
                    public_input.push_back(hash_lo);
                }
                nil::crypto3::random::algebraic_engine<BlueprintFieldType> rnd(0);
                value_type rlc_challenge = rnd();
                std::cout << std::hex << "rlc_challenge = " << rlc_challenge << std::dec << std::endl;
                public_input.push_back(rlc_challenge);

                auto result_check = [](AssignmentType &assignment,
                    typename component_type::result_type &real_res) {
                };

                std::array<std::uint32_t, WitnessColumns> witnesses;
                for (std::uint32_t i = 0; i < WitnessColumns; i++) {
                    witnesses[i] = i;
                }

                component_type component_instance = component_type(witnesses, std::array<std::uint32_t, 1>{0},
                                                                std::array<std::uint32_t, 1>{0}, 2046);
                nil::crypto3::test_component<component_type, BlueprintFieldType, hash_type, Lambda>
                    (component_instance, desc, public_input, result_check, instance_input,
                    nil::blueprint::connectedness_check_type::type::NONE, 2046);
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
        };

    }     // namespace blueprint
}    // namespace nil

#endif    // EVM1_ASSIGNER_INCLUDE_NIL_BLUEPRINT_HANDLER_BASE_HPP_
