//---------------------------------------------------------------------------//
// Copyright (c) Nil Foundation and its affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//---------------------------------------------------------------------------//

#ifndef EVM1_ASSIGNER_INCLUDE_NIL_BLUEPRINT_ZKEVM_WORD_HPP_
#define EVM1_ASSIGNER_INCLUDE_NIL_BLUEPRINT_ZKEVM_WORD_HPP_

#include <evmc/evmc.h>
#include <intx/intx.hpp>
#include <ethash/keccak.hpp>

#include <nil/crypto3/algebra/curves/pallas.hpp>
#include <nil/blueprint/blueprint/plonk/assignment.hpp>
#include <nil/marshalling/field_type.hpp>
#include <nil/marshalling/endianness.hpp>
#include <nil/crypto3/marshalling/algebra/types/field_element.hpp>

namespace nil {
    namespace blueprint {

        using Endianness = nil::marshalling::option::big_endian;
        using TTypeBase = nil::marshalling::field_type<Endianness>;

        template <typename BlueprintFieldType>
        struct zkevm_word {
            using column_type_t = typename crypto3::zk::snark::plonk_column<BlueprintFieldType>;
            using value_type = intx::uint256;
            // constructors
            zkevm_word() {
                value = 0;
            }

            zkevm_word(int v) {
                value = v;
            }

            zkevm_word(int64_t v) {
                value = v;
            }

            zkevm_word(size_t v) {
                value = v;
            }

            zkevm_word(const intx::uint256& v) {
                value = v;
            }

            zkevm_word(const evmc::uint256be& v) {
                value = intx::be::load<intx::uint256>(v);
            }

            zkevm_word(const evmc::address& addr) {
                value = intx::be::load<intx::uint256>(addr);
            }

            zkevm_word(const ethash::hash256& hash) {
                value = intx::be::load<intx::uint256>(hash);
            }

            zkevm_word(const uint8_t* data, size_t len) {
                value = intx::be::unsafe::load<intx::uint256>(data);
            }

            zkevm_word(column_type_t column_type, size_t column_idx, size_t row_idx, const assignment<BlueprintFieldType>& tbl) {
                typename BlueprintFieldType::value_type field_val;
                switch (column_type) {
                    case column_type_t::witness:
                        field_val = tbl.witness(column_idx, row_idx);
                        break;
                    case column_type_t::constant:
                        field_val = tbl.constant(column_idx, row_idx);
                        break;
                    case column_type_t::public_input:
                        field_val = tbl.public_input(column_idx, row_idx);
                        break;
                };
                auto field_container = nil::crypto3::marshalling::types::field_element<TTypeBase, typename BlueprintFieldType::value_type>(field_val);
                evmc::uint256be data;
                auto write_iter = data.bytes;
                field_container.write(write_iter, field_container.length());
                value = value = intx::be::load<intx::uint256>(data);
            }

            void put_into_assignment_table(column_type_t column_type, size_t column_idx, size_t row_idx, assignment<BlueprintFieldType>& tbl) const {
                auto field_container = nil::crypto3::marshalling::types::field_element<TTypeBase, typename BlueprintFieldType::value_type>();
                auto data = intx::be::store<evmc::uint256be>(value);
                auto read_iter = data.bytes;
                field_container.read(read_iter, field_container.length());
            }

            // operators
            zkevm_word<BlueprintFieldType> operator+(const zkevm_word<BlueprintFieldType>& other) const {
                return zkevm_word<BlueprintFieldType>(value + other.value);
            }

            zkevm_word<BlueprintFieldType> operator-(const zkevm_word<BlueprintFieldType>& other) const {
                return zkevm_word<BlueprintFieldType>(value - other.value);
            }

            zkevm_word<BlueprintFieldType> operator*(const zkevm_word<BlueprintFieldType>& other) const {
                return zkevm_word<BlueprintFieldType>(value * other.value);
            }

            zkevm_word<BlueprintFieldType> operator/(const zkevm_word<BlueprintFieldType>& other) const {
                return zkevm_word<BlueprintFieldType>(value / other.value);
            }

            zkevm_word<BlueprintFieldType> operator%(const zkevm_word<BlueprintFieldType>& other) const {
                return zkevm_word<BlueprintFieldType>(value % other.value);
            }

            zkevm_word<BlueprintFieldType> sdiv(const zkevm_word<BlueprintFieldType>& other) const {
                return other.value != 0 ? intx::sdivrem(value, other.value).quot : 0;
            }

