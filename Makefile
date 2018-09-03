proj = melodeergui
objects = main.o mdgui.o mdguifilebox.o mdguimeta.o
libs = openal FLAC pthread mp3lame

melodir = lib
meloinclude = ../Melodeer/include
srcdir = source
builddir = build
depsdir = include

$(proj): $(objects)
	gcc $(addprefix $(builddir)/,$^) $(addprefix -l,$(libs)) $(melodir)/* -o $(proj)

%.o: $(srcdir)/%.c $(depsdir)/%.h
	gcc -c $< -o $(addprefix $(builddir)/,$@) -I$(depsdir) -O3

main.o: $(srcdir)/main.c
	-mkdir $(builddir)
	gcc -c $< -o $(addprefix $(builddir)/,$@) -I$(depsdir) -I$(meloinclude) -O3

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
