
all: bin tool_output tool bin/tiny

bin/tiny: hashTable/hashTable.h tool_output/tinyGram.c \
stringListStack/stringListStack.h tool_output/tinyLex.c vm.c tiny.c
	gcc -O2 -march=native -fno-builtin-strlen -fno-builtin-memmove \
	-fno-builtin-memset -fno-stack-protector -o bin/tiny tiny.c \
	-Wall -Wextra -Wno-pointer-sign -Wno-unused-function
	size bin/tiny

tool_output/tinyLex.c: tinyLex.re
	re2c -i -W tinyLex.re -o tool_output/tinyLex.c

tool_output/tinyGram.c: tool/lemon tinyGram.y
	./tool/lemon tinyGram.y -s -l -dtool_output 
	sed -i 's/void tinyParse/static void tinyParse/g' \
	tool_output/tinyGram.c
	sed -i 's/static void yy_destructor(/#define yy_destructor(x,y,z) \n #if 0/g' \
	tool_output/tinyGram.c
	sed -i 's/stack once./hello*\/ #endif \/\*/g' \
	tool_output/tinyGram.c
	sed -i 's/YYMINORTYPE yyminorunion;/\/\/YYMINORTYPE yyminorunion;/g' \
	tool_output/tinyGram.c
	sed -i 's/yyminorunion.yy0 = yyminor;/\/\/yyminorunion.yy0 = yyminor;/g' \
	tool_output/tinyGram.c
	sed -i 's/static void yy_pop_parser_stack(yyParser \*pParser){/#define yy_pop_parser_stack(pParser) pParser->yytos-- \n#if 0/g' \
	tool_output/tinyGram.c
	sed -i 's/allocations from the parser/hello*\/ #endif \/\*/g' \
	tool_output/tinyGram.c
	sed -i 's/int tinyParseFallback(int iToken){/static inline int tinyParseFallback(int iToken){/g' \
	tool_output/tinyGram.c

tool/lemon: tool/lemon.c tool/lempar.c
	gcc -O2 tool/lemon.c -o tool/lemon

tool/lemon.c:
	curl https://raw.githubusercontent.com/sqlite/sqlite/master/tool/lemon.c \
	> tool/lemon.c

tool/lempar.c:
	curl https://raw.githubusercontent.com/sqlite/sqlite/master/tool/lempar.c \
	> tool/lempar.c

bin:
	mkdir bin

tool:
	mkdir tool

tool_output:
	mkdir tool_output

hashTable/hashTable.h:
	git clone https://github.com/cornerStone-Dev/hashTable.git

stringListStack/stringListStack.h:
	git clone https://github.com/cornerStone-Dev/stringListStack.git

test: bin/tiny
	time ./bin/tiny tinyTest.tiny

clean:
	rm -f bin/tiny
	rm -f tool_output/tinyLex.c
