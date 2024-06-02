# fonthdr

Extract an freetype font array from font file with fixed size, this can be useful when device has very limited storage space and fixed font size.

Include all output in one header file.

## build

```
git clone https://github.com/junka/fonthdr.git
cd fonthdr
mkdir build
cmake ..
make
```

It will automatically build the example ```ftbmp ``` with alphabet from a to z.
run example ftbmp will give you the bmp image that contains a-z.

Also it shows how to use the generated header file ```ftsrc.h```.

## usage

```
fonthdr -f <font file> -s <string>
```

This will generate ```ftsrc.h``` with a given charactors. Any utf8 string could be decoded right.

