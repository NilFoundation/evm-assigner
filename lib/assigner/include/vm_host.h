#ifndef EVM_ASSIGNER_LIB_ASSIGNER_INCLUDE_VM_HOST_H_
#define EVM_ASSIGNER_LIB_ASSIGNER_INCLUDE_VM_HOST_H_

// Based on example host

#include <evmc/evmc.hpp>
#include <nil/blueprint/handler_base.hpp>

#include <algorithm>
#include <map>
#include <vector>
#include <memory>

using namespace evmc::literals;

namespace evmc
{
struct account
{
    virtual ~account() = default;

    evmc::uint256be balance = {};
    std::vector<uint8_t> code;
    std::map<evmc::bytes32, evmc::bytes32> storage;
    std::map<evmc::bytes32, evmc::bytes32> transient_storage;

    virtual evmc::bytes32 code_hash() const
    {
        // Extremely dumb "hash" function.
        evmc::bytes32 ret{};
        for (const auto v : code)
            ret.bytes[v % sizeof(ret.bytes)] ^= v;
        return ret;
    }
};

using accounts = std::map<evmc::address, account>;

}  // namespace evmc

class VMHost : public evmc::Host
{
    evmc::accounts accounts;
    evmc_tx_context tx_context{};
    std::shared_ptr<nil::blueprint::handler_base> handler;

public:
    VMHost() = default;
    explicit VMHost(evmc_tx_context& _tx_context, std::shared_ptr<nil::blueprint::handler_base> _handler) noexcept
      : tx_context{_tx_context}, handler{_handler}
    {}

    VMHost(evmc_tx_context& _tx_context, evmc::accounts& _accounts, std::shared_ptr<nil::blueprint::handler_base> _handler) noexcept
      : accounts{_accounts}, tx_context{_tx_context}, handler{_handler}
    {}

    bool account_exists(const evmc::address& addr) const noexcept final
    {
        return accounts.find(addr) != accounts.end();
    }

    evmc::bytes32 get_storage(const evmc::address& addr,
                              const evmc::bytes32& key) const noexcept final
    {
        const auto account_iter = accounts.find(addr);
        if (account_iter == accounts.end())
            return {};

        const auto storage_iter = account_iter->second.storage.find(key);
        if (storage_iter != account_iter->second.storage.end())
            return storage_iter->second;
        return {};
    }

    evmc_storage_status set_storage(const evmc::address& addr,
                                    const evmc::bytes32& key,
                                    const evmc::bytes32& value) noexcept final
    {
        auto& account = accounts[addr];
        auto prev_value = account.storage[key];
        account.storage[key] = value;

        return (prev_value == value) ? EVMC_STORAGE_ASSIGNED : EVMC_STORAGE_MODIFIED;
    }

    evmc::uint256be get_balance(const evmc::address& addr) const noexcept final
    {
        auto it = accounts.find(addr);
        if (it != accounts.end())
            return it->second.balance;
        return {};
    }

    size_t get_code_size(const evmc::address& addr) const noexcept final
    {
        auto it = accounts.find(addr);
        if (it != accounts.end())
            return it->second.code.size();
        return 0;
    }

    evmc::bytes32 get_code_hash(const evmc::address& addr) const noexcept final
    {
        auto it = accounts.find(addr);
        if (it != accounts.end())
            return it->second.code_hash();
        return {};
    }

    size_t copy_code(const evmc::address& addr,
                     size_t code_offset,
                     uint8_t* buffer_data,
                     size_t buffer_size) const noexcept final
    {
        const auto it = accounts.find(addr);
        if (it == accounts.end())
            return 0;

        const auto& code = it->second.code;

        if (code_offset >= code.size())
            return 0;

        const auto n = std::min(buffer_size, code.size() - code_offset);

        if (n > 0)
            std::copy_n(&code[code_offset], n, buffer_data);
        return n;
    }

    bool selfdestruct(const evmc::address& addr, const evmc::address& beneficiary) noexcept final
    {
        (void)addr;
        (void)beneficiary;
        return false;
    }

    evmc::Result call(const evmc_message& msg) noexcept final
    {
        switch (msg.kind)
        {
        case EVMC_CALL:
        case EVMC_CALLCODE:
        case EVMC_DELEGATECALL:
            return handle_call(msg);
        case EVMC_CREATE:
        case EVMC_CREATE2:
            return handle_create(msg);
        default:
            // Unexpected opcode
            return evmc::Result{EVMC_INTERNAL_ERROR};
        }
    }

    evmc_tx_context get_tx_context() const noexcept final { return tx_context; }

    // NOLINTNEXTLINE(bugprone-exception-escape)
    evmc::bytes32 get_block_hash(int64_t number) const noexcept final
    {
        const int64_t current_block_number = get_tx_context().block_number;

        return (number < current_block_number && number >= current_block_number - 256) ?
                   0xb10c8a5fb10c8a5fb10c8a5fb10c8a5fb10c8a5fb10c8a5fb10c8a5fb10c8a5f_bytes32 :
                   0x0000000000000000000000000000000000000000000000000000000000000000_bytes32;
    }

    void emit_log(const evmc::address& addr,
                  const uint8_t* data,
                  size_t data_size,
                  const evmc::bytes32 topics[],
                  size_t topics_count) noexcept final
    {
        (void)addr;
        (void)data;
        (void)data_size;
        (void)topics;
        (void)topics_count;
    }

    evmc_access_status access_account(const evmc::address& addr) noexcept final
    {
        (void)addr;
        return EVMC_ACCESS_COLD;
    }

    evmc_access_status access_storage(const evmc::address& addr,
                                      const evmc::bytes32& key) noexcept final
    {
        (void)addr;
        (void)key;
        return EVMC_ACCESS_COLD;
    }

    evmc::bytes32 get_transient_storage(const evmc::address& addr,
                                        const evmc::bytes32& key) const noexcept override
    {
        const auto account_iter = accounts.find(addr);
        if (account_iter == accounts.end())
            return {};

        const auto transient_storage_iter = account_iter->second.transient_storage.find(key);
        if (transient_storage_iter != account_iter->second.transient_storage.end())
            return transient_storage_iter->second;
        return {};
    }

    void set_transient_storage(const evmc::address& addr,
                               const evmc::bytes32& key,
                               const evmc::bytes32& value) noexcept override
    {
        accounts[addr].transient_storage[key] = value;
    }

private:
    evmc::Result handle_call(const evmc_message& msg);
    evmc::Result handle_create(const evmc_message& msg);
    evmc::address calculate_address(const evmc_message& msg);
};


extern "C" {

evmc_host_context* vm_host_create_context(evmc_tx_context tx_context, std::shared_ptr<nil::blueprint::handler_base> handler);
void vm_host_destroy_context(evmc_host_context* context);
}

#endif  // EVM_ASSIGNER_LIB_ASSIGNER_INCLUDE_VM_HOST_H_
