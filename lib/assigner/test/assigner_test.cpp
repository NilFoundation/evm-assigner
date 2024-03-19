#include <map>

#include <nil/blueprint/assigner.hpp>
#include <nil/blueprint/blueprint/plonk/assignment.hpp>

#include <evmc/evmc.hpp>
#include <evmc/mocked_host.hpp>

#include <gtest/gtest.h>

TEST(assigner, execute) {
    using BlueprintFieldType = typename nil::crypto3::algebra::curves::pallas::base_field_type;
    using ArithmetizationType = nil::crypto3::zk::snark::plonk_constraint_system<BlueprintFieldType>;

    const std::size_t WitnessColumns = 15;
    const std::size_t PublicInputColumns = 1;

    const std::size_t ConstantColumns = 5;
    const std::size_t SelectorColumns = 30;

    nil::crypto3::zk::snark::plonk_table_description<BlueprintFieldType> desc(
        WitnessColumns, PublicInputColumns, ConstantColumns, SelectorColumns);

    std::vector<nil::blueprint::assignment<ArithmetizationType>> assignments;

    nil::blueprint::assigner<BlueprintFieldType> assigner_instance(desc, assignments);

    auto vm = evmc_create_evmone();
    evmc_revision rev = {};

    // EVM bytecode goes here. This is one of the examples.
    const uint8_t code[] = "\x43\x60\x00\x55\x43\x60\x00\x52\x59\x60\x00\xf3";
    const size_t code_size = sizeof(code) - 1;
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
    const struct evmc_host_interface* host_interface = &evmc::Host::get_interface();
    evmc::MockedHost host;
    struct evmc_host_context* ctx = host.to_context();
    struct evmc_message msg = {
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

    assigner_instance.evaluate(vm, host_interface, ctx, rev, &msg, code, code_size);
}
