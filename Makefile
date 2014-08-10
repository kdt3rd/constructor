
.DEFAULT: all
.ONESHELL:

.phony: clean all constructor

OUTPUT:= Build

SRC:= item.cpp main.cpp variable.cpp

SRC:=$(addprefix $(OUTPUT)/,$(SRC))

CXXFLAGS := --std=c++11 -Wall -Os

all: constructor

constructor: $(OUTPUT)/constructor

$(OUTPUT)/%.o: src/%.cpp | $(OUTPUT)
	@g++ $(CXXFLAGS) -c -MMD -MF $(OUTPUT)/$*.dep -o $@ $<

$(OUTPUT)/constructor: $(SRC:.cpp=.o)
	@g++ $(CXXFLAGS) -o $(OUTPUT)/constructor $^

$(OUTPUT):
	mkdir $(OUTPUT)

clean:
	rm -rf $(OUTPUT)

-include $(SRC:.o=.dep)
