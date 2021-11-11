/*
 * LeanHRPT Demod
 * Copyright (C) 2021 Xerbo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DIGITAL_BLOCKS_H
#define DIGITAL_BLOCKS_H

#include "dsp/block.h"
#include "util/math.h"

#include "deframer.h"
#include "derand.h"
#include "reedsolomon.h"
#include "viterbi.h"

class MetopViterbi : public Block<complex, uint8_t> {
    public:
        MetopViterbi() : vit(0.45f, 5) { }

        size_t work(const complex *in, uint8_t *out, size_t n) {
            symbols.reserve(n);
            for (size_t i = 0; i < n; i++) {
                float real = clamp(in[i].real()*127.0f, 127.0f);
                float imag = clamp(in[i].imag()*127.0f, 127.0f);
                symbols[i] = std::complex<int8_t>(real, imag);
            }

            return vit.work(symbols.data(), out, BUFFER_SIZE);
        }
    private:
        Viterbi vit;
        std::vector<std::complex<int8_t>> symbols;
};

class VCDUExtractor : public Block<uint8_t, uint8_t> {
    public:
        VCDUExtractor() : Block(1024) { }

        size_t work(const uint8_t *in, uint8_t *out, size_t n) {
            uint8_t frame[1024];
            if (deframer.work(in, frame, n)) {
                derand.work(frame, 1024);
                rs.decode_intreleaved_ccsds(frame);

                uint8_t VCID = frame[5] & 0x3f;
                if (VCID != 63) {
                    std::memcpy(out, &frame[4], 892);
                    return 892;
                }
            }

            return 0;
        }
    private:
        ccsds::Deframer deframer;
        ccsds::Derand derand;
        SatHelper::ReedSolomon rs;
};

#endif