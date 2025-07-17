enable_language(CSharp)

include(src.txt)

add_executable(csharp_bindings_bindgen ${SRC})
set_target_properties(csharp_bindings_bindgen PROPERTIES
	# DOTNET_TARGET_FRAMEWORK_VERSION "v4.8.1"
	DOTNET_TARGET_FRAMEWORK "net8.0"
	VS_PACKAGE_REFERENCES "CommandLineParser_2.9.1"
)

set_target_output_directory(TARGET csharp_bindings_bindgen)