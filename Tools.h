#include <vector>
#include <stdint.h>

#include "ZRawImageBlockLine.hpp"

void __post_process_a(ZRawImageBlockLine &line, bool is_needed_field, int noise_level);
void __post_process_b(ZRawImageBlockLine &line, int noise_level);
void __post_process_truncate(std::vector<uint16_t> &line, int real_bitdepth, int diff);
uint32_t __estimate_noise_level(int noise_level_1, int noise_level_2, int less_than_distance_pix_count, uint32_t noise_levels[8]);
