all: build/goloc-llvm

OBJS = build/parser.o  \
       build/codegen.o \
       build/main.o    \
       build/tokens.o  \
       build/corefn.o  \
       build/golo-llvm.o  \

CPPFLAGS = -I. `llvm-config --cppflags --ldflags --libs core jit native bitwriter`
LDFLAGS  = `llvm-config --cppflags --ldflags --libs core jit native bitwriter`
LIBS     = `llvm-config --cppflags --ldflags --libs core jit native bitwriter`

clean: clean_tmp clean_build
	$(RM) -rf $(OBJS)

build/parser.cpp: src/parser.y
	bison -v -d -o $@ $^
	
build/parser.hpp: build/parser.cpp

src/tokens.cpp: src/tokens.l build/parser.hpp
	flex -v -o $@ $^

build/%.o: src/%.cpp
	g++ -c $(CPPFLAGS) -o $@ $<

build/goloc-llvm: $(OBJS)
	g++ -o $@ $(OBJS) $(LIBS) $(LDFLAGS)

clean_tmp:
	rm -f tmp/*

clean_build:
	rm -f build/*

test: build/goloc-llvm clean_tmp
	./goloc-llvm test/example.golo tmp/example.native && tmp/example.native
