all: firmware.syx

fmt:
	find src/ -type f -exec clang-format -i '{}' +
	find include/ -type f -exec clang-format -i '{}' +

check_fmt:
	find src/ -type f -exec clang-format --dry-run --Werror '{}' +
	find include/ -type f -exec clang-format --dry-run --Werror '{}' +

.pio/build/ATmega1284P/firmware.hex: $(wildcard src/* include/*)
	platformio run
	touch $@ # because platformio is strange

firmware.syx: .pio/build/ATmega1284P/firmware.hex
	PYTHONPATH=3rdparty python2 3rdparty/tools/hex2sysex/hex2sysex.py -s -o $@ -v 0x7f $<

TESTS=mono_voice_tracker poly_voice_allocator glide voice_state util

test: .build/test/test
	./$<

.build/test/test: $(addprefix .build/test/,$(addsuffix .o,$(TESTS))) .build/test/catch_amalgamated.o
	g++ $^ -o $@

.build/test/catch_amalgamated.o: test/3rdparty/catch_amalgamated.cpp
	g++ -c $< -o $@

.build/test/%.o: test/%.cpp include/voice_allocation.hpp include/voice_state.hpp
	g++ -c -Iinclude/ $< -o $@

compile_commands.json:
	pio run -t compiledb
	mv compile_commands.json .compile_commands.tmp
	echo '[' > compile_commands.json
	for test in $(TESTS); do echo '    { "command": "g++ -Iinclude -c test/'$$test'.cpp", "directory": "'$(PWD)'", "file": "test/'$$test'.cpp" },' >> compile_commands.json; done
	# remove the first [
	tail -n +2 .compile_commands.tmp >> compile_commands.json


clean:
	platformio run -t clean
	rm -f firmware.syx
	rm -f .build/test/*.o
	rm -f .build/test/test

.PHONY: clean fmt check_fmt compile_commands.json test
