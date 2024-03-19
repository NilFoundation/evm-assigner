//---------------------------------------------------------------------------//
// Copyright (c) Nil Foundation and its affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//---------------------------------------------------------------------------//

#ifndef EVM1_ASSIGNER_INCLUDE_NIL_BLUEPRINT_HANDLER_BASE_HPP_
#define EVM1_ASSIGNER_INCLUDE_NIL_BLUEPRINT_HANDLER_BASE_HPP_

#include <intx/intx.hpp>

namespace nil {
    namespace blueprint {
        using uint256 = intx::uint256;

        class handler_base {

        public:

            virtual void set_witness(std::size_t table_idx, std::size_t column_idx, std::size_t row_idx, const uint256& v) = 0;
            virtual void set_constant(std::size_t table_idx, std::size_t column_idx, std::size_t row_idx, const uint256& v) = 0;
            virtual void set_public_input(std::size_t table_idx, std::size_t column_idx, std::size_t row_idx, const uint256& v) = 0;
            virtual void set_selector(std::size_t table_idx, std::size_t column_idx, std::size_t row_idx, const uint256& v) = 0;

            virtual uint256 witness(std::size_t table_idx, std::size_t column_idx, std::size_t row_idx) = 0;
            virtual uint256 constant(std::size_t table_idx, std::size_t column_idx, std::size_t row_idx) = 0;
            virtual uint256 public_input(std::size_t table_idx, std::size_t column_idx, std::size_t row_idx) = 0;
            virtual uint256 selector(std::size_t table_idx, std::size_t column_idx, std::size_t row_idx) = 0;
        };

    }     // namespace blueprint
}    // namespace nil

#endif    // EVM1_ASSIGNER_INCLUDE_NIL_BLUEPRINT_HANDLER_BASE_HPP_
