project(Btk)

aux_source_directory(./src, SOURCE_LIST)

add_library(libbtk, ${SOURCE_LIST})