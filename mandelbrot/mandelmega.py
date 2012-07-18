from argparse import ArgumentParser
from math import sqrt
from multiprocessing import Pool, cpu_count
from subprocess import call
from os import mkdir
from signal import signal, SIGINT, SIG_IGN
from sys import stdout

def initWorker():
    signal(SIGINT, SIG_IGN)

def xyPair(xy):
    x, comma, y = xy.partition(',')
    if comma != ',' or y is None:
        raise RuntimeError('expected an X,Y pair, but got ', xy)
    return float(x), float(y)

def makeTile(mand, path, lox, loy, hix, hiy, iterations, w, h):
    call([mand, path, str(lox), str(loy), str(hix), str(hiy), str(iterations), str(w), str(h)])
    stdout.write('.')
    stdout.flush()

def writeTiles(file, base, y, x):
    if x == 1:
        file.write('      - ' + str(base) + '\n')
    else:
        x = x >> 1
        writeTiles(file, base,       y, x)
        writeTiles(file, base+x,     y, x)
        writeTiles(file, base+x*y,   y, x)
        writeTiles(file, base+x*y+x, y, x)

SWIZZLE_B = [0x55555555, 0x33333333, 0x0F0F0F0F, 0x00FF00FF];
SWIZZLE_S = [1, 2, 4, 8];
def swizzle(x, y):
    assert(x <= 0xFFFF and y <= 0xFFFF)

    x = (x | (x << SWIZZLE_S[3])) & SWIZZLE_B[3]
    x = (x | (x << SWIZZLE_S[2])) & SWIZZLE_B[2]
    x = (x | (x << SWIZZLE_S[1])) & SWIZZLE_B[1]
    x = (x | (x << SWIZZLE_S[0])) & SWIZZLE_B[0]

    y = (y | (y << SWIZZLE_S[3])) & SWIZZLE_B[3]
    y = (y | (y << SWIZZLE_S[2])) & SWIZZLE_B[2]
    y = (y | (y << SWIZZLE_S[1])) & SWIZZLE_B[1]
    y = (y | (y << SWIZZLE_S[0])) & SWIZZLE_B[0]

    return x | (y << 1)


argp = ArgumentParser(description='Render a Mandelbrot fractal as a Megacanvas document.')
argp.add_argument('name', help='Output document name.')
argp.add_argument('--tile', metavar='size', type=int, help='Tile size in pixels. (Must be power of 2.)',
    required=True)
argp.add_argument('--size', metavar='size', type=int, help='Image size in tiles. (Must be power of 2.)',
    required=True)
argp.add_argument('--lo', metavar='X,Y', type=xyPair, help='Lower bound coordinates.',
    required=True)
argp.add_argument('--hi', metavar='X,Y', type=xyPair, help='Upper bound coordinates.',
    required=True)
argp.add_argument('--iterations', metavar='N', type=int, help='Maximum number of Mandelbrot iterations.',
    required=True)
argp.add_argument('--jobs', metavar='N', type=int, help='Number of concurrent jobs to run.',
    default=cpu_count())
argp.add_argument('--mandelbrot', metavar='path', help='Path to the Mandelbrot engine.',
    required=True)

args = argp.parse_args()

if args.tile <= 0 or (args.tile & (args.tile - 1) != 0):
    raise RuntimeError('tile size is not a power of 2')
if args.size <= 1 or (args.size & (args.size - 1) != 0):
    raise RuntimeError('image size is not a power of 2')

mkdir(args.name)

pool = Pool(processes=args.jobs, initializer=initWorker)
tilespan = [(hi - lo)/float(args.size) for hi,lo in zip(args.hi, args.lo)]
tilesize = [span * float(args.tile) / float(args.tile - 1) for span in tilespan]

tilei = 1
try:
    for y in xrange(args.size):
        for x in xrange(args.size):
            tilelo = [lo + span * xy for lo, span, xy in zip(args.lo, tilespan, [x,y])]
            tilehi = [lo + size for lo, size in zip(tilelo, tilesize)]
            path = args.name + '/' + str(tilei) + '.rgba'
            pool.apply_async(makeTile, [
                args.mandelbrot, path,
                tilelo[0], tilelo[1], tilehi[0], tilehi[1],
                args.iterations, args.tile, args.tile])
            tilei += 1
    pool.close()
    pool.join()
except KeyboardInterrupt, x:
    print "\nInterrupted!"
    pool.terminate()
    raise

print

tilecount = args.size*args.size
quadtreeDepth = len(bin(args.size)) - len('0b1')
tileLogSize = len(bin(args.tile)) - len('0b1')

metadata = open(args.name + '/mega.yaml', 'w')
metadata.write('mega: 1\n')
metadata.write('tile-count: ' + str(tilecount) + '\n')
metadata.write('tile-size: ' + str(tileLogSize) + '\n')
metadata.write('layers:\n')
metadata.write('  - parallax: [1,1]\n')
metadata.write('    origin: [0,0]\n')
metadata.write('    size: ' + str(quadtreeDepth) + '\n')
metadata.write('    tiles:\n')
writeTiles(metadata, 1, args.size, args.size)

metadata.close()
