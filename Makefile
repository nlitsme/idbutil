cmake:
	cmake -B build . $(if $(D),-DCMAKE_BUILD_TYPE=Debug,-DCMAKE_BUILD_TYPE=Release) $(CMAKEARGS)
	$(MAKE) -C build $(if $(V),VERBOSE=1)

clean: 
	$(RM) -r build CMakeFiles CMakeCache.txt CMakeOutput.log


