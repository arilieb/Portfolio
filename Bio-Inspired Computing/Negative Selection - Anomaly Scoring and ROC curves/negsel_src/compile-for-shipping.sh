#!/bin/bash

svn export . shipout

cd shipout 

for d in org algorithms alphabets util ; do
   echo "compiling $d ..."
   find $d -name '*.java' | xargs javac 
done

echo "compiling Main.java ..."
javac Main.java

jar cfe ../negsel2.jar Main .

cd ..

rm -rf shipout
