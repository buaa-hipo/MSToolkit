add_subdirectory(jsirun)
#add_subdirectory(jsiwrap)
add_subdirectory(jsiread)
# add_subdirectory(analysis)
# add_subdirectory(compress)
# add_subdirectory(decompress)
add_subdirectory(jsiconvert)

if (USE_MATRIX) 
    add_subdirectory(mt_instrumenter)
endif()

#add_subdirectory(gvarp)

