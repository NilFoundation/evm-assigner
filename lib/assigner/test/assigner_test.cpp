#include <map>

#include <nil/blueprint/assigner.hpp>
#include <nil/blueprint/blueprint/plonk/assignment.hpp>

#include <evmc/evmc.hpp>
#include <evmone/evmone.h>
#include <evmc/mocked_host.hpp>
#include "instructions_opcodes.hpp"
#include "vm_host.h"

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

        assignments.emplace_back(desc);
        assignments.emplace_back(desc); // for check create, call op codes, where message depth = 1

        assigner_ptr =
            std::make_shared<nil::blueprint::assigner<BlueprintFieldType>>(assignments);

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
            .create2_salt = {0},
            .code_address = code_addr
        };


        host_interface = &evmc::Host::get_interface();
        ctx = vm_host_create_context<BlueprintFieldType>(tx_context, assigner_ptr);
    }

    static void TearDownTestSuite()
    {
        assigner_ptr.reset();
        vm_host_destroy_context<BlueprintFieldType>(ctx);
    }

    static std::shared_ptr<nil::blueprint::assigner<BlueprintFieldType>> assigner_ptr;
    static std::vector<nil::blueprint::assignment<ArithmetizationType>> assignments;
    static const struct evmc_host_interface* host_interface;
    static struct evmc_host_context* ctx;
    static evmc_revision rev;
    static struct evmc_message msg;
};

std::shared_ptr<nil::blueprint::assigner<AssignerTest::BlueprintFieldType>>
    AssignerTest::assigner_ptr;
std::vector<nil::blueprint::assignment<AssignerTest::ArithmetizationType>>
    AssignerTest::assignments;
const struct evmc_host_interface* AssignerTest::host_interface;
struct evmc_host_context* AssignerTest::ctx;
evmc_revision AssignerTest::rev = {};
struct evmc_message AssignerTest::msg;

inline void check_eq(const uint8_t* l, const uint8_t* r, size_t len) {
    for (int i = 0; i < len; i++) {
        EXPECT_EQ(l[i], r[i]);
    }
}

TEST_F(AssignerTest, conversions_uint256be_to_zkevm_word)
{
    evmc::uint256be uint256be_number;
    uint256be_number.bytes[2] = 10;  // Some big number, 10 << 128
    // conversion to zkevm_word
    auto tmp = nil::blueprint::zkevm_word<BlueprintFieldType>(uint256be_number);
    // conversion back to uint256be
    evmc::uint256be uint256be_result = tmp.to_uint256be();
    // check if same
    check_eq(uint256be_number.bytes, uint256be_result.bytes, 32);
}

TEST_F(AssignerTest, conversions_address_to_zkevm_word)
{
    evmc::address address;
    address.bytes[19] = 10;
    // conversion to zkevm_word
    auto tmp = nil::blueprint::zkevm_word<BlueprintFieldType>(address);
    // conversion back to address
    evmc::address address_result = tmp.to_address();
    // check if same
    check_eq(address.bytes, address_result.bytes, 20);
}

TEST_F(AssignerTest, conversions_hash_to_zkevm_word)
{
    ethash::hash256 hash;
    hash.bytes[2] = 10;
    // conversion to zkevm_word
    auto tmp = nil::blueprint::zkevm_word<BlueprintFieldType>(hash);
    // conversion back to address
    ethash::hash256 hash_result = tmp.to_hash();
    // check if same
    check_eq(hash.bytes, hash_result.bytes, 32);
}

TEST_F(AssignerTest, conversions_uint64_to_zkevm_word)
{
    uint64_t number = std::numeric_limits<uint64_t>::max();
    // conversion to zkevm_word
    auto tmp = nil::blueprint::zkevm_word<BlueprintFieldType>(number);
    // conversion back to address
    auto number_result = tmp.to_uint64();
    // check if same
    EXPECT_EQ(number_result, number);
}

TEST_F(AssignerTest, load_store_zkevm_word)
{
    uint64_t number = std::numeric_limits<uint64_t>::max();

    nil::blueprint::zkevm_word<BlueprintFieldType> tmp;
    // load data to the lase 64 bits
    tmp.load_partial_data(reinterpret_cast<const uint8_t*>(&number), 8, 0);
    uint64_t number_result;
    // store back to the uint64_t variable
    tmp.store<uint64_t>(reinterpret_cast<uint8_t*>(&number_result));
    // check if same
    EXPECT_EQ(number_result, number);
}

TEST_F(AssignerTest, mul) {

    std::vector<uint8_t> code = {
        evmone::OP_PUSH1,
        4,
        evmone::OP_PUSH1,
        8,
        evmone::OP_MUL,
    };

    nil::blueprint::evaluate<BlueprintFieldType>(host_interface, ctx, rev, &msg, code.data(), code.size(), assigner_ptr);
    EXPECT_EQ(assignments[0].witness(0, 0), 8);
    EXPECT_EQ(assignments[0].witness(0, 1), 4);
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
    nil::blueprint::evaluate<BlueprintFieldType>(host_interface, ctx, rev, &msg, code.data(), code.size(), assigner_ptr);
    //EXPECT_EQ(assignments[0].witness(1, 0), nil::blueprint::to_field<BlueprintFieldType>(msg.value));
    EXPECT_EQ(assignments[0].witness(1, 1), index);
}

