# SystemC 3.0.2

Сборка из исходников:

```bash
# Зависимости
sudo apt install build-essential cmake

git clone https://github.com/accellera-official/systemc
cd systemc
git checkout 3.0.2
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$(pwd)/../install -DCMAKE_CXX_STANDARD=17
make -j$(nproc)
make install
```

Компиляция модели:
```bash
g++ -std=c++17 -I systemc/install/include -L systemc/install/lib \
    -o model model.cpp -lsystemc
LD_LIBRARY_PATH=systemc/install/lib ./model
```

Используется в:
- [01_03_executable_spec](../01_requirements/01_03_executable_spec.md)
