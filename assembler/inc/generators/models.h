#pragma once
#include "cli/value.h"

namespace assembler::generators {
class models {
  public:
	models();
	~models() = default;

	models(models const&)			 = delete;
	models(models&&)				 = delete;
	models& operator=(models const&) = delete;
	models& operator=(models&&)		 = delete;


	psl::cli::pack& pack() {
		return m_Pack;
	}

  private:
	void on_invoke(psl::cli::pack& pack);

	psl::cli::pack m_Pack;
};
}	 // namespace assembler::generators
