
# create hierarchical source groups based on a dir tree
#
# EXAMPLE USAGE:
#
#  create_source_group ( "Header Files" ${PROJ_SRC_DIR} "${PROJ_HEADERS}" )
#  create_source_group ( "Source Files" ${PROJ_SRC_DIR} "${PROJ_SOURCES}" )

macro ( create_source_group GroupPrefix RootDir ProjectSources  )
  set ( DirSources ${ProjectSources} )
  foreach ( Source ${DirSources} )
    string ( REGEX REPLACE "${RootDir}" "" RelativePath "${Source}" )
    string ( REGEX REPLACE "[\\\\/][^\\\\/]*$" "" RelativePath "${RelativePath}" )
    string ( REGEX REPLACE "^[\\\\/]" "" RelativePath "${RelativePath}" )
    string ( REGEX REPLACE "/" "\\\\\\\\" RelativePath "${RelativePath}" )
    source_group ( "${GroupPrefix}\\${RelativePath}" FILES ${Source} )
  endforeach ( Source )
endmacro ( create_source_group )


