CC = gcc
simple_tree: simple_tree.c
	gcc simple_tree.c -o simple_tree

clean:
	rm -f simple_tree

run: simple_tree
	./simple_tree
