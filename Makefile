all:
	meson setup build
	ninja -C build
run:
	./build/backroom
