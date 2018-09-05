proj = melodeergui
objects = main.o mdgui.o mdguifilebox.o mdguimeta.o
libs = melodeer pthread ncurses tinfo

srcdir = source
builddir = build
depsdir = include

$(proj): $(objects)
	gcc $(addprefix $(builddir)/,$^) $(addprefix -l,$(libs)) -o $(proj)

%.o: $(srcdir)/%.c $(depsdir)/%.h
	gcc -c $< -o $(addprefix $(builddir)/,$@) -I$(depsdir) -O3

main.o: $(srcdir)/main.c
	-mkdir $(builddir)
	gcc -c $< -o $(addprefix $(builddir)/,$@) -I$(depsdir) -O3

.PHONY=clean
clean:
	-rm $(builddir)/* $(proj)

.PHONY=run
run:
	make $(proj)
	./$(proj)

.PHONY=debug
debug:
	make $(proj)
	valgrind -v --leak-check ./$(proj)
