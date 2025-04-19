# 编译器配置
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O3 -pthread
DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d

# 目录配置
SRC_DIR := src
BUILD_DIR := build
TARGET := httpserver

# 自动扫描源文件
SRCS := $(shell find $(SRC_DIR) -name '*.cpp')
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

# 头文件路径
INC_DIRS := include
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

# 主规则
all: $(TARGET)

$(TARGET): $(OBJS) main.cpp
	@echo "Linking $@..."
	@$(CXX) $(CXXFLAGS) $(INC_FLAGS) $^ -o $@
	@echo "Build successful!"

# 模式规则处理cpp文件
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) $(INC_FLAGS) $(DEPFLAGS) -c $< -o $@

# 包含自动生成的依赖
-include $(DEPS)

# 清理规则
.PHONY:clean
clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD_DIR) $(TARGET)
	@echo "Clean complete"
