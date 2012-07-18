//
//  main.cpp
//  mandelbrot
//
//  Created by Joe Groff on 7/17/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

// maxItValue=sqrt(abs(2*sqrt(abs(1-sqrt(5*scale)))))*66.5

#include <cstdio>
#include <cstdint>
#include <memory>

union pixel_t {
    uint8_t bytes[4];
    uint32_t word;
};
static const pixel_t blackPixel = {0,0,0,255};

static std::uint32_t colorPixel(std::size_t index, std::size_t iterations)
{
    using namespace std;
    
    if (index == iterations) {
        return blackPixel.word;
    } else {
        static const pixel_t colors[8] = {
            {0, 0, 1, 1},
            {0, 1, 1, 1},
            {1, 1, 1, 1},
            {1, 1, 0, 1},
            {1, 0, 0, 1},
            {0, 0, 0, 1},
            {0, 1, 0, 1},
            {0, 0, 0, 1},
        };
        uint32_t grade = index*4 % 256;
        uint32_t color0 = (index*4 / 256) % 8;
        uint32_t color1 = (color0 + 1) % 8;
        
        return uint32_t(255 - grade) * colors[color0].word + grade * colors[color1].word;
    }
}

static std::uint32_t mandelbrot(double cr, double ci, std::size_t iterations)
{
    double zr = 0.0, zi = 0.0;
    std::size_t i = 0;
    while (i < iterations && zr*zr + zi*zi < 4.0) {
        double zr1 = zr*zr - zi*zi + cr;
        zi = 2*zr*zi + ci;
        zr = zr1;
        ++i;
    }
    return colorPixel(i, iterations);
}

static bool inBulb(double lowx, double lowy, double highx, double highy)
{
    return false;
}

int main(int argc, const char * argv[])
{
    using namespace std;
    
    if (argc < 9) {
        fprintf(stderr, "usage: %s filename lowx lowy highx highy iterations width height\n", argv[0]);
        return 2;
    }
    
    char const * outfile = argv[1];
    double lowx, lowy, highx, highy;
    size_t iterations, wi, hi;
    
    lowx = strtod(argv[2], nullptr);
    lowy = strtod(argv[3], nullptr);
    highx = strtod(argv[4], nullptr);
    highy = strtod(argv[5], nullptr);
    iterations = size_t(strtoull(argv[6], nullptr, 10));
    wi = size_t(strtoull(argv[7], nullptr, 10));
    hi = size_t(strtoull(argv[8], nullptr, 10));
    
    if (iterations == 0) {
        fprintf(stderr, "invalid iteration count %s\n", argv[7]);
        return 1;
    }
    if (wi == 0) {
        fprintf(stderr, "invalid width %s\n", argv[7]);
        return 1;
    }
    if (hi == 0) {
        fprintf(stderr, "invalid height %s\n", argv[8]);
        return 1;        
    }
    
    unique_ptr<uint32_t[]> data(new uint32_t[wi*hi]);
    FILE *out = fopen(outfile, "wb");
    if (!out) {
        fprintf(stderr, "unable to write to %s: %s\n", outfile, strerror(errno));
        return 1;
    }
    
    if (inBulb(lowx, lowy, highx, highy)) {
        for (uint32_t *pixel = data.get(), *end = pixel + wi*hi; pixel < end; ++pixel)
            *pixel = blackPixel.word;
    } else {
        double dx = (highx - lowx)/double(wi);
        double dy = (highy - lowy)/double(hi);
        
        double y = lowy + 0.5*dy;
        uint32_t *pixel = data.get();
        for (size_t yi = 0; yi < hi; ++yi, y += dy) {
            double x = lowx + 0.5*dx;
            for (size_t xi = 0; xi < wi; ++xi, x += dx, ++pixel) {
                *pixel = mandelbrot(x, y, iterations);
            }
        }
    }
    
    fwrite(reinterpret_cast<void*>(data.get()), wi*hi*sizeof(uint32_t), 1, out);
    fclose(out);
    
    return 0;
}