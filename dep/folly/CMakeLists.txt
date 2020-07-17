# folly (v2017.09.11.00) (https://github.com/facebook/folly)

file(GLOB_RECURSE sources folly-stripped/*)
add_library(folly STATIC ${sources})
target_include_directories(folly SYSTEM PUBLIC folly-stripped/)
target_link_libraries(folly PRIVATE boost)

set_target_properties(folly
  PROPERTIES
	FOLDER
	  "dep")
