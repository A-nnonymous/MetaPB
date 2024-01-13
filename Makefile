rebuild:
	cd build && rm -rf CMakeCache.txt CMakeFiles/ && cmake ../ -DCMAKE_BUILD_TYPE=Release && make -j
