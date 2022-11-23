tests := bzip2 sjeng wrf sphinx3

all: $(tests)

build:
	cmake --build build

$(tests): build
	./build/_deps/intelpin-src/pin -t ./build/libbrchPredict.so -- runspec --size=test --noreportable --nobuild $@

.PHONY: build all $(tests)