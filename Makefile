# Basic config
CXX       := ccache g++
CXXFLAGS  := -MMD -MP
LDFLAGS   := -lclang
LTO_FLAGS := -flto -fno-fat-lto-objects

CXXFLAGS += -std=c++23 -march=native -Iinclude -fexceptions -lreadline
LDFLAGS  += -lstdc++exp

# Directories
SRC_DIR := src
INC_DIR := include
OBJ_DIR := obj
BIN_DIR := bin

# Target
TARGET := $(BIN_DIR)/lambda

SRCS := $(shell find $(SRC_DIR) -name '*.cpp')

OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

all: deploy

segfault: CXXFLAGS += -g3 -Og -D_GLIBCXX_DEBUG -D_GLIBCXX_ASSERTIONS -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3 -fstack-protector-all -fstack-clash-protection -fsanitize=undefined -fsanitize=leak -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow -fno-omit-frame-pointer -fPIE -pie -Wl,-z,now -Wl,-z,relro -Wl,-z,noexecstack -fno-common
segfault: $(TARGET)

deploy: CXXFLAGS += -O3 -fomit-frame-pointer
deploy: $(TARGET)

shy: CXXFLAGS += -Oz -s -fno-asynchronous-unwind-tables -fno-plt -ffunction-sections -fdata-sections -fno-stack-protector -fno-ident -fvisibility=hidden -fvisibility-inlines-hidden -Wl,--gc-sections -Wl,--build-id=none
shy: $(TARGET)
	@strip --strip-all --remove-section=.comment --remove-section=.note $(TARGET)
	@sstrip $(TARGET) || echo "⚠️ sstrip no encontrado, saltando..."
	@upx --ultra-brute $(TARGET) || echo "⚠️ upx no encontrado, saltando..."

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "Enlazando $@"
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Compilando $<"
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo "Limpiando archivos de compilación..."
	@rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean

-include $(OBJS:.o=.d)
