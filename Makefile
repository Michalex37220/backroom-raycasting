NAME = backroom
BUILD_DIR = build

all:
	meson setup $(BUILD_DIR) --wipe
	ninja -C $(BUILD_DIR)

build:
	ninja -C $(BUILD_DIR)

run:
	./$(BUILD_DIR)/$(NAME)

clean:
	rm -rf $(BUILD_DIR)

re: clean all
