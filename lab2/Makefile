tests := bzip2 sjeng wrf sphinx3
extra_tests := coremark

all: $(tests) $(extra_tests)

build:
	cmake --build build

configure:
	cmake -B build -S . -G Ninja

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

stats:
	python scripts/draw.py

clean:
	-rm -rf build brchPredict*.txt

.PHONY: build all coremark $(tests) clean