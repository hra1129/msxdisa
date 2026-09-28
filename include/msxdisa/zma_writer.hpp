// MIT License
#pragma once

#include "msxdisa/analyzer.hpp"

#include <iosfwd>

namespace msxdisa {

void write_zma(std::ostream& output, const RomAnalysis& analysis);

}