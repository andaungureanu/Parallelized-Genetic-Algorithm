# Octombrie 2021

build:
	@echo "Building..."
	@gcc -o main main.c genetic_algorithm.c -lm -Wall -Werror -lpthread
	@echo "Done"

build_debug:
	@echo "Building debug..."
	@gcc -o main main.c genetic_algorithm.c -lm -Wall -Werror -O0 -g3 -DDEBUG
	@echo "Done"

clean:
	@echo "Cleaning..."
	@rm -rf main
	@echo "Done"
