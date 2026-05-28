CXX = g++
CXXFLAGS = -std=c++17 -g -O0 -Wall -Wextra -I. -I..

TARGET = test_parser
SOURCES = AST.cpp SqlParser.cpp test_parser.cpp
GENERATED = lexer.cpp parser.cpp parser.h
OBJECTS = $(SOURCES:.cpp=.o) lexer.o parser.o

all: $(TARGET)

# Генерируем парсер и лексер перед компиляцией
generate: parser.h lexer.cpp

# Генерация парсера
parser.h parser.cpp: parser.y
	bison -d -o parser.cpp parser.y --defines=parser.h

# Генерация лексера (зависит от parser.h)
lexer.cpp: lexer.l parser.h
	flex++ -o lexer.cpp lexer.l

# Объектные файлы зависят от сгенерированных заголовков
AST.o: AST.cpp AST.h ../engine/Schema.h ../utils/Value.h
SqlParser.o: SqlParser.cpp SqlParser.h AST.h parser.h SqlScanner.h
test_parser.o: test_parser.cpp SqlParser.h AST.h
lexer.o: lexer.cpp parser.h SqlScanner.h
parser.o: parser.cpp parser.h AST.h ../utils/Value.h

# Правило компиляции .cpp в .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Линковка (зависит от сгенерированных объектных файлов)
$(TARGET): generate $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS)

clean:
	rm -f $(GENERATED) $(OBJECTS) $(TARGET) *.hh

check: $(TARGET)
	./$(TARGET) test_queries.sql

.PHONY: all clean check generate