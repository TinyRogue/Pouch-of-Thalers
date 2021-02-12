TARGET_EXEC := player

CC := gcc
BUILD_DIR := ../build/$(TARGET_EXEC)
SRC_DIRS := .

SRCS := $(shell find $(SRC_DIRS) -name '*.c')
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
CFLAGS := -std=c11 -O2 -Wno-deprecated -Wall -Wextra -pedantic -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wunreachable-code -Wfloat-equal -D_XOPEN_SOURCE=500 -g
LDFLAGS := -lrt -lpthread -lncurses
VFLAGS := --leak-check=full --track-fds=yes

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	@echo "\033[1;34mFinal compilition:\033[0;37m"
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	@echo "\033[1;34mCompleted.\n" "\033[0;37m"

$(BUILD_DIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	@echo "\033[1;33mBuilding" $< "\033[0;37m"
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "\033[1;33mCompleted.\n" "\033[0;37m"

run:
	./$(BUILD_DIR)/$(TARGET_EXEC)

bug:
	gdb ./$(BUILD_DIR)/$(TARGET_EXEC)

val:
	@valgrind $(VFLAGS) ./$(BUILD_DIR)/$(TARGET_EXEC)

.PHONY: clean
clean:
	@echo "\033[1;31mDeleting -r " $(BUILD_DIR) "\033[0;37m"
	@rm -r $(BUILD_DIR)

rebuild:
	make clean && make