#!/usr/bin/env python3
"""Render and audit Matrix native 3D effects on the host."""

from __future__ import annotations

import subprocess
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
BUILD_DIR = REPO_ROOT / ".pio" / "matrix3d_audit"
SOURCE_PATH = BUILD_DIR / "matrix3d_audit.cpp"
BINARY_PATH = BUILD_DIR / "matrix3d_audit"
PPM_PATH = BUILD_DIR / "matrix_3d_effects.ppm"


CPP_SOURCE = r'''
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "lib/matrix_service/effects/MatrixFxEngine3D.cpp"

namespace {

constexpr uint8_t kScale = 8;
constexpr uint8_t kRows = 3;
constexpr uint8_t kModes = MATRIX_FX::kNative3DModeMax + 1;
constexpr uint16_t kImageWidth = MATRIX_FX::kMatrixFxWidth * kScale * kModes;
constexpr uint16_t kImageHeight = MATRIX_FX::kMatrixFxHeight * kScale * kRows;

MATRIX_FX::MatrixFxConfig makeConfig(uint8_t mode, bool warm) {
    MATRIX_FX::MatrixFxConfig config;
    config.mode = mode;
    config.speedMs = 900;
    config.provider = MATRIX_FX::ReactiveProvider::Imu;
    config.reactivityGain = 125;
    if (warm) {
        config.color1 = 0x400020;
        config.color2 = 0xFF5030;
        config.color3 = 0xFFD060;
    } else {
        config.color1 = 0x0030FF;
        config.color2 = 0x00FF90;
        config.color3 = 0xB0F0FF;
    }
    return config;
}

MATRIX_FX::MatrixFxInput makeInput(bool tilted) {
    MATRIX_FX::MatrixFxInput input;
    if (tilted) {
        input.imuValid = true;
        input.ax = -0.72f;
        input.ay = 0.58f;
        input.az = 0.38f;
        input.gx = 90.0f;
        input.gy = -55.0f;
        input.gz = 40.0f;
        input.motionEnergy = 0.45f;
    }
    return input;
}

void renderFrame(uint8_t mode, bool warm, bool tilted, uint32_t* frame) {
    MATRIX_FX::MatrixFxEngine3D engine;
    engine.configure(makeConfig(mode, warm));
    engine.setInput(makeInput(tilted));
    for (uint8_t step = 0; step < 8; ++step) {
        if (!engine.render(1000 + step * 33, frame, MATRIX_FX::kMatrixFxPixelCount)) {
            std::fprintf(stderr, "render failed for mode %u\n", mode);
            std::exit(2);
        }
    }
}

uint8_t brightness(uint32_t color) {
    const uint8_t r = static_cast<uint8_t>((color >> 16) & 0xFF);
    const uint8_t g = static_cast<uint8_t>((color >> 8) & 0xFF);
    const uint8_t b = static_cast<uint8_t>(color & 0xFF);
    return static_cast<uint8_t>((static_cast<uint16_t>(r) + g + b) / 3);
}

uint32_t litPixels(const uint32_t* frame) {
    uint32_t count = 0;
    for (uint8_t i = 0; i < MATRIX_FX::kMatrixFxPixelCount; ++i) {
        if ((frame[i] & 0x00FFFFFFu) != 0) {
            count++;
        }
    }
    return count;
}

uint32_t uniqueColors(const uint32_t* frame) {
    uint32_t unique = 0;
    uint32_t seen[MATRIX_FX::kMatrixFxPixelCount]{};
    for (uint8_t i = 0; i < MATRIX_FX::kMatrixFxPixelCount; ++i) {
        const uint32_t color = frame[i] & 0x00FFFFFFu;
        bool exists = false;
        for (uint8_t j = 0; j < unique; ++j) {
            if (seen[j] == color) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            seen[unique++] = color;
        }
    }
    return unique;
}

uint32_t changedPixels(const uint32_t* a, const uint32_t* b) {
    uint32_t count = 0;
    for (uint8_t i = 0; i < MATRIX_FX::kMatrixFxPixelCount; ++i) {
        if ((a[i] & 0x00FFFFFFu) != (b[i] & 0x00FFFFFFu)) {
            count++;
        }
    }
    return count;
}

void writePixel(FILE* file, uint32_t color) {
    const uint8_t r = static_cast<uint8_t>((color >> 16) & 0xFF);
    const uint8_t g = static_cast<uint8_t>((color >> 8) & 0xFF);
    const uint8_t b = static_cast<uint8_t>(color & 0xFF);
    std::fprintf(file, "%u %u %u ", r, g, b);
}

void writeSheet(const char* path, const uint32_t frames[kRows][kModes][MATRIX_FX::kMatrixFxPixelCount]) {
    FILE* file = std::fopen(path, "w");
    if (!file) {
        std::perror(path);
        std::exit(3);
    }

    std::fprintf(file, "P3\n%u %u\n255\n", kImageWidth, kImageHeight);
    for (uint16_t outY = 0; outY < kImageHeight; ++outY) {
        const uint8_t row = outY / (MATRIX_FX::kMatrixFxHeight * kScale);
        const uint8_t localY = (outY / kScale) % MATRIX_FX::kMatrixFxHeight;
        for (uint16_t outX = 0; outX < kImageWidth; ++outX) {
            const uint8_t mode = outX / (MATRIX_FX::kMatrixFxWidth * kScale);
            const uint8_t localX = (outX / kScale) % MATRIX_FX::kMatrixFxWidth;
            writePixel(file, frames[row][mode][localY * MATRIX_FX::kMatrixFxWidth + localX]);
        }
        std::fputc('\n', file);
    }
    std::fclose(file);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s output.ppm\n", argv[0]);
        return 2;
    }

    uint32_t frames[kRows][kModes][MATRIX_FX::kMatrixFxPixelCount]{};
    bool ok = true;

    for (uint8_t mode = 0; mode < kModes; ++mode) {
        renderFrame(mode, false, false, frames[0][mode]);
        renderFrame(mode, false, true, frames[1][mode]);
        renderFrame(mode, true, true, frames[2][mode]);

        const uint32_t lit = litPixels(frames[0][mode]);
        const uint32_t unique = uniqueColors(frames[0][mode]);
        const uint32_t imuDelta = changedPixels(frames[0][mode], frames[1][mode]);
        const uint32_t paletteDelta = changedPixels(frames[1][mode], frames[2][mode]);

        std::printf("mode=%02u lit=%02lu unique=%02lu imu_delta=%02lu palette_delta=%02lu\n",
                    mode,
                    static_cast<unsigned long>(lit),
                    static_cast<unsigned long>(unique),
                    static_cast<unsigned long>(imuDelta),
                    static_cast<unsigned long>(paletteDelta));

        ok = ok && lit > 4 && unique > 3 && imuDelta > 0 && paletteDelta > 4;
    }

    writeSheet(argv[1], frames);
    std::printf("wrote %s (%ux%u)\n", argv[1], kImageWidth, kImageHeight);
    return ok ? 0 : 1;
}
'''


def main() -> int:
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    SOURCE_PATH.write_text(CPP_SOURCE, encoding="utf-8")

    compile_cmd = [
        "g++",
        "-std=c++17",
        "-O2",
        "-I",
        str(REPO_ROOT),
        str(SOURCE_PATH),
        "-o",
        str(BINARY_PATH),
    ]
    subprocess.run(compile_cmd, cwd=REPO_ROOT, check=True)
    subprocess.run([str(BINARY_PATH), str(PPM_PATH)], cwd=REPO_ROOT, check=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
