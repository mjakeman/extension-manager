gdbus-codegen --interface-prefix org.gnome.Shell. \
	--generate-c-code shell-dbus-interface \
	--c-namespace Shell \
	--c-generate-object-manager \
	org.gnome.Shell.Extensions.xml