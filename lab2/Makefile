tests := bzip2 sjeng wrf sphinx3

all: $(tests)

build:
	cmake --build build

$(tests): build
	./build/_deps/intelpin-src/pin -t ./build/libbrchPredict.so -- runspec --size=test --noreportable --nobuild $@

coremark: build
	./build/_deps/intelpin-src/pin -t ./build/libbrchPredict.so -- ../lab1/workspace/coremark.exe

.PHONY: build all coremark $(tests)