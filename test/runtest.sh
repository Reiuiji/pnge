#!/bin/bash
# Script to test run pnge

echo "Building pnge (unix)"
pushd ../.
make
popd

echo "Encoding Message.txt into Lenna.png (Lenna_enc.png)"
../pnge -f Message.txt Lenna.png Lenna_enc.png

echo "Decoding Message.txt from Lenna_enc.png (Message_dec.png)"
../pnge -f -d Lenna_enc.png Message_dec.txt

#echo "MD5 Check Sums for both Messages"
MD5a=`md5sum Message.txt | cut -d " " -f1`
MD5b=`md5sum Message_dec.txt | cut -d " " -f1`

if [[ $MD5a == $MD5b ]]
then
	echo "MD5 Check Sums MATCH"
else
	echo "MD5 Check Sums DO NOT MATCH"
	echo "Message.txt: $MD5a"
	echo "Message_dec.txt: $MD5b"
fi
