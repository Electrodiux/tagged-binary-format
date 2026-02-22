/*  ==============================================================================
 *  Tagged Binary Format (TBF) - www.electrodiux.com
 *  ------------------------------------------------------------------------------
 *  Copyright (c) 2026 Electrodiux. All rights reserved.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *  ==============================================================================
 */

#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>

namespace tbf {

constexpr std::endian TBF_ENDIANESS = std::endian::little;

template <typename Type>
[[gnu::always_inline]]
inline void AdjustEndianess(Type& value) {
    if constexpr (std::endian::native != TBF_ENDIANESS) {
        if constexpr (sizeof(Type) == 2) {
            value = std::bit_cast<Type>(std::byteswap(std::bit_cast<uint16_t>(value)));
        } else if constexpr (sizeof(Type) == 4) {
            value = std::bit_cast<Type>(std::byteswap(std::bit_cast<uint32_t>(value)));
        } else if constexpr (sizeof(Type) == 8) {
            value = std::bit_cast<Type>(std::byteswap(std::bit_cast<uint64_t>(value)));
        }
    }
}

template <uint32_t size>
    requires(size == 1 || size == 2 || size == 4 || size == 8)
inline void AdjustArrayEndianess(void* data, size_t count) {
    if constexpr (size > 1 && std::endian::native != TBF_ENDIANESS) {
        if constexpr (size == 2) {
            int16_t* int_data = static_cast<int16_t*>(data);
            for (size_t i = 0; i < count; ++i) {
                int_data[i] = std::byteswap(int_data[i]);
            }
        } else if constexpr (size == 4) {
            int32_t* int_data = static_cast<int32_t*>(data);
            for (size_t i = 0; i < count; ++i) {
                int_data[i] = std::byteswap(int_data[i]);
            }
        } else if constexpr (size == 8) {
            int64_t* int_data = static_cast<int64_t*>(data);
            for (size_t i = 0; i < count; ++i) {
                int_data[i] = std::byteswap(int_data[i]);
            }
        }
    }
}

}  // namespace tbf