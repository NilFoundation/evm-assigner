#include <map>

#include <assigner.hpp>
#include <nil/blueprint/blueprint/plonk/assignment.hpp>

#include <evmc/evmc.hpp>
#include <evmc/mocked_host.hpp>
#include <instructions_opcodes.hpp>
#include <vm_host.hpp>

#include <gtest/gtest.h>

const uint8_t global_input_variable_for_test[13] = "Hello World!";

class AssignerTest : public testing::Test
{
public:
    using BlueprintFieldType = typename nil::crypto3::algebra::curves::pallas::base_field_type;
    using ArithmetizationType = nil::crypto3::zk::snark::plonk_constraint_system<BlueprintFieldType>;
    static void SetUpTestSuite()
    {
        const std::size_t WitnessColumns = 65;
        const std::size_t PublicInputColumns = 1;

        const std::size_t ConstantColumns = 5;
        const std::size_t SelectorColumns = 30;

        nil::crypto3::zk::snark::plonk_table_description<BlueprintFieldType> desc(
            WitnessColumns, PublicInputColumns, ConstantColumns, SelectorColumns);

        assignments.emplace_back(desc);
        assignments.emplace_back(desc); // for check create, call op codes, where message depth = 1

        assigner_ptr =
            std::make_shared<nil::evm_assigner::assigner<BlueprintFieldType>>(assignments);

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
            .input_data = global_input_variable_for_test,
            .input_size = sizeof(global_input_variable_for_test),
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

    static std::shared_ptr<nil::evm_assigner::assigner<BlueprintFieldType>> assigner_ptr;
    static std::vector<nil::blueprint::assignment<ArithmetizationType>> assignments;
    static const struct evmc_host_interface* host_interface;
    static struct evmc_host_context* ctx;
    static evmc_revision rev;
    static struct evmc_message msg;
};

template<typename BlueprintFieldType>
struct TestRWCircuitStruct {
    std::vector<uint8_t> code;
    const evmone::bytes_view container;
    const evmone::baseline::CodeAnalysis code_analysis;
    const evmone::bytes_view data;
    evmone::ExecutionState<BlueprintFieldType> state;
    evmone::baseline::Position<BlueprintFieldType> position;

