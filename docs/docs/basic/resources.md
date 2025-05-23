# Resources

Any files required for the application to run can be embedded directly into the executable as resources.
Common examples include fonts and images, but any file type is supported.

The embedded resources are stored in the read-only data section of the resulting binary.
Optionally, compression can be applied to reduce the final executable size.

Resources are stored with associated keys (similar to file paths), which are used to load them at runtime.

Each CMake target can define its own resources by specifying a unique key, the input file path, and optionally, compression.

If a target depends on another target, it can override a specific resource by providing a different file path under the same key.
During the linking process, Brisk traverses the dependency tree and collects the list of resources to bundle into the executable.

## Example Usage

### CMake Configuration

```cmake
add_library(utilities utilities.cpp)

# Add a resource named 'table' from 'data/table.csv'
# and compress it using Brotli.
brisk_target_resource(utilities table INPUT data/table.csv BROTLI)

add_executable(main main.cpp)

# Add an icon from 'icon.png' as a resource to the main executable.
brisk_target_resource(main icon INPUT icon.png)

# Link the main target to the utilities library
# The icon resource will be available for main too.
target_link_libraries(main PRIVATE utilities)

# The following command replaces the "table" resource 
# provided by the utilities target with another file.
brisk_target_resource(utilities table INPUT data/another-table.csv LZ4)

# Must be placed after the last brisk_target_resource
# to bundle all resources into the executable.
brisk_bundle_resources(main)

# Note: The brisk_setup_executable cmake function
# automatically calls brisk_bundle_resources
# to bundle all required resource.
```

!!! note "Note"
    More methods for bundling resources with Brisk applications are planned for future releases. Examples include the `Resources` folder in macOS bundles and resources in the Win32 PE format.

### C++ Code

In C++, you can load any resource bundled with the executable using `Resources::load` or `Resources::loadText`:

```c++
#include <brisk/core/Resources.hpp>
#include <brisk/graphics/Images.hpp>

// Load the icon resource and decode it into an image.
Rc<Image> loadIcon() {
    // Assume the resource exists and ignore errors for simplicity.
    return imageDecode(Resources::load("icon"), ImageFormat::RGBA).value();
}

void loadTable() {
    // Load the 'table' resource as a text string.
    std::string t = Resources::loadText("table");

    // Process the table data...
}
```
