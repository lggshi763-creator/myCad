# Sphinx configuration for myCad API reference.
#
# Sphinx consumes the XML output produced by Doxygen (configured at
# project-root /Doxyfile, output at build/docs/doxygen/xml) via the
# Breathe extension, and renders an HTML site at build/docs/sphinx/.
#
# Run from project root:
#   doxygen Doxyfile
#   python -m sphinx -b html docs/api-reference build/docs/sphinx
#
# Or use the CMake target:
#   cmake --build build/<preset> --target docs

from __future__ import annotations

import os
from pathlib import Path

# -----------------------------------------------------------------------------
# Paths
# -----------------------------------------------------------------------------
# This file lives at <project>/docs/api-reference/conf.py.
HERE         = Path(__file__).resolve().parent
PROJECT_ROOT = HERE.parents[1]

# Doxygen XML output. The CMake target sets MYCAD_DOXYGEN_XML to the actual
# build dir (preset-specific). When invoked manually, fall back to the
# default location used by Doxyfile (build/docs/doxygen/xml).
DOXYGEN_XML = Path(
    os.environ.get(
        "MYCAD_DOXYGEN_XML",
        str(PROJECT_ROOT / "build" / "docs" / "doxygen" / "xml"),
    )
).resolve()

# -----------------------------------------------------------------------------
# Project metadata
# -----------------------------------------------------------------------------
project   = "myCad"
author    = "myCad authors"
copyright = "2026, myCad authors"  # noqa: A001 (sphinx requires this name)

# Pulled from the project's CMakeLists.txt VERSION 0.0.1; bump together.
version = "0.0.1"
release = version

language = "en"  # UI labels in English; prose can mix English + Chinese.

# -----------------------------------------------------------------------------
# Extensions
# -----------------------------------------------------------------------------
extensions = [
    "breathe",                       # Doxygen XML -> Sphinx
    "sphinx.ext.autosectionlabel",   # cross-ref headings by their text
    "sphinx.ext.intersphinx",        # link to cppreference, etc.
    "sphinx.ext.todo",               # .. todo:: directives
]

# -----------------------------------------------------------------------------
# Breathe (the Doxygen bridge)
# -----------------------------------------------------------------------------
breathe_projects = {
    "mycad": str(DOXYGEN_XML),
}
breathe_default_project    = "mycad"
breathe_default_members    = ("members", "undoc-members")
breathe_show_define_initializer    = False
breathe_show_enumvalue_initializer = True
breathe_separate_member_pages      = False

# -----------------------------------------------------------------------------
# autosectionlabel — give every heading a stable anchor
# -----------------------------------------------------------------------------
autosectionlabel_prefix_document = True
autosectionlabel_maxdepth        = 3

# -----------------------------------------------------------------------------
# todo
# -----------------------------------------------------------------------------
todo_include_todos = True

# -----------------------------------------------------------------------------
# Source files
# -----------------------------------------------------------------------------
master_doc       = "index"
source_suffix    = {".rst": "restructuredtext"}
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]

# Suppress noisy warnings that don't reflect real problems.
suppress_warnings = [
    "autosectionlabel.*",  # duplicate-label warnings across files we don't ref
]

# -----------------------------------------------------------------------------
# HTML output
# -----------------------------------------------------------------------------
# We avoid third-party themes so the docs build with the stock Sphinx install.
# Switch to 'furo' or 'sphinx_rtd_theme' once those become a hard dependency.
html_theme       = "alabaster"
html_static_path = ["_static"] if (HERE / "_static").is_dir() else []
html_title       = f"{project} {release} — API Reference"
html_short_title = f"{project} API"

html_theme_options = {
    "description":   "Open-source parametric CAD",
    "fixed_sidebar": True,
    "show_powered_by": False,
}

# -----------------------------------------------------------------------------
# C++ defaults for cppreference cross-refs
# -----------------------------------------------------------------------------
cpp_index_common_prefix = ["mycad::"]

# Highlight C++20 by default for code blocks without an explicit language tag.
highlight_language = "cpp"
pygments_style     = "default"

# -----------------------------------------------------------------------------
# intersphinx
# -----------------------------------------------------------------------------
intersphinx_mapping = {
    # Add later if the project starts using Python tooling docs.
}
intersphinx_disabled_reftypes = ["std:doc"]

# -----------------------------------------------------------------------------
# Sanity warnings to the build console
# -----------------------------------------------------------------------------
def _warn_if_no_xml(_app, _config):  # noqa: ANN001 (Sphinx hook signature)
    if not DOXYGEN_XML.is_dir() or not any(DOXYGEN_XML.iterdir()):
        # Plain print so the warning shows up even before Sphinx logger init.
        print(
            f"[myCad-conf] WARNING: Doxygen XML not found at {DOXYGEN_XML}. "
            "Run 'doxygen Doxyfile' first, or set MYCAD_DOXYGEN_XML."
        )

def setup(app):  # noqa: ANN001
    app.connect("config-inited", _warn_if_no_xml)
    return {"version": "0.0.1", "parallel_read_safe": True}