    TestRWCircuitStruct(
        const uint8_t opcode,
        AssignerTest* testInstance
    ) :
        code{opcode},
        container(code.data(), code.size()),
        code_analysis(evmone::baseline::analyze(testInstance->rev, container)),
        data(code_analysis.eof_header.get_data(container)),
        state(
            testInstance->msg,
            testInstance->rev,
            *(testInstance->host_interface),
            testInstance->ctx,
            container,
            data,
            0,
            testInstance->assigner_ptr),
        position(code.data(), state.stack_space.bottom()) {}
};

std::shared_ptr<nil::evm_assigner::assigner<AssignerTest::BlueprintFieldType>>
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

inline void rw_circuit_check(const std::vector<nil::blueprint::assignment<AssignerTest::ArithmetizationType>>& assignments,
                             uint32_t start_row_index,
                             uint8_t operation_type,
                             uint32_t call_id,
                             const typename AssignerTest::BlueprintFieldType::value_type& address,
                             const typename AssignerTest::BlueprintFieldType::value_type& stoage_key_hi,
                             const typename AssignerTest::BlueprintFieldType::value_type& stoage_key_lo,
                             uint32_t rw_id,
                             bool is_write,
                             const typename AssignerTest::BlueprintFieldType::value_type& value_hi,
                             const typename AssignerTest::BlueprintFieldType::value_type& value_lo) {
    auto& rw_table =
        assignments[nil::evm_assigner::assigner<AssignerTest::BlueprintFieldType>::RW_TABLE_INDEX];
    std::cout << "check values on row " << start_row_index << " \n";
    // OP_TYPE
    EXPECT_EQ(rw_table.witness(0, start_row_index), operation_type);
    // CALL ID
    EXPECT_EQ(rw_table.witness(1, start_row_index), call_id);
    // address (stack size)
    EXPECT_EQ(rw_table.witness(2, start_row_index), address);
    // storage key hi
    EXPECT_EQ(rw_table.witness(3, start_row_index), stoage_key_hi);
    // storage key lo
    EXPECT_EQ(rw_table.witness(4, start_row_index), stoage_key_lo);
    // RW ID
    EXPECT_EQ(rw_table.witness(6, start_row_index), rw_id);
    // is write
    EXPECT_EQ(rw_table.witness(7, start_row_index), (is_write ? 1 : 0));
    // value hi
    EXPECT_EQ(rw_table.witness(8, start_row_index), value_hi);
    // value lo
    EXPECT_EQ(rw_table.witness(9, start_row_index), value_lo);
}

inline std::string bytes_to_string(const uint8_t *data, int len)
{
    std::stringstream ss;
    ss << std::hex;

    for (int i(0); i < len; ++i) {
        ss << std::setw(2) << std::setfill('0') << (int)data[i];
    }

    // Cut all preceding zeros
    std::string res = ss.str();
    while (res[0] == '0') res.erase(0,1);

    return res;
}

inline void stack_rw_circuit_check(const std::vector<nil::blueprint::assignment<AssignerTest::ArithmetizationType>>& assignments,
                             uint32_t start_row_index,
                             const typename AssignerTest::BlueprintFieldType::value_type& address,
                             uint32_t rw_id,
                             bool is_write,
                             const typename AssignerTest::BlueprintFieldType::value_type& value_hi,
                             const typename AssignerTest::BlueprintFieldType::value_type& value_lo) {
    rw_circuit_check(assignments, start_row_index, 1/*STACK_OP*/, 0/*CALL_ID*/, address/*address in stack*/, 0/*storage key hi*/, 0/*storage key lo*/, rw_id/*trace size*/, is_write/*is_write*/, value_hi/*value_hi*/, value_lo/*value_lo*/);
}

inline void memory_rw_circuit_check(const std::vector<nil::blueprint::assignment<AssignerTest::ArithmetizationType>>& assignments,
                             uint32_t start_row_index,
                             const typename AssignerTest::BlueprintFieldType::value_type& address,
                             uint32_t rw_id,
                             bool is_write,
                             const typename AssignerTest::BlueprintFieldType::value_type& value_hi,
                             const typename AssignerTest::BlueprintFieldType::value_type& value_lo) {
    rw_circuit_check(assignments, start_row_index, 2/*MEMORY_OP*/, 0/*CALL_ID*/, address/*address in memory*/, 0/*storage key hi*/, 0/*storage key lo*/, rw_id/*trace size*/, is_write/*is_write*/, value_hi/*value_hi*/, value_lo/*value_lo*/);
}

inline void bytecode_circuit_check(const std::vector<nil::blueprint::assignment<AssignerTest::ArithmetizationType>>& assignments,
    uint32_t start_row_index,
    const typename AssignerTest::BlueprintFieldType::value_type& tag,
    const typename AssignerTest::BlueprintFieldType::value_type& index,
    const typename AssignerTest::BlueprintFieldType::value_type& value,
    const typename AssignerTest::BlueprintFieldType::value_type& is_opcode,
    const typename AssignerTest::BlueprintFieldType::value_type& push_size,
    const typename AssignerTest::BlueprintFieldType::value_type& length_left,
    const typename AssignerTest::BlueprintFieldType::value_type& hash_hi,
    const typename AssignerTest::BlueprintFieldType::value_type& hash_lo,
    const typename AssignerTest::BlueprintFieldType::value_type& value_rlc,
    const typename AssignerTest::BlueprintFieldType::value_type& rlc_challenge
) {
    EXPECT_EQ(assignments[0].witness(0, start_row_index), tag);
    EXPECT_EQ(assignments[0].witness(1, start_row_index), index);
    EXPECT_EQ(assignments[0].witness(2, start_row_index), value);
    EXPECT_EQ(assignments[0].witness(3, start_row_index), is_opcode);
    EXPECT_EQ(assignments[0].witness(4, start_row_index), push_size);
    EXPECT_EQ(assignments[0].witness(5, start_row_index), length_left);
    EXPECT_EQ(assignments[0].witness(6, start_row_index), hash_hi);
    EXPECT_EQ(assignments[0].witness(7, start_row_index), hash_lo);
    EXPECT_EQ(assignments[0].witness(8, start_row_index), value_rlc);
    EXPECT_EQ(assignments[0].witness(9, start_row_index), rlc_challenge);
}

TEST_F(AssignerTest, mul_bytecode) {

    std::vector<uint8_t> code = {
        evmone::OP_PUSH1,
        4,
        evmone::OP_PUSH1,
        8,
        evmone::OP_MUL,
    };

    assigner_ptr->handle_bytecode(code.size(), code.data());

    const typename AssignerTest::BlueprintFieldType::value_type hash_hi = 0xa496800ccbfa34512f79b5fc4faea679_cppui_modular255;
    const typename AssignerTest::BlueprintFieldType::value_type hash_lo = 0x544dfa344b6d9f3923362f5615959272_cppui_modular255;

    uint32_t start_row_index = 0;

    bytecode_circuit_check(assignments, start_row_index++, 0/*TAG*/, 0/*INDEX*/, 96/*VALUE*/, 0/*IS_OPCODE*/, 0/*PUSH_SIZE*/, 96/*LENGTH_LEFT*/, hash_hi/*HASH_HI*/, hash_lo/*HASH_LO*/, 0    /*VALUE_RLC*/, 15/*RLC_CHALLENGE*/);
    bytecode_circuit_check(assignments, start_row_index++, 1/*TAG*/, 0/*INDEX*/, 4 /*VALUE*/, 1/*IS_OPCODE*/, 0/*PUSH_SIZE*/, 95/*LENGTH_LEFT*/, hash_hi/*HASH_HI*/, hash_lo/*HASH_LO*/, 4    /*VALUE_RLC*/, 15/*RLC_CHALLENGE*/);
    bytecode_circuit_check(assignments, start_row_index++, 1/*TAG*/, 1/*INDEX*/, 96/*VALUE*/, 1/*IS_OPCODE*/, 1/*PUSH_SIZE*/, 94/*LENGTH_LEFT*/, hash_hi/*HASH_HI*/, hash_lo/*HASH_LO*/, 156  /*VALUE_RLC*/, 15/*RLC_CHALLENGE*/);
    bytecode_circuit_check(assignments, start_row_index++, 1/*TAG*/, 2/*INDEX*/, 8 /*VALUE*/, 0/*IS_OPCODE*/, 0/*PUSH_SIZE*/, 93/*LENGTH_LEFT*/, hash_hi/*HASH_HI*/, hash_lo/*HASH_LO*/, 2348 /*VALUE_RLC*/, 15/*RLC_CHALLENGE*/);
    bytecode_circuit_check(assignments, start_row_index++, 1/*TAG*/, 3/*INDEX*/, 2 /*VALUE*/, 1/*IS_OPCODE*/, 0/*PUSH_SIZE*/, 92/*LENGTH_LEFT*/, hash_hi/*HASH_HI*/, hash_lo/*HASH_LO*/, 35222/*VALUE_RLC*/, 15/*RLC_CHALLENGE*/);
}

TEST_F(AssignerTest, conversions_uint256be_to_zkevm_word)
{
    evmc::uint256be uint256be_number;
    uint256be_number.bytes[2] = 10;  // Some big number, 10 << 128
    // conversion to zkevm_word
    auto tmp = nil::evm_assigner::zkevm_word<BlueprintFieldType>(uint256be_number);
    // compare string representations of the data
    ASSERT_EQ(bytes_to_string(uint256be_number.bytes, 32), intx::to_string(tmp.get_value(), 16));
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
    auto tmp = nil::evm_assigner::zkevm_word<BlueprintFieldType>(address);
    // compare string representations of the data
    ASSERT_EQ(bytes_to_string(address.bytes, 20), intx::to_string(tmp.get_value(), 16));
    // conversion back to address
    evmc::address address_result = tmp.to_address();
    // check if same
    check_eq(address.bytes, address_result.bytes, 20);
    // Check conversion to field
    std::ostringstream ss;
    ss << std::hex << tmp.to_field_as_address();
    ASSERT_EQ(bytes_to_string(address.bytes, 20), ss.str());
}

TEST_F(AssignerTest, conversions_hash_to_zkevm_word)
{
    ethash::hash256 hash;
    hash.bytes[2] = 10;
    // conversion to zkevm_word
    auto tmp = nil::evm_assigner::zkevm_word<BlueprintFieldType>(hash);
    // compare string representations of the data
    ASSERT_EQ(bytes_to_string(hash.bytes, 32), intx::to_string(tmp.get_value(), 16));
    // conversion back to address
    ethash::hash256 hash_result = tmp.to_hash();
    // check if same
    check_eq(hash.bytes, hash_result.bytes, 32);
}

TEST_F(AssignerTest, conversions_uint64_to_zkevm_word)
{
    uint64_t number = std::numeric_limits<uint64_t>::max();
    // conversion to zkevm_word
    auto tmp = nil::evm_assigner::zkevm_word<BlueprintFieldType>(number);
    // compare string representations of the data
    std::ostringstream ss;
    ss << std::hex << number;
    ASSERT_EQ(ss.str(), intx::to_string(tmp.get_value(), 16));
    // conversion back to address
    auto number_result = tmp.to_uint64();
    // check if same
    EXPECT_EQ(number_result, number);
}

TEST_F(AssignerTest, conversions_load_store)
{
    uint32_t number = std::numeric_limits<uint32_t>::max();
    // initialize from byte array
    auto tmp = nil::evm_assigner::zkevm_word<BlueprintFieldType>(
        reinterpret_cast<uint8_t*>(&number), sizeof(number));

    // compare string representations of the data
    std::ostringstream ss;
    ss << std::hex << number;
    ASSERT_EQ(ss.str(), intx::to_string(tmp.get_value(), 16));
    // check store result
    uint32_t restored_number = 0;
    tmp.store<uint32_t>(reinterpret_cast<uint8_t *>(&restored_number));
    // check if same
    EXPECT_EQ(restored_number, number);
}

TEST_F(AssignerTest, load_partial_zkevm_word)
{
    uint64_t number = std::numeric_limits<uint64_t>::max();
    std::array<uint8_t, 26> bytes;
    for (unsigned i = 0; i < bytes.size(); ++i) {
        bytes[i] = static_cast<uint8_t>(bytes.size() + i);
    }

    nil::evm_assigner::zkevm_word<BlueprintFieldType> tmp;
    constexpr size_t word_size = 8;
    constexpr auto num_full_words = bytes.size() / word_size;
    constexpr auto num_partial_bytes = bytes.size() % word_size;
    auto data = bytes.data();

    // Load top partial word.
    if constexpr (num_partial_bytes != 0)
    {
        tmp.load_partial_data(data, num_partial_bytes, num_full_words);
        data += num_partial_bytes;
    }

    // Load full words.
    for (size_t i = 0; i < num_full_words; ++i)
    {
        tmp.load_partial_data(data, word_size, num_full_words - 1 - i);
        data += word_size;
    }
    ASSERT_EQ(bytes_to_string(bytes.data(), bytes.size()), intx::to_string(tmp.get_value(), 16));
}

TEST_F(AssignerTest, field_bitwise_and) {
    using intx::operator""_u256;
    auto bits_set_zkevm_word = [](std::initializer_list<unsigned> bits) {
        nil::evm_assigner::zkevm_word<BlueprintFieldType> tmp(0);
        for (unsigned bit_number : bits)
        {
            tmp = tmp + nil::evm_assigner::zkevm_word<BlueprintFieldType>(1_u256 << bit_number);
        }
        return tmp;
    };
    auto bits_set_integral = [](std::initializer_list<unsigned> bits) {
        BlueprintFieldType::integral_type tmp = 0;
        for (unsigned bit_number : bits)
        {
            boost::multiprecision::bit_set(tmp, bit_number);
        }
        return tmp;
    };
    auto bits_test_integral = [&](BlueprintFieldType::integral_type val,
                                  std::unordered_set<unsigned> bits) {
        for (unsigned bit_number = 0; bit_number < BlueprintFieldType::number_bits; ++bit_number)
        {
            EXPECT_EQ(boost::multiprecision::bit_test(val, bit_number), bits.contains(bit_number));
        }
    };
    std::initializer_list<unsigned> init_bits = {1, 12, 42, 77, 136, 201, 222};
    nil::evm_assigner::zkevm_word<BlueprintFieldType> init = bits_set_zkevm_word(init_bits);

    std::initializer_list<unsigned> conjunction_bits = {4, 12, 77, 165, 222};
    BlueprintFieldType::integral_type op = bits_set_integral(conjunction_bits);

    BlueprintFieldType::integral_type res = init & op;

    std::unordered_set<unsigned> res_bit_set;
    std::set_intersection(init_bits.begin(), init_bits.end(), conjunction_bits.begin(),
        conjunction_bits.end(), std::inserter(res_bit_set, res_bit_set.begin()));
    bits_test_integral(res, res_bit_set);
}

TEST_F(AssignerTest, w_hi_lo)
{
    using intx::operator""_u256;
    intx::uint256 hi = 0x12345678901234567890_u256;
    intx::uint256 lo = 0x98765432109876543210_u256;
    nil::evm_assigner::zkevm_word<BlueprintFieldType> tmp((hi << 128) + lo);
    std::ostringstream original_numbers;
    original_numbers << intx::to_string(hi, 16) << "|" << intx::to_string(lo, 16);
    std::ostringstream result_numbers;
    result_numbers << std::hex << tmp.w_hi() << "|" << tmp.w_lo();
    EXPECT_EQ(original_numbers.str(), result_numbers.str());
}

TEST_F(AssignerTest, set_val)
{
    using intx::operator""_u256;
    nil::evm_assigner::zkevm_word<BlueprintFieldType> tmp;
    tmp.set_val(0x1, 0);
    tmp.set_val(0x12, 1);
    tmp.set_val(0x123, 2);
    tmp.set_val(0x1234, 3);
    intx::uint256 expected_val =
        (0x1234_u256 << 64 * 3) + (0x123_u256 << 64 * 2) + (0x12_u256 << 64) + 0x1;
    ASSERT_EQ(tmp.get_value(), expected_val);
}

template<typename BlueprintFieldType>
inline void test_stack_rw_circuit_1(
    void (*instr_fn)(evmone::StackTop<BlueprintFieldType>, evmone::ExecutionState<BlueprintFieldType>&) noexcept,
    uint8_t opcode,
    typename nil::evm_assigner::zkevm_word<BlueprintFieldType> inp,
    typename nil::evm_assigner::zkevm_word<BlueprintFieldType> expected,
    AssignerTest* testInstance
) {

    TestRWCircuitStruct<BlueprintFieldType> wrapper(opcode, testInstance);
    *(++wrapper.position.stack_top) = inp;
    instr_fn(wrapper.position.stack_top, wrapper.state);
    testInstance->assigner_ptr->handle_rw(wrapper.state.rw_trace);

    stack_rw_circuit_check(testInstance->assignments, 0/*row*/, 0/*address in stack*/, 0/*trace size*/, false/*is_write*/, inp.w_hi()    /*value_hi*/, inp.w_lo()    /*value_lo*/);
    stack_rw_circuit_check(testInstance->assignments, 1/*row*/, 0/*address in stack*/, 1/*trace size*/, true /*is_write*/, expected.w_hi()/*value_hi*/, expected.w_lo()/*value_lo*/);
}

template<typename BlueprintFieldType>
inline void test_stack_rw_circuit_2(
    void (*instr_fn)(evmone::StackTop<BlueprintFieldType>, evmone::ExecutionState<BlueprintFieldType>&) noexcept,
    uint8_t opcode,
    typename nil::evm_assigner::zkevm_word<BlueprintFieldType> inp1,
    typename nil::evm_assigner::zkevm_word<BlueprintFieldType> inp2,
    typename nil::evm_assigner::zkevm_word<BlueprintFieldType> expected,
    AssignerTest* testInstance
) {

    TestRWCircuitStruct<BlueprintFieldType> wrapper(opcode, testInstance);
    *(++wrapper.position.stack_top) = inp1;
    *(++wrapper.position.stack_top) = inp2;
    instr_fn(wrapper.position.stack_top, wrapper.state);
    testInstance->assigner_ptr->handle_rw(wrapper.state.rw_trace);

    stack_rw_circuit_check(testInstance->assignments, 0/*row*/, 0/*address in stack*/, 0/*trace size*/, false/*is_write*/, inp1.w_hi()    /*value_hi*/, inp1.w_lo()    /*value_lo*/);
    stack_rw_circuit_check(testInstance->assignments, 1/*row*/, 0/*address in stack*/, 2/*trace size*/, true /*is_write*/, expected.w_hi()/*value_hi*/, expected.w_lo()/*value_lo*/);
    stack_rw_circuit_check(testInstance->assignments, 2/*row*/, 1/*address in stack*/, 1/*trace size*/, false/*is_write*/, inp2.w_hi()    /*value_hi*/, inp2.w_lo()    /*value_lo*/);
}

template<typename BlueprintFieldType>
inline void test_stack_rw_circuit_exp(
    evmone::Result (*instr_fn)(evmone::StackTop<BlueprintFieldType>, int64_t, evmone::ExecutionState<BlueprintFieldType>&) noexcept,
    uint8_t opcode,
    typename nil::evm_assigner::zkevm_word<BlueprintFieldType> inp1,
    typename nil::evm_assigner::zkevm_word<BlueprintFieldType> inp2,
    typename nil::evm_assigner::zkevm_word<BlueprintFieldType> expected,
    AssignerTest* testInstance
) {

    TestRWCircuitStruct<BlueprintFieldType> wrapper(opcode, testInstance);
    *(++wrapper.position.stack_top) = inp1;
    *(++wrapper.position.stack_top) = inp2;
    instr_fn(wrapper.position.stack_top, testInstance->msg.gas, wrapper.state);
    testInstance->assigner_ptr->handle_rw(wrapper.state.rw_trace);

    stack_rw_circuit_check(testInstance->assignments, 0/*row*/, 0/*address in stack*/, 0/*trace size*/, false/*is_write*/, inp1.w_hi()    /*value_hi*/, inp1.w_lo()    /*value_lo*/);
    stack_rw_circuit_check(testInstance->assignments, 1/*row*/, 0/*address in stack*/, 2/*trace size*/, true /*is_write*/, expected.w_hi()/*value_hi*/, expected.w_lo()/*value_lo*/);
    stack_rw_circuit_check(testInstance->assignments, 2/*row*/, 1/*address in stack*/, 1/*trace size*/, false/*is_write*/, inp2.w_hi()    /*value_hi*/, inp2.w_lo()    /*value_lo*/);
}

template<typename BlueprintFieldType>
inline void test_stack_rw_circuit_keccak(
    evmone::Result (*instr_fn)(evmone::StackTop<BlueprintFieldType>, int64_t, evmone::ExecutionState<BlueprintFieldType>&) noexcept,
    uint8_t opcode,
    typename nil::evm_assigner::zkevm_word<BlueprintFieldType> inp1,
    typename nil::evm_assigner::zkevm_word<BlueprintFieldType> inp2,
    AssignerTest* testInstance
) {
    std::vector<uint8_t> code = {opcode};

    const evmone::bytes_view container{code.data(), code.size()};
    const auto code_analysis = evmone::baseline::analyze(testInstance->rev, container);
    const auto data = code_analysis.eof_header.get_data(container);
    evmone::ExecutionState<BlueprintFieldType> state(
        testInstance->msg,
        testInstance->rev,
        *(testInstance->host_interface),
        testInstance->ctx,
        container,
        data,
        0,
        testInstance->assigner_ptr);
    evmone::baseline::Position<BlueprintFieldType> position{code.data(), state.stack_space.bottom()};

    position.stack_top++;
    *position.stack_top = inp1;
    position.stack_top++;
    *position.stack_top = inp2;

    const auto i = inp2.to_uint64();
    const auto s = inp1.to_uint64();

    assert(s != 0);

    uint8_t keccak_input[32];
    for (std::size_t j = 0; j < 32; j++) { keccak_input[j] = 255; }

    (&state.memory)->grow(32);

    uint8_t* mem_data = &state.memory[i];
    for (std::size_t j = 0; j < 32; j++) { mem_data[j] = 255; }

    nil::evm_assigner::zkevm_word<BlueprintFieldType> expected = nil::evm_assigner::zkevm_word<BlueprintFieldType>(ethash::keccak256(keccak_input, s));

    instr_fn(position.stack_top, testInstance->msg.gas, state);

    testInstance->assigner_ptr->handle_rw(state.rw_trace);

    uint32_t row = 0;
    stack_rw_circuit_check(testInstance->assignments, row++/*row*/, 0/*address in stack*/, 0/*trace size*/, false/*is_write*/, inp1.w_hi()    /*value_hi*/, inp1.w_lo()    /*value_lo*/);
    stack_rw_circuit_check(testInstance->assignments, row++/*row*/, 0/*address in stack*/, s == 0 ? 2 : 2+32/*trace size*/, true /*is_write*/, expected.w_hi()/*value_hi*/, expected.w_lo()/*value_lo*/);
    stack_rw_circuit_check(testInstance->assignments, row++/*row*/, 1/*address in stack*/, 1/*trace size*/, false/*is_write*/, inp2.w_hi()    /*value_hi*/, inp2.w_lo()    /*value_lo*/);
    if (s != 0 ) {
        for(uint32_t j = 0; j < 32; j++){
            memory_rw_circuit_check(testInstance->assignments, row++, (inp2 + j).w_lo(), 2 + j, false, 0, mem_data[j]);
        }
    }
    // TODO: what if s == 0
}


template<typename BlueprintFieldType>
inline void test_stack_rw_circuit_3(
    void (*instr_fn)(evmone::StackTop<BlueprintFieldType>, evmone::ExecutionState<BlueprintFieldType>&) noexcept,
    uint8_t opcode,
    typename nil::evm_assigner::zkevm_word<BlueprintFieldType> inp1,
    typename nil::evm_assigner::zkevm_word<BlueprintFieldType> inp2,
    typename nil::evm_assigner::zkevm_word<BlueprintFieldType> inp3,
    typename nil::evm_assigner::zkevm_word<BlueprintFieldType> expected,
    AssignerTest* testInstance
) {

    TestRWCircuitStruct<BlueprintFieldType> wrapper(opcode, testInstance);
    *(++wrapper.position.stack_top) = inp1;
    *(++wrapper.position.stack_top) = inp2;
    *(++wrapper.position.stack_top) = inp3;
    instr_fn(wrapper.position.stack_top, wrapper.state);
    testInstance->assigner_ptr->handle_rw(wrapper.state.rw_trace);

    stack_rw_circuit_check(testInstance->assignments, 0/*row*/, 0/*address in stack*/, 0/*trace size*/, false/*is_write*/, inp1.w_hi()    /*value_hi*/, inp1.w_lo()    /*value_lo*/);
    stack_rw_circuit_check(testInstance->assignments, 1/*row*/, 0/*address in stack*/, 3/*trace size*/, true /*is_write*/, expected.w_hi()/*value_hi*/, expected.w_lo()/*value_lo*/);
    stack_rw_circuit_check(testInstance->assignments, 2/*row*/, 1/*address in stack*/, 1/*trace size*/, false/*is_write*/, inp2.w_hi()    /*value_hi*/, inp2.w_lo()    /*value_lo*/);
    stack_rw_circuit_check(testInstance->assignments, 3/*row*/, 2/*address in stack*/, 2/*trace size*/, false/*is_write*/, inp3.w_hi()    /*value_hi*/, inp3.w_lo()    /*value_lo*/);

}

template<typename BlueprintFieldType>
typename nil::evm_assigner::zkevm_word<BlueprintFieldType> expected_result_1 (
    uint8_t opcode,
    typename nil::evm_assigner::zkevm_word<BlueprintFieldType> inp
) {
    switch(opcode) {
        case evmone::OP_NOT:    return ~inp;
        case evmone::OP_ISZERO: return inp == 0;
        default: std::abort();
    }
}

template<typename BlueprintFieldType>
typename nil::evm_assigner::zkevm_word<BlueprintFieldType> expected_result_2 (
    uint8_t opcode,
    typename nil::evm_assigner::zkevm_word<BlueprintFieldType> inp1,
    typename nil::evm_assigner::zkevm_word<BlueprintFieldType> inp2
) {
    switch(opcode) {
        case evmone::OP_ADD:  return inp2 + inp1;
        case evmone::OP_MUL:  return inp2 * inp1;
        case evmone::OP_SUB:  return inp2 - inp1;
        case evmone::OP_DIV:  return inp1 != 0 ? inp2 / inp1 : 0;
        case evmone::OP_SDIV: return inp2.sdiv(inp1);
        case evmone::OP_MOD:  return inp1 != 0 ? inp2 % inp1 : 0;
        case evmone::OP_SMOD: return inp2.smod(inp1);
        case evmone::OP_SIGNEXTEND: {

            nil::evm_assigner::zkevm_word<BlueprintFieldType> res(inp1);
            if (inp2<31)
            {
                const auto e = inp2.to_uint64();  // uint256 -> uint64.
                const auto sign_word_index =
                    static_cast<size_t>(e / sizeof(e));      // Index of the word with the sign bit.
                const auto sign_byte_index = e % sizeof(e);  // Index of the sign byte in the sign word.
                auto sign_word = inp1.to_uint64(sign_word_index);

                const auto sign_byte_offset = sign_byte_index * 8;
                const auto sign_byte = sign_word >> sign_byte_offset;  // Move sign byte to position 0.

                // Sign-extend the "sign" byte and move it to the right position. Value bits are zeros.
                const auto sext_byte = static_cast<uint64_t>(int64_t{static_cast<int8_t>(sign_byte)});
                const auto sext = sext_byte << sign_byte_offset;

                const auto sign_mask = ~uint64_t{0} << sign_byte_offset;
                const auto value = sign_word & ~sign_mask;  // Reset extended bytes.
                sign_word = sext | value;                   // Combine the result word.

                // Produce bits (all zeros or ones) for extended words. This is done by SAR of
                // the sign-extended byte. Shift by any value 7-63 would work.
                const auto sign_ex = static_cast<uint64_t>(static_cast<int64_t>(sext_byte) >> 8);

                for (size_t i = 3; i > sign_word_index; --i)
                    res.set_val(sign_ex, i);  // Clear extended words.
            }
            return res;
        }
        case evmone::OP_LT:  return inp2 < inp1;
        case evmone::OP_GT:  return inp1 < inp2;
        case evmone::OP_SLT: return inp2.slt(inp1);
        case evmone::OP_SGT: return inp1.slt(inp2);
        case evmone::OP_EQ:  return inp1 == inp2;
        case evmone::OP_AND: return inp1 & inp2;
        case evmone::OP_OR:  return inp1 | inp2;
        case evmone::OP_XOR: return inp1 ^ inp2;
        case evmone::OP_BYTE: {
            const bool n_valid = inp2 < 32;
            const uint64_t byte_mask = (n_valid ? 0xff : 0);
            const auto index = 31 - static_cast<unsigned>(inp2.to_uint64() % 32);
            const auto word = inp1.to_uint64(index / 8);
            const auto byte_index = index % 8;
            const auto byte = (word >> (byte_index * 8)) & byte_mask;
            return nil::evm_assigner::zkevm_word<BlueprintFieldType>(byte);
        }
        case evmone::OP_SHL: return inp1 << inp2;
        case evmone::OP_SHR: return inp1 >> inp2;
        case evmone::OP_SAR: {
            const bool is_neg = inp1.to_uint64(3) < 0;  // Inspect the top bit (words are LE).
            const nil::evm_assigner::zkevm_word<BlueprintFieldType> sign_mask = is_neg ? 1 : 0;
            const auto mask_shift = (inp2< 256) ? (256 - inp2.to_uint64(0)) : 0;
            return (inp1 >> inp2) | (sign_mask << mask_shift);
        }
        default: std::abort();
    }
}

template<typename BlueprintFieldType>
typename nil::evm_assigner::zkevm_word<BlueprintFieldType> expected_result_3 (
    uint8_t opcode,
    typename nil::evm_assigner::zkevm_word<BlueprintFieldType> inp1,
    typename nil::evm_assigner::zkevm_word<BlueprintFieldType> inp2,
    typename nil::evm_assigner::zkevm_word<BlueprintFieldType> inp3
) {
    switch(opcode) {
        case evmone::OP_ADDMOD: return inp3.addmod(inp2, inp1);
        case evmone::OP_MULMOD: return inp3.mulmod(inp2, inp1);
        default: std::abort();
    }
}


#define TEST_STACK_1_INPUTS(INSTR, OP_CODE, INP) \
TEST_F(AssignerTest, INSTR ## _ ## INP) { \
    test_stack_rw_circuit_1<BlueprintFieldType>( \
        evmone::instr::core::instructions<BlueprintFieldType>::INSTR, \
        evmone::OP_CODE, \
        typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(INP), \
        expected_result_1<BlueprintFieldType>(evmone::OP_CODE, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(INP)), \
        this); \
}

#define TEST_STACK_2_INPUTS(INSTR, OP_CODE, INP1, INP2) \
TEST_F(AssignerTest, INSTR ## _ ## INP1 ## _ ## INP2) { \
    test_stack_rw_circuit_2<BlueprintFieldType>( \
        evmone::instr::core::instructions<BlueprintFieldType>::INSTR, \
        evmone::OP_CODE, \
        typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(INP1), \
        typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(INP2), \
        expected_result_2<BlueprintFieldType>(evmone::OP_CODE, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(INP1), typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(INP2)), \
        this); \
}

#define TEST_STACK_3_INPUTS(INSTR, OP_CODE, INP1, INP2, INP3) \
TEST_F(AssignerTest, INSTR ## _ ## INP1 ## _ ## INP2 ## _ ## INP3) { \
    test_stack_rw_circuit_3<BlueprintFieldType>( \
        evmone::instr::core::instructions<BlueprintFieldType>::INSTR, \
        evmone::OP_CODE, \
        typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(INP1), \
        typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(INP2), \
        typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(INP3), \
        expected_result_3<BlueprintFieldType>(evmone::OP_CODE, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(INP1), typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(INP2), typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(INP3)), \
        this); \
}

#define TEST_STACK_1_INPUTS_ALL_CASES(INSTR, OP_CODE) \
    TEST_STACK_1_INPUTS(INSTR, OP_CODE, 0) \
    TEST_STACK_1_INPUTS(INSTR, OP_CODE, 1) \
    TEST_STACK_1_INPUTS(INSTR, OP_CODE, 0xffffffffffffffff)

#define TEST_STACK_2_INPUTS_ALL_CASES(INSTR, OP_CODE) \
    TEST_STACK_2_INPUTS(INSTR, OP_CODE, 0, 0) \
    TEST_STACK_2_INPUTS(INSTR, OP_CODE, 0, 1) \
    TEST_STACK_2_INPUTS(INSTR, OP_CODE, 1, 0) \
    TEST_STACK_2_INPUTS(INSTR, OP_CODE, 1, 1) \
    TEST_STACK_2_INPUTS(INSTR, OP_CODE, 0xffffffffffffffff, 3)


