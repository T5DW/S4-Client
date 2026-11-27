#pragma once
namespace lua::runtime {
bool try_install() noexcept;
void shutdown();
void request_reload();
}
