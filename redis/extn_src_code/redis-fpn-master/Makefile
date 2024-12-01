
CURRENT_OS := $(shell sh -c 'uname -s 2>/dev/null || echo not')
CURRENT_DIR := $(abspath $(patsubst %/, %, $(dir $(abspath $(lastword $(MAKEFILE_LIST))))))

BASE_SOURCE_DIR := $(CURRENT_DIR)/src
SOURCE_DIR := $(BASE_SOURCE_DIR)/main
TEST_SOURCE_DIR := $(BASE_SOURCE_DIR)/test
SOURCE_DOCKER_GCC_DIR := $(CURRENT_DIR)/docker/gcc
SOURCE_DOCKER_REDIS_DIR := $(CURRENT_DIR)/docker/redis

TARGET_DIR := $(CURRENT_DIR)/target
OBJECT_DIR := $(TARGET_DIR)/object
TARGET_DOCKER_GCC_DIR := $(TARGET_DIR)/docker/gcc
TARGET_DOCKER_REDIS_DIR := $(TARGET_DIR)/docker/redis
CONTAINER_OUTPUT_DIR := $(TARGET_DIR)/container_output

VERSION := 1.1.8
TARGET_NAME := fpn_module-$(VERSION).so
TARGET := $(TARGET_DIR)/$(TARGET_NAME)

SOURCE_STRUCTURE := $(shell find $(SOURCE_DIR) -type d)
ALL_SOURCE_PATHS := $(addsuffix /*, $(SOURCE_STRUCTURE))
ALL_SOURCE_PATHS := $(wildcard $(ALL_SOURCE_PATHS))
ALL_SOURCE_PATHS := $(filter $(SOURCE_DIR)/%.c, $(ALL_SOURCE_PATHS))
ALL_OBJECT_PATHS := $(subst $(SOURCE_DIR), $(OBJECT_DIR), $(ALL_SOURCE_PATHS:%.c=%.o))
INTEGRATION_TEST_SOURCE := $(TEST_SOURCE_DIR)/integration_tests.sh
INTEGRATION_TEST_TARGET := $(TARGET_DIR)/integration_tests.sh


COMPILER = gcc
COMPILER_FLAGS = \
	-v \
	-Wall \
	-Wextra \
	-Wpedantic \
	-Wstrict-overflow \
	-fno-strict-aliasing \
	$(DEBUG_FLAGS) \
	-std=gnu99 \
	-O2 \
	-fPIC \
	-D MODULE_VERSION=\"$(VERSION)\"

ifeq ($(DEBUG), 1)
	COMPILER_FLAGS = -g -ggdb -O0
endif

LINKER = gcc
LINKER_FLAGS = \
	-v \
	-lc

ifeq ($(CURRENT_OS), Linux)
	COMPILER_FLAGS +=

	LINKER_FLAGS += \
		-shared \
		-Bsymbolic \
		-Bsymbolic-functions
else
  MIN_OSX_VERSION = 10.6

	COMPILER_FLAGS += \
		-dynamic \
		-mmacosx-version-min=$(MIN_OSX_VERSION)

	LINKER_FLAGS += \
		-bundle \
		-undefined dynamic_lookup \
		-mmacosx-version-min=$(MIN_OSX_VERSION)
endif


all: build

integration_tests: start_redis $(TARGET_DOCKER_REDIS_DIR)
	cp $(INTEGRATION_TEST_SOURCE) $(INTEGRATION_TEST_TARGET)
	$(INTEGRATION_TEST_TARGET)
	docker stop redis4_with_fpn

start_redis: container_build $(TARGET_DOCKER_REDIS_DIR)
	docker stop redis4_with_fpn || true && docker rm -f redis4_with_fpn || true
	cp -r $(CONTAINER_OUTPUT_DIR)/$(TARGET_NAME) $(SOURCE_DOCKER_REDIS_DIR)/ $(TARGET_DOCKER_REDIS_DIR)
	docker build --rm -t redis4_with_fpn $(TARGET_DOCKER_REDIS_DIR)
	docker run --privileged -d -p 6379:6379 --name redis4_with_fpn redis4_with_fpn

container_build: $(CONTAINER_OUTPUT_DIR) $(TARGET_DOCKER_GCC_DIR)
	cp -r $(CURRENT_DIR)/Makefile $(BASE_SOURCE_DIR) $(SOURCE_DOCKER_GCC_DIR)/ $(TARGET_DOCKER_GCC_DIR)
	docker build --rm -t gcc_redis_module_build $(TARGET_DOCKER_GCC_DIR)
	docker run --privileged -v $(CONTAINER_OUTPUT_DIR)/:/target/ gcc_redis_module_build

build: $(ALL_OBJECT_PATHS)
	$(LINKER) -o $(TARGET) $^ $(LINKER_FLAGS)

$(OBJECT_DIR)/%.o: $(SOURCE_DIR)/%.c $(OBJECT_DIR)
	$(COMPILER) $(COMPILER_FLAGS) -c $< -o $@

$(OBJECT_DIR):
	mkdir -p $(OBJECT_DIR)

$(TARGET_DOCKER_GCC_DIR):
	mkdir -p $(TARGET_DOCKER_GCC_DIR)

$(TARGET_DOCKER_REDIS_DIR):
	mkdir -p $(TARGET_DOCKER_REDIS_DIR)

$(CONTAINER_OUTPUT_DIR):
	mkdir -p $(CONTAINER_OUTPUT_DIR)

clean:
	rm -rf $(TARGET_DIR)