    // OP_STOP // no trace, non-void
TEST_STACK_2_INPUTS_ALL_CASES(add, OP_ADD)
TEST_STACK_2_INPUTS_ALL_CASES(mul, OP_MUL)
TEST_STACK_2_INPUTS_ALL_CASES(sub, OP_SUB)
TEST_STACK_2_INPUTS_ALL_CASES(div, OP_DIV)
TEST_STACK_2_INPUTS_ALL_CASES(sdiv, OP_SDIV)
TEST_STACK_2_INPUTS_ALL_CASES(mod, OP_MOD)
TEST_STACK_2_INPUTS_ALL_CASES(smod, OP_SMOD)
TEST_STACK_3_INPUTS(addmod, OP_ADDMOD, 11, 22, 5)
TEST_STACK_3_INPUTS(mulmod, OP_MULMOD, 11, 3, 7)

TEST_F(AssignerTest, test_exp) { \
    test_stack_rw_circuit_exp<BlueprintFieldType>( \
        evmone::instr::core::instructions<BlueprintFieldType>::exp,
        evmone::OP_EXP,
        typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(1),
        typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(1),
        typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(1).exp(typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(1)),
        this);
}

TEST_STACK_2_INPUTS_ALL_CASES(signextend, OP_SIGNEXTEND)

TEST_STACK_2_INPUTS_ALL_CASES(lt, OP_LT)
TEST_STACK_2_INPUTS_ALL_CASES(gt, OP_GT)
TEST_STACK_2_INPUTS_ALL_CASES(slt, OP_SLT)
TEST_STACK_2_INPUTS_ALL_CASES(sgt, OP_SGT)
TEST_STACK_2_INPUTS_ALL_CASES(eq, OP_EQ)
TEST_STACK_1_INPUTS_ALL_CASES(iszero, OP_ISZERO)
TEST_STACK_2_INPUTS_ALL_CASES(and_, OP_AND)
TEST_STACK_2_INPUTS_ALL_CASES(or_, OP_OR)
TEST_STACK_2_INPUTS_ALL_CASES(xor_, OP_XOR)
TEST_STACK_1_INPUTS_ALL_CASES(not_, OP_NOT)
TEST_STACK_2_INPUTS_ALL_CASES(byte, OP_BYTE)
TEST_STACK_2_INPUTS_ALL_CASES(shl, OP_SHL)
TEST_STACK_2_INPUTS_ALL_CASES(shr, OP_SHR)
TEST_STACK_2_INPUTS_ALL_CASES(sar, OP_SAR)

TEST_F(AssignerTest, test_keccak) { \
    test_stack_rw_circuit_keccak<BlueprintFieldType>( \
        evmone::instr::core::instructions<BlueprintFieldType>::keccak256,
        evmone::OP_KECCAK256,
        typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(32),
        typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(0),
        this);
}

TEST_F(AssignerTest, test_op_address) {
    TestRWCircuitStruct<BlueprintFieldType> wrapper(evmone::OP_ADDRESS, this);
    evmone::instr::core::instructions<BlueprintFieldType>::address(wrapper.position.stack_top, wrapper.state);
    this->assigner_ptr->handle_rw(wrapper.state.rw_trace);

    stack_rw_circuit_check(this->assignments, 0/*row*/, 0/*address in stack*/, 0/*trace size*/, true /*is_write*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(this->msg.recipient).w_hi()/*value_hi*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(this->msg.recipient).w_lo()/*value_lo*/);
}

TEST_F(AssignerTest, test_op_balance) {

    TestRWCircuitStruct<BlueprintFieldType> wrapper(evmone::OP_BALANCE, this);

    nil::evm_assigner::zkevm_word<BlueprintFieldType> x = 0;
    const auto addr = x.to_address();
    nil::evm_assigner::zkevm_word<BlueprintFieldType> result = nil::evm_assigner::zkevm_word<BlueprintFieldType>(wrapper.state.host.get_balance(addr));

    *(++wrapper.position.stack_top) = result;

    evmone::instr::core::instructions<BlueprintFieldType>::balance(wrapper.position.stack_top, msg.gas, wrapper.state);
    assigner_ptr->handle_rw(wrapper.state.rw_trace);

    stack_rw_circuit_check(assignments, 0/*row*/, 0/*address in stack*/, 0/*trace size*/, false/*is_write*/, result.w_hi()/*value_hi*/, result.w_lo()/*value_lo*/);
    stack_rw_circuit_check(assignments, 1/*row*/, 0/*address in stack*/, 1/*trace size*/, true /*is_write*/, result.w_hi()/*value_hi*/, result.w_lo()/*value_lo*/);

}

TEST_F(AssignerTest, test_op_origin) {
    TestRWCircuitStruct<BlueprintFieldType> wrapper(evmone::OP_ORIGIN, this);
    evmone::instr::core::instructions<BlueprintFieldType>::origin(wrapper.position.stack_top, wrapper.state);
    this->assigner_ptr->handle_rw(wrapper.state.rw_trace);

    stack_rw_circuit_check(this->assignments, 0/*row*/, 0/*address in stack*/, 0/*trace size*/, true /*is_write*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(wrapper.state.get_tx_context().tx_origin).w_hi()/*value_hi*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(wrapper.state.get_tx_context().tx_origin).w_lo()/*value_lo*/);
}


TEST_F(AssignerTest, test_op_caller) {
    TestRWCircuitStruct<BlueprintFieldType> wrapper(evmone::OP_CALLER, this);
    evmone::instr::core::instructions<BlueprintFieldType>::caller(wrapper.position.stack_top, wrapper.state);
    this->assigner_ptr->handle_rw(wrapper.state.rw_trace);

    stack_rw_circuit_check(this->assignments, 0/*row*/, 0/*address in stack*/, 0/*trace size*/, true /*is_write*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(wrapper.state.msg->sender).w_hi()/*value_hi*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(wrapper.state.msg->sender).w_lo()/*value_lo*/);
}


TEST_F(AssignerTest, test_op_callvalue) {
    TestRWCircuitStruct<BlueprintFieldType> wrapper(evmone::OP_CALLVALUE, this);
    evmone::instr::core::instructions<BlueprintFieldType>::callvalue(wrapper.position.stack_top, wrapper.state);
    this->assigner_ptr->handle_rw(wrapper.state.rw_trace);

    stack_rw_circuit_check(this->assignments, 0/*row*/, 0/*address in stack*/, 0/*trace size*/, true /*is_write*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(wrapper.state.msg->value).w_hi()/*value_hi*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(wrapper.state.msg->value).w_lo()/*value_lo*/);
}

template<typename BlueprintFieldType>
void test_calldataload (
    TestRWCircuitStruct<BlueprintFieldType>& wrapper,
    typename nil::evm_assigner::zkevm_word<BlueprintFieldType> index,
    AssignerTest* testInstance
) {
    *(++wrapper.position.stack_top) = index;
        const auto index_uint64 = index.to_uint64();
        typename nil::evm_assigner::zkevm_word<BlueprintFieldType> result;
        if (wrapper.state.msg->input_size < index_uint64)
            typename nil::evm_assigner::zkevm_word<BlueprintFieldType> result = 0;
        else
        {
            const auto begin = index_uint64;
            const auto end = std::min(begin + 32, wrapper.state.msg->input_size);
            uint8_t data[32] = {};
            for (size_t i = 0; i < (end - begin); ++i)
                data[i] = wrapper.state.msg->input_data[begin + i];

            result = typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(data, 32);
        }

    evmone::instr::core::instructions<BlueprintFieldType>::calldataload(wrapper.position.stack_top, wrapper.state);
    testInstance->assigner_ptr->handle_rw(wrapper.state.rw_trace);

    stack_rw_circuit_check(testInstance->assignments, 0/*row*/, 0/*address in stack*/, 0/*trace size*/, false/*is_write*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(index).w_hi()/*value_hi*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(index).w_lo()/*value_lo*/);
    stack_rw_circuit_check(testInstance->assignments, 1/*row*/, 0/*address in stack*/, 1/*trace size*/, true /*is_write*/, result.w_hi()/*value_hi*/, result.w_lo()/*value_lo*/);
}

TEST_F(AssignerTest, test_op_calldataload_0) {
    TestRWCircuitStruct<BlueprintFieldType> wrapper(evmone::OP_CALLDATALOAD, this);
    test_calldataload<BlueprintFieldType>(wrapper, 0, this);
}
TEST_F(AssignerTest, test_op_calldataload_1) {
    TestRWCircuitStruct<BlueprintFieldType> wrapper(evmone::OP_CALLDATALOAD, this);
    typename nil::evm_assigner::zkevm_word<BlueprintFieldType> input =
        typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(wrapper.state.msg->input_size) + 1;
    test_calldataload<BlueprintFieldType>(wrapper, input, this);
}
TEST_F(AssignerTest, test_op_calldataload_2) {
    TestRWCircuitStruct<BlueprintFieldType> wrapper(evmone::OP_CALLDATALOAD, this);
    typename nil::evm_assigner::zkevm_word<BlueprintFieldType> input =
        typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(wrapper.state.msg->input_size) / 2;
    test_calldataload<BlueprintFieldType>(wrapper, input, this);
}

TEST_F(AssignerTest, test_op_calldatasize) {
    TestRWCircuitStruct<BlueprintFieldType> wrapper(evmone::OP_CALLDATASIZE, this);
    evmone::instr::core::instructions<BlueprintFieldType>::calldatasize(wrapper.position.stack_top, wrapper.state);
    this->assigner_ptr->handle_rw(wrapper.state.rw_trace);

    stack_rw_circuit_check(this->assignments, 0/*row*/, 0/*address in stack*/, 0/*trace size*/, true /*is_write*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(wrapper.state.msg->input_size).w_hi()/*value_hi*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(wrapper.state.msg->input_size).w_lo()/*value_lo*/);
}

template<typename BlueprintFieldType>
void test_calldatacopy (
    TestRWCircuitStruct<BlueprintFieldType>& wrapper,
    AssignerTest* testInstance
) {
    typename nil::evm_assigner::zkevm_word<BlueprintFieldType> inp1 = 13;
    typename nil::evm_assigner::zkevm_word<BlueprintFieldType> inp2 = 0;
    typename nil::evm_assigner::zkevm_word<BlueprintFieldType> inp3 = 0;

    *(++wrapper.position.stack_top) = inp1;
    *(++wrapper.position.stack_top) = inp2;
    *(++wrapper.position.stack_top) = inp3;
        const auto& mem_index = inp3;
        const auto& input_index = inp2;
        const auto& size = inp1;

        (&wrapper.state.memory)->grow(32);
        for (std::size_t i = 0; i < 32; i++) {
            wrapper.state.memory[i] = 255;
        }

    evmone::instr::core::instructions<BlueprintFieldType>::calldatacopy(wrapper.position.stack_top, wrapper.state.msg->gas, wrapper.state);
    testInstance->assigner_ptr->handle_rw(wrapper.state.rw_trace);

    stack_rw_circuit_check(testInstance->assignments, 0/*row*/, 0/*address in stack*/, 0/*trace size*/, false/*is_write*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(inp1).w_hi()/*value_hi*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(inp1).w_lo()/*value_lo*/);
    stack_rw_circuit_check(testInstance->assignments, 1/*row*/, 1/*address in stack*/, 1/*trace size*/, false/*is_write*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(inp2).w_hi()/*value_hi*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(inp2).w_lo()/*value_lo*/);
    stack_rw_circuit_check(testInstance->assignments, 2/*row*/, 2/*address in stack*/, 2/*trace size*/, false/*is_write*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(inp3).w_hi()/*value_hi*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(inp3).w_lo()/*value_lo*/);

    memory_rw_circuit_check(testInstance->assignments, 3,  0, 3,  true, 0, 'H');
    memory_rw_circuit_check(testInstance->assignments, 4,  1, 4,  true, 0, 'e');
    memory_rw_circuit_check(testInstance->assignments, 5,  2, 5,  true, 0, 'l');
    memory_rw_circuit_check(testInstance->assignments, 6,  3, 6,  true, 0, 'l');
    memory_rw_circuit_check(testInstance->assignments, 7,  4, 7,  true, 0, 'o');
    memory_rw_circuit_check(testInstance->assignments, 8,  5, 8,  true, 0, ' ');
    memory_rw_circuit_check(testInstance->assignments, 9,  6, 9,  true, 0, 'W');
    memory_rw_circuit_check(testInstance->assignments, 10, 7, 10, true, 0, 'o');
    memory_rw_circuit_check(testInstance->assignments, 11, 8, 11, true, 0, 'r');
    memory_rw_circuit_check(testInstance->assignments, 12, 9, 12, true, 0, 'l');
    memory_rw_circuit_check(testInstance->assignments, 13, 10, 13, true, 0, 'd');
    memory_rw_circuit_check(testInstance->assignments, 14, 11, 14, true, 0, '!');
}

TEST_F(AssignerTest, test_op_calldatacopy_1) {
    TestRWCircuitStruct<BlueprintFieldType> wrapper(evmone::OP_CALLDATACOPY, this);
    test_calldatacopy<BlueprintFieldType>(wrapper, this);
}

TEST_F(AssignerTest, test_op_codesize) {
    TestRWCircuitStruct<BlueprintFieldType> wrapper(evmone::OP_CODESIZE, this);
    evmone::instr::core::instructions<BlueprintFieldType>::codesize(wrapper.position.stack_top, wrapper.state);
    this->assigner_ptr->handle_rw(wrapper.state.rw_trace);

    stack_rw_circuit_check(this->assignments, 0/*row*/, 0/*address in stack*/, 0/*trace size*/, true /*is_write*/, 0/*value_hi*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(wrapper.state.original_code.size()).w_lo()/*value_lo*/);
}


