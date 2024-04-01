#include <map>

#include <nil/blueprint/assigner.hpp>
#include <nil/blueprint/blueprint/plonk/assignment.hpp>

#include <evmc/evmc.hpp>
#include <evmc/mocked_host.hpp>
#include "instructions_opcodes.hpp"
#include "test_host.h"

#include <gtest/gtest.h>

class AssignerTest : public testing::Test
{
public:
    using BlueprintFieldType = typename nil::crypto3::algebra::curves::pallas::base_field_type;
    using ArithmetizationType = nil::crypto3::zk::snark::plonk_constraint_system<BlueprintFieldType>;
    static void SetUpTestSuite()
    {
        const std::size_t WitnessColumns = 15;
        const std::size_t PublicInputColumns = 1;

        const std::size_t ConstantColumns = 5;
        const std::size_t SelectorColumns = 30;

        nil::crypto3::zk::snark::plonk_table_description<BlueprintFieldType> desc(
            WitnessColumns, PublicInputColumns, ConstantColumns, SelectorColumns);

        assigner_ptr =
            std::make_unique<nil::blueprint::assigner<BlueprintFieldType>>(desc, assignments);

        vm = evmc_create_evmone();


        const uint8_t input[] = "Hello World!";
        const evmc_uint256be value = {{1, 0}};
        const evmc_address sender_addr = {{0, 1, 2}};
        const evmc_address recipient_addr = {{1, 1, 2}};
        const evmc_address code_addr = {{2, 1, 2}};
        const int64_t gas = 200000;
        struct evmc_tx_context tx_context = {
            .block_number = 42,
            .block_timestamp = 66,
            .block_gas_limit = gas * 2,
        };

        msg = evmc_message {
            .kind = EVMC_CALL,
            .flags = 0,
            .depth = 0,
            .gas = gas,
            .recipient = recipient_addr,
            .sender = sender_addr,
            .input_data = input,
            .input_size = sizeof(input),
            .value = value,
            .create2_salt = 0,
            .code_address = code_addr
        };


        host_interface = &evmc::Host::get_interface();
        ctx = example_host_create_context(tx_context);
    }

    static void TearDownTestSuite()
    {
        assigner_ptr.reset();
    }

    static std::unique_ptr<nil::blueprint::assigner<BlueprintFieldType>> assigner_ptr;
    static std::vector<nil::blueprint::assignment<ArithmetizationType>> assignments;
    static const struct evmc_host_interface* host_interface;
    static struct evmc_host_context* ctx;
    static struct evmc_vm* vm;
    static evmc_revision rev;
    static struct evmc_message msg;
};

std::unique_ptr<nil::blueprint::assigner<AssignerTest::BlueprintFieldType>>
    AssignerTest::assigner_ptr;
std::vector<nil::blueprint::assignment<AssignerTest::ArithmetizationType>>
    AssignerTest::assignments;
const struct evmc_host_interface* AssignerTest::host_interface;
struct evmc_host_context* AssignerTest::ctx;
struct evmc_vm* AssignerTest::vm;
evmc_revision AssignerTest::rev = {};
struct evmc_message AssignerTest::msg;

TEST_F(AssignerTest, conversions)
{
    intx::uint256 intx_number;
    intx_number[2] = 10;  // Some big number, 10 << 128
    auto field = nil::blueprint::handler<BlueprintFieldType>::to_field(intx_number);
    // Compare string representations
    std::ostringstream oss;
    oss << field.data;
    EXPECT_EQ(intx::to_string(intx_number), oss.str());
    oss.str("");

    // Modify field and test conversion to uint256
    field *= 3;
    intx_number = nil::blueprint::handler<BlueprintFieldType>::to_uint256(field);
    oss << field.data;
    EXPECT_EQ(intx::to_string(intx_number), oss.str());
    oss.str("");
}

TEST_F(AssignerTest, mul) {

    std::vector<uint8_t> code = {
        evmone::OP_PUSH1,
        4,
        evmone::OP_PUSH1,
        8,
        evmone::OP_MUL,
    };

    assigner_ptr->evaluate(vm, host_interface, ctx, rev, &msg, code.data(), code.size());
    EXPECT_EQ(assignments[0].witness(0, 0), 8);
    EXPECT_EQ(assignments[0].witness(1, 0), 4);
}

TEST_F(AssignerTest, callvalue_calldataload)
{
    const uint8_t index = 10;
    std::vector<uint8_t> code = {
        evmone::OP_CALLVALUE,
        evmone::OP_PUSH1,
        index,
        evmone::OP_CALLDATALOAD,
    };
    assigner_ptr->evaluate(vm, host_interface, ctx, rev, &msg, code.data(), code.size());
    EXPECT_EQ(nil::blueprint::handler<BlueprintFieldType>::to_uint256(assignments[1].witness(0, 0)),
        intx::be::load<intx::uint256>(msg.value));
    EXPECT_EQ(assignments[1].witness(0, 2), index);
}

// TODO: test dataload instruction (for now its cost is not defined)
TEST_F(AssignerTest, DISABLED_dataload) {
    const uint8_t index = 10;
    std::vector<uint8_t> code = {
        evmone::OP_PUSH1,
        index,
        evmone::OP_DATALOAD,
    };
    assigner_ptr->evaluate(vm, host_interface, ctx, rev, &msg, code.data(), code.size());
    EXPECT_EQ(assignments[1].witness(0, 1), index);
}

TEST_F(AssignerTest, mstore_load)
{
    const uint8_t value = 12;
    const uint8_t index = 1;
    std::vector<uint8_t> code = {
        evmone::OP_PUSH1,
        value,
        evmone::OP_PUSH1,
        index,
        evmone::OP_MSTORE,
        evmone::OP_PUSH1,
        index,
        evmone::OP_MLOAD,
    };
    assigner_ptr->evaluate(vm, host_interface, ctx, rev, &msg, code.data(), code.size());
    EXPECT_EQ(assignments[2].witness(0, 0), value);
    EXPECT_EQ(assignments[2].witness(0, 1), index);
    EXPECT_EQ(assignments[2].witness(0, 2), value);
}

TEST_F(AssignerTest, sstore_load)
{
    const uint8_t value = 12;
    const uint8_t key = 4;
    std::vector<uint8_t> code = {
        evmone::OP_PUSH1,
        value,
        evmone::OP_PUSH1,
        key,
        evmone::OP_SSTORE,
        evmone::OP_PUSH1,
        key,
        evmone::OP_SLOAD,
    };
    assigner_ptr->evaluate(vm, host_interface, ctx, rev, &msg, code.data(), code.size());
    EXPECT_EQ(assignments[3].witness(0, 0), value);
    EXPECT_EQ(assignments[3].witness(0, 1), key);
    EXPECT_EQ(assignments[3].witness(0, 2), value);
}

// TODO: test transient storage opcodes (for now their costs are not defined)
TEST_F(AssignerTest, DISABLED_tstore_load) {
    const uint8_t value = 12;
    const uint8_t key = 4;
    std::vector<uint8_t> code = {
        evmone::OP_PUSH1,
        value,
        evmone::OP_PUSH1,
        key,
        evmone::OP_TSTORE,
        evmone::OP_PUSH1,
        key,
        evmone::OP_TLOAD,
    };
    assigner_ptr->evaluate(vm, host_interface, ctx, rev, &msg, code.data(), code.size());
    EXPECT_EQ(assignments[3].witness(1, 0), value);
    EXPECT_EQ(assignments[3].witness(1, 1), key);
    EXPECT_EQ(assignments[3].witness(1, 2), value);
}
