set -e
cd thirdparty

# build spdlog
git clone https://github.com/gabime/spdlog.git
cd spdlog
cmake -B build -DCMAKE_INSTALL_PREFIX=install
cd build
make install
cd ..

# build pfr
git clone https://github.com/boostorg/pfr.git


# build backward-cpp
git clone https://github.com/bombela/backward-cpp.git