    // OP_CODECOPY // contains TODOs, skipping for now

TEST_F(AssignerTest, test_op_gasprice) {
    TestRWCircuitStruct<BlueprintFieldType> wrapper(evmone::OP_GASPRICE, this);
    evmone::instr::core::instructions<BlueprintFieldType>::gasprice(wrapper.position.stack_top, wrapper.state);
    this->assigner_ptr->handle_rw(wrapper.state.rw_trace);

    stack_rw_circuit_check(this->assignments, 0/*row*/, 0/*address in stack*/, 0/*trace size*/, true /*is_write*/, 0/*value_hi*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(wrapper.state.get_tx_context().tx_gas_price).w_lo()/*value_lo*/);
}

    // OP_EXTCODESIZE // which address should be used?
    // OP_EXTCODECOPY // contains TODOs

TEST_F(AssignerTest, test_op_returndatasize) {
    TestRWCircuitStruct<BlueprintFieldType> wrapper(evmone::OP_RETURNDATASIZE, this);
    evmone::instr::core::instructions<BlueprintFieldType>::returndatasize(wrapper.position.stack_top, wrapper.state);
    this->assigner_ptr->handle_rw(wrapper.state.rw_trace);

    stack_rw_circuit_check(this->assignments, 0/*row*/, 0/*address in stack*/, 0/*trace size*/, true /*is_write*/, 0/*value_hi*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(wrapper.state.return_data.size()).w_lo()/*value_lo*/);
}

