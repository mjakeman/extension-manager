# Useful for updating the POTFILES file (run from this directory)

# Print out these specific data files
echo "data/com.mattjakeman.ExtensionManager.desktop.in.in"
echo "data/com.mattjakeman.ExtensionManager.metainfo.xml.in.in"

# Prints out all *.c and *.blp files in the project
pushd ../ > /dev/null
(find src -name *.c; find src -name *.blp) | sort
popd > /dev/null
