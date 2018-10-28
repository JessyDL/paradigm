# Meta
A small library that handles generating, and maintaining the meta library and data for files. It will generate UID's and keep track of them, as well as keep track which files exist on disk (in the library when loaded), and which ones are generated during runtime (added later on in the library).
It supports polymorphic serialization of its metadata through the `format` project.
To generate a library file, use the `assembler` project.