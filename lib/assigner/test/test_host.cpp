#include "test_host.h"

#include <intx/intx.hpp>
#include <ethash/keccak.hpp>

extern "C" {

evmc_host_context* test_host_create_context(evmc_tx_context tx_context, AssignerType *assigner)
{
    auto host = new TestHost{tx_context, assigner};
    return host->to_context();
}

void test_host_destroy_context(evmc_host_context* context)
{
    delete evmc::Host::from_context<TestHost>(context);
}
}

evmc::Result TestHost::handle_call(const evmc_message& msg)
{
    evmc_vm * vm = evmc_create_evmone();
    auto account_iter = accounts.find(msg.code_address);
    auto &sender_acc = accounts[msg.sender];
    if (account_iter == accounts.end())
    {
        // Create account
        accounts[msg.code_address] = {};
    }
    auto& acc = account_iter->second;
    if (msg.kind == EVMC_CALL) {
        auto value_to_transfer = intx::be::load<intx::uint256>(msg.value);
        auto balance = intx::be::load<intx::uint256>(sender_acc.balance);
        // Balance was already checked in evmone, so simply adjust it
        sender_acc.balance = intx::be::store<evmc::uint256be>(balance - value_to_transfer);
        acc.balance = intx::be::store<evmc::uint256be>(
            value_to_transfer + intx::be::load<intx::uint256>(acc.balance));
    }
    if (acc.code.empty())
    {
        return evmc::Result{EVMC_SUCCESS, msg.gas, 0, msg.input_data, msg.input_size};
    }
    // TODO: handle precompiled contracts
    auto res = assigner->evaluate(vm, &get_interface(), to_context(),
        EVMC_LATEST_STABLE_REVISION, &msg, acc.code.data(), acc.code.size());
    return res;
}

evmc::Result TestHost::handle_create(const evmc_message& msg)
{
    evmc::address new_contract_address = calculate_address(msg);
    accounts[new_contract_address] = {};
    if (msg.input_size == 0)
    {
        return evmc::Result{EVMC_SUCCESS, msg.gas, 0, new_contract_address};
    }
    evmc::VM vm{evmc_create_evmone()};
    evmc_message init_msg(msg);
    init_msg.kind = EVMC_CALL;
    init_msg.recipient = new_contract_address;
    init_msg.sender = msg.sender;
    init_msg.input_size = 0;
    auto res = assigner->evaluate(vm.get_raw_pointer(), &get_interface(), to_context(),
        EVMC_LATEST_STABLE_REVISION, &init_msg, msg.input_data, msg.input_size);

    if (res.status_code == EVMC_SUCCESS)
    {
        accounts[new_contract_address].code =
            std::vector<uint8_t>(res.output_data, res.output_data + res.output_size);
    }
    res.create_address = new_contract_address;
    return res;
}

evmc::address TestHost::calculate_address(const evmc_message& msg)
{
    // TODO: Implement for CREATE opcode, for now the result is only correct for CREATE2
    // CREATE requires rlp encoding
    auto seed = intx::be::load<intx::uint256>(msg.create2_salt);
    auto hash = intx::be::load<intx::uint256>(ethash::keccak256(msg.input_data, msg.input_size));
    auto sender = intx::be::load<intx::uint256>(msg.sender);
    auto sum = 0xff + seed + hash + sender;
    auto rehash = ethash::keccak256(reinterpret_cast<const uint8_t *>(&sum), sizeof(sum));
    // Result address is the last 20 bytes of the hash
    evmc::address res;
    std::memcpy(res.bytes, rehash.bytes + 12, 20);
    return res;
}
