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
export PATH="/root/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin:${PATH}"
make clean
make -f MakefileARM
mv ./CubeSQLPlugin.so ../CubeSQLPlugin/Build\ Resources/Linux\ ARM/

# Cleanup
make clean

# Update GitHub repo
cd ..
git add CubeSQLPlugin/Build\ Resources/Linux\ ARM/CubeSQLPlugin.so
git add CubeSQLPlugin/Build\ Resources/Linux\ X86/CubeSQLPlugin.so
git add CubeSQLPlugin/Build\ Resources/Linux\ x86-64/CubeSQLPlugin.so

git commit -m "Linux Builds"
git push