            zkevm_word<BlueprintFieldType> smod(const zkevm_word<BlueprintFieldType>& other) const {
                return other.value != 0 ? intx::sdivrem(value, other.value).rem : 0;
            }

            zkevm_word<BlueprintFieldType> addmod(const zkevm_word<BlueprintFieldType>& other, const zkevm_word<BlueprintFieldType>& m) const {
                return m != 0 ? intx::addmod(value, other.value, m.value) : 0;
            }

            zkevm_word<BlueprintFieldType> mulmod(const zkevm_word<BlueprintFieldType>& other, const zkevm_word<BlueprintFieldType>& m) const {
                return m != 0 ? intx::mulmod(value, other.value, m.value) : 0;
            }

            zkevm_word<BlueprintFieldType> exp(const zkevm_word<BlueprintFieldType>& e) const {
                return intx::exp(value, e.value);
            }

            unsigned count_significant_bytes() const {
                return intx::count_significant_bytes(value);
            }

            bool slt(const zkevm_word<BlueprintFieldType>& other) const {
                return intx::slt(value, other.value);
            }

            bool operator==(const zkevm_word<BlueprintFieldType>& other) const {
                return value == other.value;
            }

            bool operator!=(const zkevm_word<BlueprintFieldType>& other) const {
                return value != other.value;
            }

            bool operator!=(int v) const {
                return value != v;
            }

            bool operator<(const zkevm_word<BlueprintFieldType>& other) const {
                return value < other.value;
            }

            bool operator<(int v) const {
                return value < v;
            }

            zkevm_word<BlueprintFieldType> operator&(const zkevm_word<BlueprintFieldType>& other) const {
                return value & other.value;
            }

            zkevm_word<BlueprintFieldType> operator|(const zkevm_word<BlueprintFieldType>& other) const {
                return value | other.value;
            }

            zkevm_word<BlueprintFieldType> operator^(const zkevm_word<BlueprintFieldType>& other) const {
                return value ^ other.value;
            }

            zkevm_word<BlueprintFieldType> operator~() const {
                return ~value;
            }

            zkevm_word<BlueprintFieldType> operator<<(const zkevm_word<BlueprintFieldType>& other) const {
                return value << other.value;
            }

            zkevm_word<BlueprintFieldType> operator>>(const zkevm_word<BlueprintFieldType>& other) const {
                return value >> other.value;
            }

            zkevm_word<BlueprintFieldType> operator<<(uint64_t v) const {
                return value << v;
            }

            zkevm_word<BlueprintFieldType> operator>>(uint64_t v) const {
                return value >> v;
            }

            // convertions
            uint64_t to_uint64(size_t i = 0) const {
                return static_cast<uint64_t>(value[i]);
            }

            evmc::address to_address() const {
                return intx::be::trunc<evmc::address>(value);
            }

            evmc::uint256be to_uint256be() const {
                return intx::be::store<evmc::uint256be>(value);
            }

            //partial size_t
            void set_val(uint64_t v, size_t i) {
                value[i] = v;
            }

            // memory
            template<typename T> // uint8, uint16, uint32, uint64, intx::uint256
            void store(uint8_t* data) const {
                intx::be::unsafe::store(data, static_cast<T>(value));
            }

            void load_partial_data(const uint8_t* data, size_t len, size_t i) {
                switch (len) {
                    case 1:
                        value[i] = intx::be::unsafe::load<uint8_t>(data);
                        break;
                    case 2:
                        value[i] = intx::be::unsafe::load<uint16_t>(data);
                        break;
                    case 3:
                        value[i] = intx::be::unsafe::load<uint32_t>(data) >> 8;
                        break;
                    case 4:
                        value[i] = intx::be::unsafe::load<uint32_t>(data);
                        break;
                    case 5:
                    case 6:
                    case 7:
                    case 8:
                        value[i] = intx::be::unsafe::load<uint64_t>(data) >> (8 * (sizeof(uint64_t) - len));
                        break;
                };
            }

            const uint8_t* raw_data() const {
                return reinterpret_cast<const uint8_t *>(&value);
            }

            uint64_t size() const {
                return sizeof(value);
            }
        private:
                intx::uint256 value;
        };
    }     // namespace blueprint
}    // namespace nil
#endif    // EVM1_ASSIGNER_INCLUDE_NIL_BLUEPRINT_ZKEVM_WORD_HPP_
