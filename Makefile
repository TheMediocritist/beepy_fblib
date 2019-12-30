CXX = g++
LIBS = -lfreetype
CFLAGS = -I/usr/include/freetype2
src = $(wildcard *.cpp)
obj = $(src:.cpp=.o)
dep = $(obj:.o=.d)

fbtest: $(obj)
	$(CXX) -o $@ $(LIBS) $(CFLAGS) $^

-include $(dep)

# rule to generate dependency files using C preprocessor
%.d: %.cpp
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

.PHONY: clean
clean:
	rm -r $(obj) $(dep) fbtest
