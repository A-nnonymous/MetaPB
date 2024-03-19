archive:
	touch paperOutput/Thesis.pdf 
	mkdir -p ./paperArchive && mv ./paperOutput/Thesis.pdf ./paperArchive/Thesis_$(date +%m_%d_%Hh_%Mm).pdf

rebuild:
	cd build && rm -rf ./ && cmake ../ -DCMAKE_BUILD_TYPE=Release && make -j


paper:
	cd submodules/paper && ./artratex.sh xa Thesis.tex
	mv ./submodules/paper/Tmp/Thesis.pdf paperOutput/Thesis.pdf

new_paper:
	make archive
	make rebuild
	make visualize
	make paper