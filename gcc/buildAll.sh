#/bin/bash

# Build 64bit version
make clean
make
mv ./CubeSQLPlugin.so ../CubeSQLPlugin/Build\ Resources/Linux\ x86-64/

# Build 32bit version
make clean
make bit32
mv ./CubeSQLPlugin.so ../CubeSQLPlugin/Build\ Resources/Linux\ X86/

# Build ARM version
make clean
make -f MakefileARM
mv ./CubeSQLPlugin.so ../CubeSQLPlugin/Build\ Resources/Linux\ ARM/

# Cleanup
make clean
