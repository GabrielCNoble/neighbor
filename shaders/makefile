OBJ = $(wildcard *.vert).spv $(wildcard *.frag).spv

%.spv: %
	glsLangValidator -V $< -o $@

all: $(OBJ)

clean:
	del *.spv
	