    // OP_RETURNDATACOPY = how returndata is filled and what it should contain? skipped for now
    // OP_EXTCODEHASH // get_code_hash(addr), which address shodld use?

    // OP_BLOCKHASH // gettxcontext, filled with which data?
    // OP_COINBASE // gettxcontext
    // OP_TIMESTAMP // gettxcontext
    // OP_NUMBER // gettxcontext
    // OP_PREVRANDAO // gettxcontext
    // OP_GASLIMIT // gettxcontext
    // OP_CHAINID // gettxcontext
    // OP_SELFBALANCE // contains TODO
    // OP_BASEFEE // gettxcontext
    // OP_BLOBHASH // gettxcontext
    // OP_BLOBBASEFEE // gettxcontext

    // OP_POP // no trace

template<typename BlueprintFieldType>
void test_mload (
    TestRWCircuitStruct<BlueprintFieldType>& wrapper,
    typename nil::evm_assigner::zkevm_word<BlueprintFieldType> index,
    AssignerTest* testInstance
) {

    *(++wrapper.position.stack_top) = index;
    const auto addr = index.to_uint64();

    uint8_t expected_mem[64];
    for (std::uint8_t i = 0; i < 64; i++) {
        expected_mem[i] = i;
    }

    (&wrapper.state.memory)->grow(64);
    for (std::uint8_t i = 0; i < 64; i++) {
        wrapper.state.memory[i] = i;
    }

    const auto& wordsize = nil::evm_assigner::zkevm_word<BlueprintFieldType>::size;

    nil::evm_assigner::zkevm_word<BlueprintFieldType> result =
        nil::evm_assigner::zkevm_word<BlueprintFieldType>(&wrapper.state.memory[addr], nil::evm_assigner::zkevm_word<BlueprintFieldType>::size);

    evmone::instr::core::instructions<BlueprintFieldType>::template mload<intx::uint256>(wrapper.position.stack_top, wrapper.state.msg->gas, wrapper.state);
    testInstance->assigner_ptr->handle_rw(wrapper.state.rw_trace);

    std::uint32_t row = 0;
    stack_rw_circuit_check(testInstance->assignments, row++ /*row*/, 0/*address in stack*/, 0           /*trace size*/, false/*is_write*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(index).w_hi() /*value_hi*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(index).w_lo() /*value_lo*/);
    stack_rw_circuit_check(testInstance->assignments, row++ /*row*/, 0/*address in stack*/, 1 + wordsize/*trace size*/, true /*is_write*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(result).w_hi()/*value_hi*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(result).w_lo()/*value_lo*/);

    for (std::uint32_t i = 0; i < wordsize; i++) {
        memory_rw_circuit_check(testInstance->assignments, row++ , addr + i, 1 + i,  false, 0, expected_mem[addr + i]);
    }

}

TEST_F(AssignerTest, test_op_mload_0) {
    TestRWCircuitStruct<BlueprintFieldType> wrapper(evmone::OP_MLOAD, this);
    test_mload<BlueprintFieldType>(wrapper, 0, this);
}

TEST_F(AssignerTest, test_op_mload_32) {
    TestRWCircuitStruct<BlueprintFieldType> wrapper(evmone::OP_MLOAD, this);
    test_mload<BlueprintFieldType>(wrapper, 32, this);
}

TEST_F(AssignerTest, test_op_mstore) {
    TestRWCircuitStruct<BlueprintFieldType> wrapper(evmone::OP_MSTORE, this);

    typename nil::evm_assigner::zkevm_word<BlueprintFieldType> value = {0x18191a1b1c1d1e1f, 0x1011121314151617, 0x08090a0b0c0d0e0f, 0x0001020304050607};
    typename nil::evm_assigner::zkevm_word<BlueprintFieldType> index = 0;

    *(++wrapper.position.stack_top) = value;
    *(++wrapper.position.stack_top) = index;
    const auto addr = index.to_uint64();

    uint8_t expected_mem[32];
    for (int i = 0; i < 32; i++) {
        expected_mem[i] = i;
    }
    (&wrapper.state.memory)->grow(32);

    evmone::instr::core::instructions<BlueprintFieldType>::template mstore<intx::uint256>(wrapper.position.stack_top, wrapper.state.msg->gas, wrapper.state);
    assigner_ptr->handle_rw(wrapper.state.rw_trace);

    std::uint32_t row = 0;

    stack_rw_circuit_check(assignments, row++ /*row*/, 0/*address in stack*/, 0/*trace size*/, false/*is_write*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(value).w_hi()/*value_hi*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(value).w_lo()/*value_lo*/);
    stack_rw_circuit_check(assignments, row++ /*row*/, 1/*address in stack*/, 1/*trace size*/, false/*is_write*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(index).w_hi()/*value_hi*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(index).w_lo()/*value_lo*/);

    for (std::uint32_t i = 0; i < 32; i++) {
        memory_rw_circuit_check(assignments, row++ , addr + i, 2 + i, true, 0, expected_mem[i]);
    }
}

TEST_F(AssignerTest, test_op_mstore8) {
    TestRWCircuitStruct<BlueprintFieldType> wrapper(evmone::OP_MSTORE8, this);

    typename nil::evm_assigner::zkevm_word<BlueprintFieldType> value = 0x0102030405060708;
    typename nil::evm_assigner::zkevm_word<BlueprintFieldType> index = 0;

    *(++wrapper.position.stack_top) = value;
    *(++wrapper.position.stack_top) = index;
    const auto addr = index.to_uint64();

    uint8_t expected_mem[8] = {1, 2, 3, 4, 5, 6, 7, 8};

    (&wrapper.state.memory)->grow(8);

    evmone::instr::core::instructions<BlueprintFieldType>::mstore8(wrapper.position.stack_top, wrapper.state.msg->gas, wrapper.state);
    assigner_ptr->handle_rw(wrapper.state.rw_trace);

    std::uint32_t row = 0;

    stack_rw_circuit_check(assignments, row++ /*row*/, 0/*address in stack*/, 0/*trace size*/, false/*is_write*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(value).w_hi()/*value_hi*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(value).w_lo()/*value_lo*/);
    stack_rw_circuit_check(assignments, row++ /*row*/, 1/*address in stack*/, 1/*trace size*/, false/*is_write*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(index).w_hi()/*value_hi*/, typename nil::evm_assigner::zkevm_word<BlueprintFieldType>(index).w_lo()/*value_lo*/);

    for (std::uint32_t i = 0; i < 8; i++) {
        memory_rw_circuit_check(assignments, row++ , addr + i, 2 + i, true, 0, expected_mem[i]);
    }
}

