add_rules("mode.debug", "mode.release")

add_requires("cargo::cxx 1.0")

target("branch_prediction")
    set_kind("static")
    add_files("src/bp.rs")
    set_values("rust.cratetype", "staticlib")
    add_packages("cargo::cxx")

target("test")
    set_kind("binary")
    add_rules("rust.cxxbridge")
    add_deps("branch_prediction")
    add_files("src/main.cc")
    add_files("src/bridge.rsx")
