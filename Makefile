CMAKEARGS+=$(if $(D),-DCMAKE_BUILD_TYPE=Debug,-DCMAKE_BUILD_TYPE=Release)
CMAKEARGS+=$(if $(COV),-DOPT_COV=1)
CMAKEARGS+=$(if $(PROF),-DOPT_PROF=1)
CMAKEARGS+=$(if $(LIBCXX),-DOPT_LIBCXX=1)
cmake:
	cmake -B build . $(CMAKEARGS)
	$(MAKE) -C build $(if $(V),VERBOSE=1)

vc:
	"C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" -G"Visual Studio 16 2019" -B build . $(CMAKEARGS)
	"C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/MSBuild/Current/Bin/amd64/MSBuild.exe" build/*.sln -t:Rebuild

ctest: TEST=1
ctest: cmake
	cd build && ctest --verbose


clean: 
	$(RM) -r build CMakeFiles CMakeCache.txt CMakeOutput.log


