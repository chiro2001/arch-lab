# tests := bzip2 sjeng wrf sphinx3
tests := gcc astar zeusmp tonto
extra_tests := coremark

#TOOL ?= brchPredict
TOOL ?= cacheModel

base_dir := $(abspath .)

SAVE ?= .

CACHE_TEST ?= CAPACITY

all: $(tests) $(extra_tests)

build: configure
	cmake --build build

configure:
	cmake -B build -S . -G Ninja -DCMAKE_CXX_FLAGS="-DTEST_$(CACHE_TEST)=1"

build-prepare:
	cmake -B build -S . -DCMAKE_CXX_FLAGS="-DPREPARING=1"
	cmake --build build

$(tests): build
	cd $(SAVE) && $(base_dir)/build/_deps/intelpin-src/pin -t $(base_dir)/build/lib$(TOOL).so -- runspec --size=test --noreportable $@

prepare-%: build-prepare
	cd $(SAVE) && $(base_dir)/build/_deps/intelpin-src/pin -t $(base_dir)/build/lib$(TOOL).so -- runspec --size=test --noreportable $*

prepare: $(foreach t,$(tests),prepare-$(t))

coremark: build
	cd $(SAVE) && $(base_dir)/build/_deps/intelpin-src/pin -t $(base_dir)/build/lib$(TOOL).so -- $(base_dir)/../lab1/workspace/coremark.exe

stats:
	python $(base_dir)/scripts/draw.py

stats2:
	python $(base_dir)/scripts/draw2.py

clean:
	-rm -rf build $(TOOL)*.txt

cache:
	mkdir -p $(base_dir)/data/cacheModel/$(CACHE_TEST)
	$(MAKE) -C $(base_dir) SAVE=$(base_dir)/data/cacheModel/$(CACHE_TEST) CACHE_TEST=$(CACHE_TEST)

.PHONY: build all coremark $(tests) clean stats stats2 prepare configure