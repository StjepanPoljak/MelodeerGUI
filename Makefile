proj = melodeergui
objects = main.o mdgui.o mdguifilebox.o mdguimeta.o mdguilistbox.o mdguistrarr.o mdguibox.o
libs = melodeer pthread ncurses

srcdir = source
builddir = build
depsdir = include

$(proj): $(objects)
	gcc $(addprefix $(builddir)/,$^) $(addprefix -l,$(libs)) -o $(proj) -g
	@-./save.sh

%.o: $(srcdir)/%.c $(depsdir)/%.h
	gcc -c $< -o $(addprefix $(builddir)/,$@) -I$(depsdir) -g

main.o: $(srcdir)/main.c
	-mkdir $(builddir)
	gcc -c $< -o $(addprefix $(builddir)/,$@) -I$(depsdir) -g

.PHONY=clean
clean:
	-rm $(builddir)/* $(proj)
	-rm *.log

.PHONY=run
run:
	make $(proj)
	./$(proj)

.PHONY=debug
debug:
	make $(proj)
	valgrind -v --leak-check=full --track-origins=yes ./$(proj)
