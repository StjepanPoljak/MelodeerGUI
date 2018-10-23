proj = melodeergui
objects = main.o mdgui.o mdguifilebox.o mdguimeta.o mdguilistbox.o mdguistrarr.o mdguibox.o mdguiplaylist.o
libs = melodeer pthread ncurses m

srcdir = source
builddir = build
depsdir = include

$(proj): $(objects)
	gcc $(addprefix $(builddir)/,$^) $(addprefix -l,$(libs)) -o $(proj)
	@-./save.sh

%.o: $(srcdir)/%.c $(depsdir)/%.h
	gcc -c $< -o $(addprefix $(builddir)/,$@) -I$(depsdir)

main.o: $(srcdir)/main.c
	-mkdir $(builddir)
	gcc -c $< -o $(addprefix $(builddir)/,$@) -I$(depsdir)

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
	valgrind -v --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(proj)
