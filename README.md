PNG Encoder

This application will be able to encode any data into a png image.

Remember to run the test before saving to make sure the data your injecting into the image can fit with no loss.

`./pnge -t [source text] [source image]`

Default encoding is 3 bits a pixel and you can change the encoding with the "-b" option.

To encode a image follow the example below.

./pnge [source text] [source image] [encoded image]

To decode run the following
./pnge -d [encoded image] [decoded text]

If there are issues/bugs message reiuiji

Compile
#linux
make
#windows (cygwin)
make -f Makefile.win
#Mac
make -f Makefile.osx