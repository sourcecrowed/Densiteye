

densiteye:main.cpp makefile out/Densiteye.o
	g++ -o densiteye main.cpp out/Densiteye.o -I./include -std=c++23 -O3 -lsfml-graphics -lsfml-window -lsfml-system -lfmt
	
out/Densiteye.o: include/Densiteye/Densiteye.hpp src/Densiteye/Densiteye.cpp
	mkdir -p out/ && g++ -c -o out/Densiteye.o src/Densiteye/Densiteye.cpp -I./include -std=c++23 -O3 -lsfml-graphics -lsfml-system -lfmt



# main:main.cpp makefile
# 	g++ -o main main.cpp -I./include -std=c++23 -O3 -lsfml-graphics -lsfml-window -lsfml-system -lfmt
