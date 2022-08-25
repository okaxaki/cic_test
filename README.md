# CIC Sample Rate Conversion Test

```
Usage: cic [options] <file.raw>

Options: 
  -u<num>   Upsampling factor (default: 192)
  -d<num>   Downsampling factor (default: 125)
  -n<num>   Number of stages (default: 3)
  -z<num>   Padding zero in upsampling interporation. 0:off 1:on (default: 1).
  -o<file>  Output filename (default: output.raw)
  -h        Print this help.
```

The input file must be a 16-bit signed (LE) RAW wave binary format.

# How to Build

```
mkdir build
cd build
cmake ..
cmake --build .
```
