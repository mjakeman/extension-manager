# Prints out all *.c and *.blp files in the project
# Useful for updating the POTFILES file (run from this directory)
pushd ../ > /dev/null
(find src -name *.c; find src -name *.blp) | sort
popd > /dev/null
