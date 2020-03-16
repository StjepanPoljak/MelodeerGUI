proj = melodeergui

ifeq ($(CC), cc)
	CC = gcc
endif

objects = main.o mdgui.o mdguidraw.o mdguifilebox.o mdguimeta.o mdguilistbox.o mdguistrarr.o mdguibox.o mdguiplaylist.o

srcdir = source
builddir = build
depsdir = include

LDLIBS := -lmelodeer -pthread -lm
CFLAGS += -I$(depsdir)

$(proj): $(objects)
	$(CC) $(addprefix $(builddir)/,$^) $(LDLIBS) -o $@ $(LDFLAGS)
	@-./save.sh

%.o: $(srcdir)/%.c $(depsdir)/%.h
	$(CC) $(CFLAGS) -c $< -o $(addprefix $(builddir)/,$@)

main.o: $(srcdir)/main.c
	-mkdir $(builddir)
	$(CC) $(CFLAGS) -c $< -o $(addprefix $(builddir)/,$@)

.PHONY=clean run
clean:
	@-rm $(builddir)/* $(proj)
	@-rm *.log

run: $(proj)
	./$(proj)