// TODO: test dataload instruction (for now its cost is not defined)
TEST_F(AssignerTest, DISABLED_dataload) {
    const uint8_t index = 10;
    std::vector<uint8_t> code = {
        evmone::OP_PUSH1,
        index,
        evmone::OP_DATALOAD,
    };
    nil::blueprint::evaluate<BlueprintFieldType>(host_interface, ctx, rev, &msg, code.data(), code.size(), assigner_ptr);
    EXPECT_EQ(assignments[0].witness(1, 2), index);
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
    nil::blueprint::evaluate<BlueprintFieldType>(host_interface, ctx, rev, &msg, code.data(), code.size(), assigner_ptr);
    EXPECT_EQ(assignments[0].witness(2, 0), value);
    EXPECT_EQ(assignments[0].witness(2, 1), index);
    EXPECT_EQ(assignments[0].witness(2, 2), value);
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
    nil::blueprint::evaluate<BlueprintFieldType>(host_interface, ctx, rev, &msg, code.data(), code.size(), assigner_ptr);
    EXPECT_EQ(assignments[0].witness(3, 0), value);
    EXPECT_EQ(assignments[0].witness(3, 1), key);
    EXPECT_EQ(assignments[0].witness(3, 2), value);
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
    nil::blueprint::evaluate<BlueprintFieldType>(host_interface, ctx, rev, &msg, code.data(), code.size(), assigner_ptr);
    EXPECT_EQ(assignments[0].witness(4, 0), value);
    EXPECT_EQ(assignments[0].witness(4, 1), key);
    EXPECT_EQ(assignments[0].witness(4, 2), value);
}

TEST_F(AssignerTest, create) {

    std::vector<uint8_t> code = {
        // Create an account with 0 wei and no code
        evmone::OP_PUSH1,
        0,
        evmone::OP_PUSH1,
        0,
        evmone::OP_PUSH1,
        0,
        evmone::OP_PUSH1,
        0,
        evmone::OP_CREATE,

        // Create an account with 9 wei and no code
        evmone::OP_PUSH1,
        1,
        evmone::OP_PUSH1,
        0,
        evmone::OP_PUSH1,
        0,
        evmone::OP_PUSH1,
        9,
        evmone::OP_CREATE,

        // Create an account with 0 wei and 4 FF as code
        evmone::OP_PUSH13,
        // code 0x63FFFFFFFF60005260046000F3 will be inserted later

        evmone::OP_PUSH1,
        0,
        evmone::OP_MSTORE,
        evmone::OP_PUSH1,
        2,
        evmone::OP_PUSH1,
        13,
        evmone::OP_PUSH1,
        19,
        evmone::OP_PUSH1,
        0,
        evmone::OP_CREATE,
    };

    auto push13_it = std::find(code.begin(), code.end(), evmone::OP_PUSH13);
    ASSERT_NE(push13_it, code.end());
    size_t push13_idx = static_cast<size_t>(push13_it - code.begin());

    std::vector<uint8_t> contract_code = {0x63, 0xff, 0xff, 0xff, 0xff, 0x60, 0x00, 0x52, 0x60, 0x04, 0x60, 0x00, 0xf3};
    // Code is in the last 13 bytes of the container
    code.insert(code.begin() + static_cast<long int>(push13_idx) + 1, contract_code.begin(), contract_code.end());

    nil::blueprint::evaluate<BlueprintFieldType>(host_interface, ctx, rev, &msg, code.data(), code.size(), assigner_ptr);
    // Check stored witnesses of MSTORE instruction at depth 1
    EXPECT_EQ(assignments[1].witness(2, 1), 0);
    EXPECT_EQ(assignments[1].witness(2, 0), 0xFFFFFFFF);
}

TEST_F(AssignerTest, call) {

    std::vector<uint8_t> code = {
        // Create a contract that creates an exception if first word of calldata is 0
        evmone::OP_PUSH17,
        // code 0x67600035600757FE5B60005260086018F3 will be inserted later
        evmone::OP_PUSH1,
        0,

        evmone::OP_MSTORE,
        evmone::OP_PUSH1,
        17,
        evmone::OP_PUSH1,
        15,
        evmone::OP_PUSH1,
        0,
        evmone::OP_CREATE,

        // Call with no parameters, return 0
        evmone::OP_PUSH1,
        0,
        evmone::OP_PUSH1,
        0,
        evmone::OP_PUSH1,
        0,
        evmone::OP_PUSH1,
        0,
        evmone::OP_PUSH1,
        0,
        evmone::OP_DUP6,
        evmone::OP_PUSH2,
        0xFF,
        0xFF,
        evmone::OP_CALL,

        // Call with non 0 calldata, returns success
        evmone::OP_PUSH1,
        0,
        evmone::OP_PUSH1,
        0,
        evmone::OP_PUSH1,
        32,
        evmone::OP_PUSH1,
        0,
        evmone::OP_PUSH1,
        0,
        evmone::OP_DUP7,
        evmone::OP_PUSH2,
        0xFF,
        0xFF,
        evmone::OP_CALL,
    };

    auto push17_it = std::find(code.begin(), code.end(), evmone::OP_PUSH17);
    ASSERT_NE(push17_it, code.end());
    size_t push17_idx = static_cast<size_t>(push17_it - code.begin());

    std::vector<uint8_t> contract_code = {0x67, 0x60, 0x00, 0x35, 0x60, 0x07, 0x57, 0xfe, 0x5b, 0x60, 0x00, 0x52, 0x60, 0x08, 0x60, 0x18, 0xf3};
    // Code is in the last 13 bytes of the container
    code.insert(code.begin() + static_cast<long int>(push17_idx) + 1, contract_code.begin(), contract_code.end());

    nil::blueprint::evaluate<BlueprintFieldType>(host_interface, ctx, rev, &msg, code.data(), code.size(), assigner_ptr);
    // Check stored witness of CALLDATALOAD instruction at depth 1
    EXPECT_EQ(assignments[1].witness(1, 1), 0);
}