    // next opcode is OP_SLOAD = 0x54,


#undef TEST_STACK_1_INPUTS
#undef TEST_STACK_1_INPUTS_ALL_CASES
#undef TEST_STACK_2_INPUTS
#undef TEST_STACK_2_INPUTS_ALL_CASES
#undef TEST_STACK_3_INPUTS
#undef TEST_STACK_2_INPUTS_ALL_CASES


// TODO add check assignment tables
TEST_F(AssignerTest, DISABLED_callvalue_calldataload)
{
    const uint8_t index = 10;
    std::vector<uint8_t> code = {
        evmone::OP_CALLVALUE,
        evmone::OP_PUSH1,
        index,
        evmone::OP_CALLDATALOAD,
    };
    nil::evm_assigner::evaluate<BlueprintFieldType>(host_interface, ctx, rev, &msg, code.data(), code.size(), assigner_ptr);
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
    nil::evm_assigner::evaluate<BlueprintFieldType>(host_interface, ctx, rev, &msg, code.data(), code.size(), assigner_ptr);
    EXPECT_EQ(assignments[0].witness(1, 2), index);
}

// TODO add check assignment tables
TEST_F(AssignerTest, DISABLED_mstore_load)
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
    nil::evm_assigner::evaluate<BlueprintFieldType>(host_interface, ctx, rev, &msg, code.data(), code.size(), assigner_ptr);
    EXPECT_EQ(assignments[0].witness(2, 0), value);
    EXPECT_EQ(assignments[0].witness(2, 1), index);
    EXPECT_EQ(assignments[0].witness(2, 2), value);
}

