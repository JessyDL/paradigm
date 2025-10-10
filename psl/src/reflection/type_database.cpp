#include "psl/reflection/type_database.hpp"

namespace psl::refl::impl {
type_database_t& GetTypeDatabase() {
	static type_database_t db;
	return db;
}
}	 // namespace psl::refl::impl
namespace psl::refl {
type_database_t& type_database_t::global_instance() {
	return impl::GetTypeDatabase();
}
}	 // namespace psl::refl
