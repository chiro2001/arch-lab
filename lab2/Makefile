tests := bzip2 sjeng wrf sphinx3

all: $(tests)

build:
	cmake -B build -S .
	cmake --build build

build-prepare:
	cmake -B build -S . -DCMAKE_CXX_FLAGS="-DPREPARING=1"
	cmake --build build

$(tests): build
	# ./build/_deps/intelpin-src/pin -t ./build/libbrchPredict.so -- runspec --size=test --noreportable --nobuild $@
	./build/_deps/intelpin-src/pin -t ./build/libbrchPredict.so -- runspec --size=test --noreportable $@

prepare-%: build-prepare
	./build/_deps/intelpin-src/pin -t ./build/libbrchPredict.so -- runspec --size=test --noreportable $*

prepare: $(foreach t,$(tests),prepare-$(t))

coremark: build
	./build/_deps/intelpin-src/pin -t ./build/libbrchPredict.so -- ../lab1/workspace/coremark.exe

clean:
	-rm -rf build brchPredict*.txt

.PHONY: build all coremark $(tests) clean