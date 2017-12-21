# 
#	reset 
#
set (hello)

set (hello_cpp
	test/hello/src/main.cpp
)
    
set (hello_h
# intentionally empty
)

# define the hello world program
set (hello
  ${hello_cpp}
  ${hello_h}
)
