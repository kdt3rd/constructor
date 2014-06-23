
.DEFAULT: all
.ONESHELL:

.phony: clean all

OUTPUT:= Build

SRC:= $(OUTPUT)/item.o $(OUTPUT)/main.o

CXXFLAGS := --std=c++11 -Wall

all: constructor

$(OUTPUT)/item.o: src/item.cpp | $(OUTPUT)
	g++ $(CXXFLAGS) -c -MMD -MF $(OUTPUT)/item.dep -o $@ $<

$(OUTPUT)/main.o: src/main.cpp | $(OUTPUT)
	g++ $(CXXFLAGS) -c -MMD -MF $(OUTPUT)/main.dep -o $@ $<

constructor: $(SRC) | $(OUTPUT)
	g++ $(CXXFLAGS) -o $(OUTPUT)/constructor $^

$(OUTPUT):
	mkdir $(OUTPUT)

clean:
	rm -rf $(OUTPUT)

-include $(OUTPUT)/item.dep
-include $(OUTPUT)/main.dep
