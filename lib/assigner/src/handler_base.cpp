#include <vm.hpp>
#include <baseline.hpp>
#include <execution_state.hpp>
#include <nil/blueprint/handler_base.hpp>

namespace nil {
    namespace blueprint {
        evmc::Result evaluate(std::shared_ptr<handler_base> handler, evmc_vm* c_vm, const evmc_host_interface* host, evmc_host_context* ctx,
            evmc_revision rev, const evmc_message* msg, const uint8_t* code, size_t code_size) {

            auto vm = static_cast<evmone::VM*>(c_vm);
            const evmone::bytes_view container{code, code_size};
            const auto code_analysis = evmone::baseline::analyze(rev, container);
            const auto data = code_analysis.eof_header.get_data(container);
            auto state = std::make_unique<evmone::ExecutionState>(*msg, rev, *host, ctx, container, data);
            state->set_handler(handler);

            auto res = execute(*vm, msg->gas, *state, code_analysis);
            return evmc::Result{res};
        }
    }   // namespace blueprint
}   // namespace nil
