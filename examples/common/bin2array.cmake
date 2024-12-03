# Creates C resources file and a header file from files in the given directory recursively
function(create_resources dir output)
    # Derive the header file name by replacing .c with .h in the output file name
    string(REPLACE ".c" ".h" header_file "${output}")

    # Create empty output files
    file(WRITE ${output} "#include <stdint.h>\n\n")
    file(WRITE ${header_file} "#ifndef BINARIES_H\n#define BINARIES_H\n\n#include <stdint.h>\n\n")

    # Collect input files
    file(GLOB bin_paths ${dir}/ESP*/*)

    # Iterate through input files
    foreach(bin ${bin_paths})
        # Get short filenames, by discarding relative path
        file(RELATIVE_PATH name "${dir}" "${bin}")
        # Replace filename spaces & extension separator for C compatibility
        string(REGEX REPLACE "[\\./-]" "_" filename ${name})
        # Read hex data from file
        file(READ "${bin}" filedata HEX)
        # Convert hex data for C compatibility
        string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," filedata ${filedata})
        # Append data to C source file
        file(APPEND ${output} "const uint8_t  ${filename}[] = {${filedata}};\n")
        file(APPEND ${output} "const uint32_t ${filename}_size = sizeof(${filename});\n")
        # Append references to the header file
        file(APPEND ${header_file} "extern const uint8_t  ${filename}[];\n")
        file(APPEND ${header_file} "extern const uint32_t ${filename}_size;\n")
    endforeach()

    # Finalize header file
    file(APPEND ${header_file} "\n#endif // BINARIES_H\n")
endfunction()
