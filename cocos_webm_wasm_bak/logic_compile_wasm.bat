em++ --no-entry -s ERROR_ON_UNDEFINED_SYMBOLS=0 -Wl,--export-dynamic -std=c++20 -O2 -I../../xxlib/src -I../../emsdk/upstream/emscripten  -DEMSCRIPTEN   Logic.cpp   -o ../Resources/logic.wasm
#echo -s MALLOC=emmalloc 