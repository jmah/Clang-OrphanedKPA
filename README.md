# About

This clang plug-in emits a warning when declaring [dependent key paths](1) using `+keyPathsForValuesAffecting<Foo>`, but the corresponding `-<foo>` key doesn't seem to exist. This can happen from typos or a rename that changed the key but not the dependent keys method.

[1]: http://developer.apple.com/library/ios/#documentation/cocoa/conceptual/KeyValueObserving/Articles/KVODependentKeys.html

![Warning in Xcode](https://raw.github.com/jmah/Clang-OrphanedKPA/screenshots/xcode-warning.png)

# Compiling

First, check out llvm and clang. Clone this repository into `llvm/tools/clang/examples/`, and run `make`.

# Using in Xcode

As of Xcode 4.6.2, the included clang doesn't seem to load plugins, so building a recent version of clang is required.

Use build settings like this:

    CC = /path/to/llvm/Release/bin/clang;
    OTHER_CFLAGS = (
         "-Xclang",
        "-load",
        "-Xclang",
        /path/to/llvm/Release/lib/libOrphanedKeyPathsAffecting
        "-Xclang",
        "-add-plugin",
        "-Xclang",
        "check-orphaned-key-paths-affecting",
    );

