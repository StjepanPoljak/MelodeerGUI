proj = melodeergui
objects = main.o mdgui.o mdguifilebox.o mdguimeta.o
libs = openal FLAC pthread mp3lame ncurses

melodeerbuild = mdcore.o mdflac.o mdwav.o mdlame.o
melodir = ../Melodeer/build/
meloinclude = ../Melodeer/include
srcdir = source
builddir = build
depsdir = include

$(proj): $(objects)
	gcc $(addprefix $(builddir)/,$^) $(addprefix $(melodir),$(melodeerbuild)) $(addprefix -l,$(libs)) -o $(proj)

%.o: $(srcdir)/%.c $(depsdir)/%.h
	gcc -c $< -o $(addprefix $(builddir)/,$@) -I$(depsdir)

main.o: $(srcdir)/main.c
	-mkdir $(builddir)
	gcc -c $< -o $(addprefix $(builddir)/,$@) -I$(depsdir) -I$(meloinclude)

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
