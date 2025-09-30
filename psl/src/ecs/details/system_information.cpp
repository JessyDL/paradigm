#include "psl/ecs/details/system_information.hpp"
#include "psl/ecs/state.hpp"

namespace psl::ecs::details {
void system_token::add_dependency(system_token& token) {
	if(m_Owner) {
		m_Owner->system_connect(*this, token);
	}
}
void system_token::remove_dependency(system_token& token) {
	if(m_Owner) {
		m_Owner->system_disconnect(*this, token);
	}
}

auto system_token::dependencies() const noexcept -> std::unordered_set<system_token> const& {
	if(m_Owner) {
		return m_Owner->system_dependencies(*this);
	}
	static std::unordered_set<system_token> empty {};
	return empty;
}
}	 // namespace psl::ecs::details