// TODO add check assignment tables
TEST_F(AssignerTest, DISABLED_sstore_load)
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
    nil::evm_assigner::evaluate<BlueprintFieldType>(host_interface, ctx, rev, &msg, code.data(), code.size(), assigner_ptr);
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
    nil::evm_assigner::evaluate<BlueprintFieldType>(host_interface, ctx, rev, &msg, code.data(), code.size(), assigner_ptr);
    EXPECT_EQ(assignments[0].witness(4, 0), value);
    EXPECT_EQ(assignments[0].witness(4, 1), key);
    EXPECT_EQ(assignments[0].witness(4, 2), value);
}

// TODO add check assignment tables
TEST_F(AssignerTest, DISABLED_create) {

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

    nil::evm_assigner::evaluate<BlueprintFieldType>(host_interface, ctx, rev, &msg, code.data(), code.size(), assigner_ptr);
    // Check stored witnesses of MSTORE instruction at depth 1
    EXPECT_EQ(assignments[1].witness(2, 1), 0);
    EXPECT_EQ(assignments[1].witness(2, 0), 0xFFFFFFFF);
}

// TODO add check assignment tables
TEST_F(AssignerTest, DISABLED_call) {

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

    nil::evm_assigner::evaluate<BlueprintFieldType>(host_interface, ctx, rev, &msg, code.data(), code.size(), assigner_ptr);
    // Check stored witness of CALLDATALOAD instruction at depth 1
    EXPECT_EQ(assignments[1].witness(1, 1), 0);
}
