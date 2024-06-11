//---------------------------------------------------------------------------//
// Copyright (c) Nil Foundation and its affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//---------------------------------------------------------------------------//

#ifndef EVM1_ASSIGNER_INCLUDE_NIL_BLUEPRINT_HANDLER_BASE_HPP_
#define EVM1_ASSIGNER_INCLUDE_NIL_BLUEPRINT_HANDLER_BASE_HPP_

#include <intx/intx.hpp>
#include <evmc/evmc.hpp>

#include <memory>

namespace nil {
    namespace blueprint {
        using uint256 = intx::uint256;

        class handler_base {

        public:

            virtual void set_witness(std::uint32_t table_idx, std::uint32_t column_idx, std::uint32_t row_idx, const uint256& v) = 0;
            virtual void set_constant(std::uint32_t table_idx, std::uint32_t column_idx, std::uint32_t row_idx, const uint256& v) = 0;
            virtual void set_public_input(std::uint32_t table_idx, std::uint32_t column_idx, std::uint32_t row_idx, const uint256& v) = 0;
            virtual void set_selector(std::uint32_t table_idx, std::uint32_t column_idx, std::uint32_t row_idx, const uint256& v) = 0;
            virtual void test_zkevm_bytecode(std::vector<std::vector<std::uint8_t>> bytecodes) = 0;

            virtual uint256 witness(std::uint32_t table_idx, std::uint32_t column_idx, std::uint32_t row_idx) = 0;
            virtual uint256 constant(std::uint32_t table_idx, std::uint32_t column_idx, std::uint32_t row_idx) = 0;
            virtual uint256 public_input(std::uint32_t table_idx, std::uint32_t column_idx, std::uint32_t row_idx) = 0;
            virtual uint256 selector(std::uint32_t table_idx, std::uint32_t column_idx, std::uint32_t row_idx) = 0;
        };

        evmc::Result evaluate(std::shared_ptr<handler_base> handler, evmc_vm* c_vm, const evmc_host_interface* host, evmc_host_context* ctx,
            evmc_revision rev, const evmc_message* msg, const uint8_t* code, size_t code_size);

    }     // namespace blueprint
}    // namespace nil

#endif    // EVM1_ASSIGNER_INCLUDE_NIL_BLUEPRINT_HANDLER_BASE_HPP_
